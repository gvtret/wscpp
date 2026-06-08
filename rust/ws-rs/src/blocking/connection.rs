use std::borrow::Cow;
use std::sync::{Arc, Mutex};

use crate::blocking::transport::StdStream;
use crate::close::{parse_close_payload, sanitize_close_code};
use crate::error::{Error, Result};
#[cfg(feature = "deflate")]
use crate::extensions::permessage_deflate::{compress_message, decompress_message};
use crate::frame::{Frame, FrameBuilder, Opcode, ParseResult, Parser};
use crate::types::{Message, Role};
use crate::utf8;

/// Socket read chunk size; larger reads reduce syscalls for big payloads.
const READ_CHUNK: usize = 64 * 1024;

/// Persistent inbound buffer so `read_frame` neither reallocates per call nor
/// loses bytes pipelined after a completed frame.
struct ReadState {
    buf: Vec<u8>,
    pos: usize,
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

/// Established synchronous WebSocket session over `std::net` / blocking TLS.
pub struct Connection {
    stream: Arc<Mutex<StdStream>>,
    role: Role,
    parser: Mutex<Parser>,
    open: Mutex<bool>,
    permessage_deflate: Mutex<bool>,
    outbound: Mutex<Vec<u8>>,
    fragment: Mutex<FragmentState>,
    last_close: Mutex<Option<(u16, String)>>,
    read_state: Mutex<ReadState>,
}

impl Connection {
    /// Wrap an upgraded stream (call after [`client_upgrade`](crate::blocking::handshake::client_upgrade)
    /// or [`server_upgrade`](crate::blocking::handshake::server_upgrade)).
    pub fn new(stream: StdStream, role: Role) -> Self {
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
        if let Ok(mut guard) = self.permessage_deflate.lock() {
            *guard = enabled;
        }
    }

    /// Return true when permessage-deflate was negotiated.
    pub fn permessage_deflate(&self) -> bool {
        *self.permessage_deflate.lock().unwrap()
    }

    /// Close code and reason from the peer's close frame, if received.
    pub fn last_close(&self) -> Option<(u16, String)> {
        self.last_close.lock().unwrap().clone()
    }

    /// Return true when the stream is TLS-wrapped.
    pub fn is_tls_connected(&self) -> bool {
        self.stream.lock().unwrap().is_tls()
    }

    fn send_raw(&self, frame: &[u8]) -> Result<()> {
        let mut stream = self.stream.lock().unwrap();
        stream.write_all(frame).map_err(Error::from)?;
        stream.flush().map_err(Error::from)?;
        Ok(())
    }

    fn send_frame_into(&self, build: impl FnOnce(&mut Vec<u8>)) -> Result<()> {
        let mut out = self.outbound.lock().unwrap();
        build(&mut out);
        self.send_raw(&out)
    }

    fn send_data_frame(&self, opcode: Opcode, data: &[u8], fin: bool) -> Result<()> {
        let deflate = self.permessage_deflate();
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
    }

    /// Send a text frame or fragment.
    pub fn send_text(&self, text: &str, fin: bool) -> Result<()> {
        self.send_data_frame(Opcode::Text, text.as_bytes(), fin)
    }

    /// Send a binary frame or fragment.
    pub fn send_binary(&self, data: &[u8], fin: bool) -> Result<()> {
        self.send_data_frame(Opcode::Binary, data, fin)
    }

    /// Send a continuation fragment.
    pub fn send_continuation(&self, data: &[u8], fin: bool) -> Result<()> {
        let mask = self.role == Role::Client;
        self.send_frame_into(|out| {
            FrameBuilder::build_into_opcode(out, Opcode::Continuation, data, fin, mask, false);
        })
    }

    /// Send a ping control frame.
    pub fn send_ping(&self, data: &[u8]) -> Result<()> {
        let mask = self.role == Role::Client;
        self.send_frame_into(|out| {
            FrameBuilder::build_into_opcode(out, Opcode::Ping, data, true, mask, false);
        })
    }

    /// Send a pong control frame.
    pub fn send_pong(&self, data: &[u8]) -> Result<()> {
        let mask = self.role == Role::Client;
        self.send_frame_into(|out| {
            FrameBuilder::build_into_opcode(out, Opcode::Pong, data, true, mask, false);
        })
    }

    /// Initiate close with the given RFC 6455 status code and reason.
    pub fn close(&self, code: u16, reason: &str) -> Result<()> {
        let mask = self.role == Role::Client;
        let code = sanitize_close_code(code);
        self.send_frame_into(|out| {
            FrameBuilder::build_close_into(out, code, reason, mask);
        })?;
        *self.open.lock().unwrap() = false;
        Ok(())
    }

    /// Return true until a close frame is sent or the stream ends.
    pub fn is_open(&self) -> bool {
        *self.open.lock().unwrap()
    }

    fn fail_protocol<T>(&self, reason: &str) -> Result<T> {
        let _ = self.close(1002, reason);
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
    pub fn read_frame(&self) -> Result<Option<Frame>> {
        if !self.is_open() {
            return Ok(None);
        }

        let mut rs = self.read_state.lock().unwrap();
        let mut parser = self.parser.lock().unwrap();

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
                    let deflate = self.permessage_deflate();
                    drop(parser);
                    drop(rs);
                    Self::validate_incoming(&frame.header, self.role, deflate)?;
                    return Ok(Some(frame));
                }
                ParseResult::Error => {
                    drop(parser);
                    drop(rs);
                    return self.fail_protocol("invalid frame");
                }
                ParseResult::Incomplete => {
                    let pos = rs.pos;
                    if pos > 0 {
                        rs.buf.drain(..pos);
                        rs.pos = 0;
                    }
                    let n = {
                        let mut stream = self.stream.lock().unwrap();
                        stream.read(&mut rs.scratch).map_err(Error::from)?
                    };
                    if n == 0 {
                        drop(parser);
                        drop(rs);
                        *self.open.lock().unwrap() = false;
                        return Ok(None);
                    }
                    let st = &mut *rs;
                    st.buf.extend_from_slice(&st.scratch[..n]);
                }
            }
        }
    }

    fn deliver_message(
        &self,
        opcode: Opcode,
        payload: Vec<u8>,
        compressed: bool,
    ) -> Result<Message> {
        let deflate = self.permessage_deflate();
        let mut plain = payload;
        if compressed {
            if !deflate || !cfg!(feature = "deflate") {
                return self.fail_protocol("Compressed frame without negotiated extension");
            }
            plain = decompress_message(&plain)?;
        }
        if opcode == Opcode::Text && !utf8::is_valid_utf8(&plain) {
            self.close(1007, "Invalid UTF-8 in text frame")?;
            return Err(Error::Protocol("invalid UTF-8 in text frame".into()));
        }
        Ok(Message {
            opcode,
            payload: plain,
        })
    }

    fn handle_close_frame(&self, payload: &[u8]) -> Result<()> {
        let (code, reason) = parse_close_payload(payload);
        *self.last_close.lock().unwrap() = Some((code, reason.clone()));
        let mask = self.role == Role::Client;
        self.send_frame_into(|out| {
            FrameBuilder::build_close_into(out, code, "", mask);
        })?;
        *self.open.lock().unwrap() = false;
        Ok(())
    }

    /// Read one reassembled message (handles fragmentation, ping/pong, close).
    pub fn read_message(&self) -> Result<Option<Message>> {
        loop {
            let Some(frame) = self.read_frame()? else {
                return Ok(None);
            };

            let opcode = frame.header.opcode;
            let fin = frame.header.fin;
            let compressed = frame.header.rsv1;
            let payload = frame.payload;

            match opcode {
                Opcode::Close => {
                    self.handle_close_frame(&payload)?;
                    return Ok(None);
                }
                Opcode::Ping => {
                    self.send_pong(&payload)?;
                    continue;
                }
                Opcode::Pong => continue,
                Opcode::Continuation => {
                    let mut frag = self.fragment.lock().unwrap();
                    if !frag.active {
                        return self.fail_protocol("Unexpected continuation frame");
                    }
                    frag.buffer.extend_from_slice(&payload);
                    if fin {
                        let opcode = frag.opcode;
                        let compressed = frag.compressed;
                        let buffer = std::mem::take(&mut frag.buffer);
                        frag.reset();
                        drop(frag);
                        let msg = self.deliver_message(opcode, buffer, compressed)?;
                        return Ok(Some(msg));
                    }
                }
                Opcode::Text | Opcode::Binary => {
                    let mut frag = self.fragment.lock().unwrap();
                    if frag.active {
                        return self
                            .fail_protocol("New data frame before fragmented message completed");
                    }
                    if !fin {
                        frag.active = true;
                        frag.opcode = opcode;
                        frag.buffer = payload;
                        frag.compressed = compressed;
                        continue;
                    }
                    return Ok(Some(self.deliver_message(opcode, payload, compressed)?));
                }
            }
        }
    }
}
