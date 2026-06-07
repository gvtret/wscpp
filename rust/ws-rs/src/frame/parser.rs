use super::mask::copy_and_unmask;

/// RFC 6455 frame opcode (§5.2).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Opcode {
    /// Continuation of a fragmented message.
    Continuation = 0x0,
    /// UTF-8 text payload.
    Text = 0x1,
    /// Binary payload.
    Binary = 0x2,
    /// Connection close.
    Close = 0x8,
    /// Keepalive ping.
    Ping = 0x9,
    /// Ping response.
    Pong = 0xA,
}

impl Opcode {
    fn from_u8(v: u8) -> Option<Self> {
        match v {
            0x0 => Some(Self::Continuation),
            0x1 => Some(Self::Text),
            0x2 => Some(Self::Binary),
            0x8 => Some(Self::Close),
            0x9 => Some(Self::Ping),
            0xA => Some(Self::Pong),
            _ => None,
        }
    }

    /// Returns true for close, ping, and pong opcodes.
    pub fn is_control(self) -> bool {
        matches!(self, Self::Close | Self::Ping | Self::Pong)
    }
}

/// Decoded RFC 6455 frame header fields.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FrameHeader {
    /// FIN bit — true when this is the last fragment of a message.
    pub fin: bool,
    /// RSV1 — set when permessage-deflate marks a compressed data frame.
    pub rsv1: bool,
    /// RSV2 — reserved; must be false unless negotiated.
    pub rsv2: bool,
    /// RSV3 — reserved; must be false unless negotiated.
    pub rsv3: bool,
    /// Frame opcode.
    pub opcode: Opcode,
    /// MASK bit — true when payload is XOR-masked (client frames only).
    pub mask: bool,
    /// Payload length in bytes.
    pub payload_len: u64,
    /// Masking key (meaningful only when `mask` is true).
    pub masking_key: [u8; 4],
}

impl Default for FrameHeader {
    fn default() -> Self {
        Self {
            fin: false,
            rsv1: false,
            rsv2: false,
            rsv3: false,
            opcode: Opcode::Text,
            mask: false,
            payload_len: 0,
            masking_key: [0; 4],
        }
    }
}

/// A complete parsed WebSocket frame (header + unmasked payload).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Frame {
    /// Decoded header.
    pub header: FrameHeader,
    /// Unmasked application payload.
    pub payload: Vec<u8>,
}

/// Result of feeding bytes into [`Parser::parse`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ParseResult {
    /// Need more bytes to finish the current frame.
    Incomplete,
    /// One full frame is ready — call [`Parser::frame`].
    Complete,
    /// Invalid framing (opcode, length, etc.).
    Error,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum State {
    Header,
    PayloadLengthExt,
    MaskingKey,
    Payload,
}

/// Incremental RFC 6455 frame parser (feed bytes from the stream).
pub struct Parser {
    state: State,
    header: FrameHeader,
    payload_remaining: u64,
    buffer: Vec<u8>,
}

impl Default for Parser {
    fn default() -> Self {
        Self::new()
    }
}

impl Parser {
    /// Create a parser ready for the next frame.
    pub fn new() -> Self {
        Self {
            state: State::Header,
            header: FrameHeader::default(),
            payload_remaining: 0,
            buffer: Vec::new(),
        }
    }

    /// Discard partial state and start parsing a new frame.
    pub fn reset(&mut self) {
        *self = Self::new();
    }

    /// Consume bytes from `data`; returns `(result, bytes_consumed)`.
    pub fn parse(&mut self, data: &[u8]) -> (ParseResult, usize) {
        let mut consumed = 0usize;

        while consumed < data.len() {
            let prev = consumed;
            let result = match self.state {
                State::Header => self.parse_header(&data[consumed..], &mut consumed),
                State::PayloadLengthExt => {
                    self.parse_payload_length(&data[consumed..], &mut consumed)
                }
                State::MaskingKey => self.parse_masking_key(&data[consumed..], &mut consumed),
                State::Payload => self.parse_payload(&data[consumed..], &mut consumed),
            };

            match result {
                ParseResult::Error => return (ParseResult::Error, consumed),
                ParseResult::Complete => return (ParseResult::Complete, consumed),
                ParseResult::Incomplete => {}
            }

            if consumed == prev {
                break;
            }
        }

        (ParseResult::Incomplete, consumed)
    }

    /// Return the frame completed by the last `Complete` parse result.
    pub fn frame(&self) -> Frame {
        Frame {
            header: self.header.clone(),
            payload: self.buffer.clone(),
        }
    }

    fn parse_header(&mut self, data: &[u8], consumed: &mut usize) -> ParseResult {
        if data.len() < 2 {
            return ParseResult::Incomplete;
        }

        let first = data[0];
        let second = data[1];

        self.header.fin = (first & 0x80) != 0;
        self.header.rsv1 = (first & 0x40) != 0;
        self.header.rsv2 = (first & 0x20) != 0;
        self.header.rsv3 = (first & 0x10) != 0;
        self.header.opcode = match Opcode::from_u8(first & 0x0F) {
            Some(op) => op,
            None => return ParseResult::Error,
        };
        self.header.mask = (second & 0x80) != 0;
        self.header.payload_len = (second & 0x7F) as u64;

        *consumed += 2;

        if self.header.payload_len <= 125 {
            self.state = if self.header.mask {
                State::MaskingKey
            } else {
                State::Payload
            };
            self.payload_remaining = self.header.payload_len;
            self.buffer.clear();
            if self.header.payload_len > 0 {
                self.buffer.reserve(self.header.payload_len as usize);
            }
        } else {
            self.state = State::PayloadLengthExt;
            self.payload_remaining = 0;
        }

        ParseResult::Incomplete
    }

    fn parse_payload_length(&mut self, data: &[u8], consumed: &mut usize) -> ParseResult {
        let len_byte = (self.header.payload_len & 0x7F) as u8;
        if len_byte == 126 {
            if data.len() < 2 {
                return ParseResult::Incomplete;
            }
            self.header.payload_len = (u64::from(data[0]) << 8) | u64::from(data[1]);
            *consumed += 2;
        } else {
            if data.len() < 8 {
                return ParseResult::Incomplete;
            }
            self.header.payload_len = 0;
            for &b in &data[..8] {
                self.header.payload_len = (self.header.payload_len << 8) | u64::from(b);
            }
            *consumed += 8;
        }

        if self.header.payload_len > 0x7FFF_FFFF_FFFF_FFFF {
            return ParseResult::Error;
        }

        self.state = if self.header.mask {
            State::MaskingKey
        } else {
            State::Payload
        };
        self.payload_remaining = self.header.payload_len;
        self.buffer.clear();
        if self.header.payload_len > 0 {
            self.buffer.reserve(self.header.payload_len as usize);
        }

        ParseResult::Incomplete
    }

    fn parse_masking_key(&mut self, data: &[u8], consumed: &mut usize) -> ParseResult {
        if data.len() < 4 {
            return ParseResult::Incomplete;
        }
        self.header.masking_key.copy_from_slice(&data[..4]);
        *consumed += 4;
        self.state = State::Payload;
        self.payload_remaining = self.header.payload_len;
        self.buffer.reserve(self.header.payload_len as usize);
        ParseResult::Incomplete
    }

    fn parse_payload(&mut self, data: &[u8], consumed: &mut usize) -> ParseResult {
        if self.payload_remaining == 0 {
            self.state = State::Header;
            return ParseResult::Complete;
        }

        let to_read = std::cmp::min(self.payload_remaining as usize, data.len());
        let offset = self.buffer.len();
        self.buffer.resize(offset + to_read, 0);

        if self.header.mask {
            copy_and_unmask(
                &mut self.buffer[offset..],
                &data[..to_read],
                &self.header.masking_key,
                offset,
            );
        } else {
            self.buffer[offset..offset + to_read].copy_from_slice(&data[..to_read]);
        }

        *consumed += to_read;
        self.payload_remaining -= to_read as u64;

        if self.payload_remaining == 0 {
            self.state = State::Header;
            ParseResult::Complete
        } else {
            ParseResult::Incomplete
        }
    }
}
