use std::borrow::Cow;
use std::sync::Arc;

use tokio::sync::Mutex;

use crate::close::{parse_close_payload, sanitize_close_code};
use crate::error::{Error, Result};
#[cfg(feature = "deflate")]
use crate::extensions::permessage_deflate::{compress_message, decompress_message};
use crate::frame::{Frame, FrameBuilder, Opcode, ParseResult, Parser};
use crate::transport::IoStream;
use crate::types::{Message, Role};
use crate::utf8;

/// Established WebSocket session over an async TCP/TLS stream.
///
/// Prefer [`read_message`](Self::read_message) for application data; it handles fragmentation,
/// permessage-deflate, ping/pong, and close echo.
pub struct Connection {
    stream: Arc<Mutex<IoStream>>,
    role: Role,
    parser: Mutex<Parser>,
    open: Mutex<bool>,
    permessage_deflate: Mutex<bool>,
    outbound: Mutex<Vec<u8>>,
    fragment: Mutex<FragmentState>,
    last_close: Mutex<Option<(u16, String)>>,
    read_state: Mutex<ReadState>,
}

/// Socket read chunk size; larger reads reduce syscalls for big payloads.
const READ_CHUNK: usize = 64 * 1024;

/// Persistent inbound buffer so `read_frame` neither reallocates per call nor
/// loses bytes pipelined after a completed frame.
struct ReadState {
    /// Unparsed bytes accumulated from the socket.
    buf: Vec<u8>,
    /// Parse cursor into `buf` (bytes before it are already consumed).
    pos: usize,
    /// Reusable destination for socket reads.
    scratch: Vec<u8>,
}

struct FragmentState {
    active: bool,
    buffer: Vec<u8>,
    opcode: Opcode,
    compressed: bool,
}

impl FragmentState {
    fn reset(&mut self) {
        self.active = false;
        self.buffer.clear();
        self.opcode = Opcode::Text;
        self.compressed = false;
    }
}

impl Connection {
    /// Wraps an upgraded stream (call after [`client_upgrade`](crate::handshake::client_upgrade) or
    /// [`server_upgrade`](crate::handshake::server_upgrade)).
    pub fn new(stream: IoStream, role: Role) -> Self {
        Self {
            stream: Arc::new(Mutex::new(stream)),
            role,
            parser: Mutex::new(Parser::new()),
            open: Mutex::new(true),
            permessage_deflate: Mutex::new(false),
            outbound: Mutex::new(Vec::with_capacity(4096)),
            fragment: Mutex::new(FragmentState {
                active: false,
                buffer: Vec::new(),
                opcode: Opcode::Text,
                compressed: false,
            }),
            last_close: Mutex::new(None),
            read_state: Mutex::new(ReadState {
                buf: Vec::with_capacity(READ_CHUNK),
                pos: 0,
                scratch: vec![0u8; READ_CHUNK],
            }),
        }
    }

    /// Enable or disable RFC 7692 permessage-deflate for this session.
    pub fn set_permessage_deflate(&self, enabled: bool) {
        if let Ok(mut guard) = self.permessage_deflate.try_lock() {
            *guard = enabled;
        }
    }

    /// Return true when permessage-deflate was negotiated.
    pub async fn permessage_deflate(&self) -> bool {
        *self.permessage_deflate.lock().await
    }

    /// Close code and reason from the peer's close frame, if received.
    pub async fn last_close(&self) -> Option<(u16, String)> {
        self.last_close.lock().await.clone()
    }

    /// Return true when the stream is TLS-wrapped.
    pub async fn is_tls_connected(&self) -> bool {
        self.stream.lock().await.is_tls()
    }

    /// Write a pre-encoded frame to the wire (low-level; prefer `send_*`).
    pub async fn send_raw(&self, frame: &[u8]) -> Result<()> {
        let mut stream = self.stream.lock().await;
        stream.write_all(frame).await.map_err(Error::from)?;
        stream.flush().await.map_err(Error::from)?;
        Ok(())
    }

    async fn send_frame_into(&self, build: impl FnOnce(&mut Vec<u8>)) -> Result<()> {
        let mut out = self.outbound.lock().await;
        build(&mut out);
        self.send_raw(&out).await
    }

    async fn send_data_frame(&self, opcode: Opcode, data: &[u8], fin: bool) -> Result<()> {
        let deflate = self.permessage_deflate().await;
        let mask = self.role == Role::Client;

        let (payload, rsv1): (Cow<[u8]>, bool) = if cfg!(feature = "deflate")
            && deflate
            && fin
            && matches!(opcode, Opcode::Text | Opcode::Binary)
        {
            (Cow::Owned(compress_message(data)?), true)
        } else {
            (Cow::Borrowed(data), false)
        };

        self.send_frame_into(|out| {
            FrameBuilder::build_into_opcode(out, opcode, &payload, fin, mask, rsv1);
        })
        .await
    }

    /// Send a text frame or fragment.
    pub async fn send_text(&self, text: &str, fin: bool) -> Result<()> {
        self.send_data_frame(Opcode::Text, text.as_bytes(), fin)
            .await
    }

    /// Send a binary frame or fragment.
    pub async fn send_binary(&self, data: &[u8], fin: bool) -> Result<()> {
        self.send_data_frame(Opcode::Binary, data, fin).await
    }

    /// Send a continuation fragment.
    pub async fn send_continuation(&self, data: &[u8], fin: bool) -> Result<()> {
        let mask = self.role == Role::Client;
        self.send_frame_into(|out| {
            FrameBuilder::build_into_opcode(out, Opcode::Continuation, data, fin, mask, false);
        })
        .await
    }

    /// Send a ping control frame.
    pub async fn send_ping(&self, data: &[u8]) -> Result<()> {
        let mask = self.role == Role::Client;
        self.send_frame_into(|out| {
            FrameBuilder::build_into_opcode(out, Opcode::Ping, data, true, mask, false);
        })
        .await
    }

    /// Send a pong control frame.
    pub async fn send_pong(&self, data: &[u8]) -> Result<()> {
        let mask = self.role == Role::Client;
        self.send_frame_into(|out| {
            FrameBuilder::build_into_opcode(out, Opcode::Pong, data, true, mask, false);
        })
        .await
    }

    /// Initiate close with the given RFC 6455 status code and reason.
    pub async fn close(&self, code: u16, reason: &str) -> Result<()> {
        let mask = self.role == Role::Client;
        let code = sanitize_close_code(code);
        self.send_frame_into(|out| {
            FrameBuilder::build_close_into(out, code, reason, mask);
        })
        .await?;
        *self.open.lock().await = false;
        Ok(())
    }

    /// Return true until a close frame is sent or the stream ends.
    pub async fn is_open(&self) -> bool {
        *self.open.lock().await
    }

    async fn fail_protocol<T>(&self, reason: &str) -> Result<T> {
        let _ = self.close(1002, reason).await;
        Err(Error::Protocol(reason.into()))
    }

    fn validate_incoming(
        header: &crate::frame::FrameHeader,
        role: Role,
        deflate: bool,
    ) -> Result<()> {
        let data_opcode = matches!(
            header.opcode,
            Opcode::Text | Opcode::Binary | Opcode::Continuation
        );

        if header.rsv1 {
            if !deflate || !data_opcode {
                return Err(Error::Protocol(
                    "RSV1 set without permessage-deflate".into(),
                ));
            }
            if header.opcode == Opcode::Continuation {
                return Err(Error::Protocol("RSV1 on continuation frame".into()));
            }
        }
        if header.rsv2 || header.rsv3 || (!deflate && header.rsv1) {
            return Err(Error::Protocol(
                "RSV bits set without extension negotiation".into(),
            ));
        }
        if role == Role::Server && !header.mask {
            return Err(Error::Protocol("client frame must be masked".into()));
        }
        if role == Role::Client && header.mask {
            return Err(Error::Protocol("server frame must not be masked".into()));
        }
        Ok(())
    }

    /// Read one complete WebSocket frame from the stream.
    pub async fn read_frame(&self) -> Result<Option<Frame>> {
        if !self.is_open().await {
            return Ok(None);
        }

        let mut rs = self.read_state.lock().await;
        let mut parser = self.parser.lock().await;

        loop {
            let (result, consumed) = parser.parse(&rs.buf[rs.pos..]);
            rs.pos += consumed;

            match result {
                ParseResult::Complete => {
                    let frame = parser.take_frame();
                    if rs.pos == rs.buf.len() {
                        rs.buf.clear();
                        rs.pos = 0;
                    }
                    let deflate = self.permessage_deflate().await;
                    drop(parser);
                    drop(rs);
                    Self::validate_incoming(&frame.header, self.role, deflate)?;
                    return Ok(Some(frame));
                }
                ParseResult::Error => {
                    drop(parser);
                    drop(rs);
                    return self.fail_protocol("invalid frame").await;
                }
                ParseResult::Incomplete => {
                    let pos = rs.pos;
                    if pos > 0 {
                        rs.buf.drain(..pos);
                        rs.pos = 0;
                    }
                    let n = {
                        let mut stream = self.stream.lock().await;
                        stream.read(&mut rs.scratch).await.map_err(Error::from)?
                    };
                    if n == 0 {
                        drop(parser);
                        drop(rs);
                        *self.open.lock().await = false;
                        return Ok(None);
                    }
                    let st = &mut *rs;
                    st.buf.extend_from_slice(&st.scratch[..n]);
                }
            }
        }
    }

    async fn deliver_message(
        &self,
        opcode: Opcode,
        payload: Vec<u8>,
        compressed: bool,
    ) -> Result<Message> {
        let deflate = self.permessage_deflate().await;
        let mut plain = payload;
        if compressed {
            if !deflate || !cfg!(feature = "deflate") {
                return self
                    .fail_protocol("Compressed frame without negotiated extension")
                    .await;
            }
            plain = decompress_message(&plain)?;
        }
        if opcode == Opcode::Text && !utf8::is_valid_utf8(&plain) {
            self.close(1007, "Invalid UTF-8 in text frame").await?;
            return Err(Error::Protocol("invalid UTF-8 in text frame".into()));
        }
        Ok(Message {
            opcode,
            payload: plain,
        })
    }

    async fn handle_close_frame(&self, payload: &[u8]) -> Result<()> {
        let (code, reason) = parse_close_payload(payload);
        *self.last_close.lock().await = Some((code, reason.clone()));
        let mask = self.role == Role::Client;
        self.send_frame_into(|out| {
            FrameBuilder::build_close_into(out, code, "", mask);
        })
        .await?;
        *self.open.lock().await = false;
        Ok(())
    }

    /// Read one reassembled message (handles fragmentation, ping/pong, close).
    pub async fn read_message(&self) -> Result<Option<Message>> {
        loop {
            let Some(frame) = self.read_frame().await? else {
                return Ok(None);
            };

            let opcode = frame.header.opcode;
            let fin = frame.header.fin;
            let compressed = frame.header.rsv1;
            let payload = frame.payload;

            match opcode {
                Opcode::Close => {
                    self.handle_close_frame(&payload).await?;
                    return Ok(None);
                }
                Opcode::Ping => {
                    self.send_pong(&payload).await?;
                    continue;
                }
                Opcode::Pong => continue,
                Opcode::Continuation => {
                    let mut frag = self.fragment.lock().await;
                    if !frag.active {
                        return self.fail_protocol("Unexpected continuation frame").await;
                    }
                    frag.buffer.extend_from_slice(&payload);
                    if fin {
                        let opcode = frag.opcode;
                        let compressed = frag.compressed;
                        let buffer = std::mem::take(&mut frag.buffer);
                        frag.reset();
                        drop(frag);
                        let msg = self.deliver_message(opcode, buffer, compressed).await?;
                        return Ok(Some(msg));
                    }
                }
                Opcode::Text | Opcode::Binary => {
                    let mut frag = self.fragment.lock().await;
                    if frag.active {
                        return self
                            .fail_protocol("New data frame before fragmented message completed")
                            .await;
                    }
                    if !fin {
                        frag.active = true;
                        frag.opcode = opcode;
                        frag.buffer = payload;
                        frag.compressed = compressed;
                        continue;
                    }
                    return Ok(Some(
                        self.deliver_message(opcode, payload, compressed).await?,
                    ));
                }
            }
        }
    }

    /// Legacy helper: handle a single frame opcode (prefer `read_message`).
    pub async fn handle_incoming(&self, opcode: Opcode, payload: &[u8]) -> Result<bool> {
        match opcode {
            Opcode::Ping => {
                self.send_pong(payload).await?;
                Ok(true)
            }
            Opcode::Pong => Ok(true),
            Opcode::Close => {
                self.handle_close_frame(payload).await?;
                Ok(false)
            }
            Opcode::Text if !utf8::is_valid_utf8(payload) => {
                self.close(1007, "Invalid UTF-8 in text frame").await?;
                Ok(false)
            }
            _ => Ok(true),
        }
    }
}
