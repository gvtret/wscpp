use super::mask::{apply_mask, generate_masking_key};
use super::Opcode;

/// Largest possible frame header (2 base + 8 extended length + 4 masking key).
const MAX_HEADER_LEN: usize = 14;

/// Write the RFC 6455 frame header (everything before the masking key/payload) into `out`.
#[inline]
fn push_header(out: &mut Vec<u8>, opcode: Opcode, fin: bool, rsv1: bool, mask: bool, len: usize) {
    let mut first = (opcode as u8) & 0x0F;
    if fin {
        first |= 0x80;
    }
    if rsv1 {
        first |= 0x40;
    }
    out.push(first);

    let mask_bit: u8 = if mask { 0x80 } else { 0 };
    if len < 126 {
        out.push(mask_bit | (len as u8));
    } else if len <= 0xFFFF {
        out.push(mask_bit | 126);
        out.push((len >> 8) as u8);
        out.push((len & 0xFF) as u8);
    } else {
        out.push(mask_bit | 127);
        let len64 = len as u64;
        for i in (0..8).rev() {
            out.push((len64 >> (i * 8)) as u8);
        }
    }
}

/// Encode `payload` as a frame into `out` with a single payload copy (masking is applied
/// in place inside `out`, avoiding an intermediate owned buffer). Reuses `out` capacity.
#[inline]
fn encode_into(
    out: &mut Vec<u8>,
    opcode: Opcode,
    payload: &[u8],
    fin: bool,
    mask: bool,
    rsv1: bool,
) {
    out.clear();
    let cap = MAX_HEADER_LEN + payload.len();
    if out.capacity() < cap {
        out.reserve(cap - out.capacity());
    }

    push_header(out, opcode, fin, rsv1, mask, payload.len());

    if mask {
        let key = generate_masking_key();
        out.extend_from_slice(&key);
        let start = out.len();
        out.extend_from_slice(payload);
        apply_mask(&mut out[start..], &key);
    } else {
        out.extend_from_slice(payload);
    }
}

/// Incremental RFC 6455 frame encoder.
pub struct FrameBuilder {
    fin: bool,
    rsv1: bool,
    rsv2: bool,
    rsv3: bool,
    opcode: Opcode,
    mask: bool,
    payload: Vec<u8>,
    masking_key: [u8; 4],
}

impl FrameBuilder {
    /// Default builder: FIN text frame, unmasked, empty payload.
    pub fn new() -> Self {
        Self {
            fin: true,
            rsv1: false,
            rsv2: false,
            rsv3: false,
            opcode: Opcode::Text,
            mask: false,
            payload: Vec::new(),
            masking_key: [0; 4],
        }
    }

    /// Set FIN bit (true = final fragment of a message).
    pub fn fin(mut self, fin: bool) -> Self {
        self.fin = fin;
        self
    }

    /// Set RSV1 (permessage-deflate uses this on compressed data frames).
    pub fn rsv1(mut self, rsv1: bool) -> Self {
        self.rsv1 = rsv1;
        self
    }

    /// Set RSV2 (must be false unless an extension negotiates it).
    pub fn rsv2(mut self, rsv2: bool) -> Self {
        self.rsv2 = rsv2;
        self
    }

    /// Set RSV3 (must be false unless an extension negotiates it).
    pub fn rsv3(mut self, rsv3: bool) -> Self {
        self.rsv3 = rsv3;
        self
    }

    /// Set frame opcode.
    pub fn opcode(mut self, opcode: Opcode) -> Self {
        self.opcode = opcode;
        self
    }

    /// Mask outgoing payload (required for client → server frames).
    pub fn mask(mut self, mask: bool) -> Self {
        self.mask = mask;
        self
    }

    /// Set payload bytes (copied into the frame).
    pub fn payload(mut self, payload: impl Into<Vec<u8>>) -> Self {
        self.payload = payload.into();
        self
    }

    /// Build frame into `out`, reusing `out` capacity (mirrors C++ `build_into`).
    pub fn build_into(mut self, out: &mut Vec<u8>) {
        if self.mask {
            self.masking_key = generate_masking_key();
            apply_mask(&mut self.payload, &self.masking_key);
        }

        out.clear();
        let cap = 2 + self.payload.len() + if self.mask { 4 } else { 0 };
        if out.capacity() < cap {
            out.reserve(cap - out.capacity());
        }

        let mut first = (self.opcode as u8) & 0x0F;
        if self.fin {
            first |= 0x80;
        }
        if self.rsv1 {
            first |= 0x40;
        }
        if self.rsv2 {
            first |= 0x20;
        }
        if self.rsv3 {
            first |= 0x10;
        }
        out.push(first);

        let len = self.payload.len();
        if len < 126 {
            out.push((if self.mask { 0x80 } else { 0 }) | (len as u8));
        } else if len <= 0xFFFF {
            out.push((if self.mask { 0x80 } else { 0 }) | 126);
            out.push((len >> 8) as u8);
            out.push((len & 0xFF) as u8);
        } else {
            out.push((if self.mask { 0x80 } else { 0 }) | 127);
            let len64 = len as u64;
            for i in (0..8).rev() {
                out.push((len64 >> (i * 8)) as u8);
            }
        }

        if self.mask {
            out.extend_from_slice(&self.masking_key);
        }
        out.extend_from_slice(&self.payload);
    }

    /// Encode the frame into a new `Vec<u8>`.
    pub fn build(self) -> Vec<u8> {
        let mut out = Vec::new();
        self.build_into(&mut out);
        out
    }
}

impl Default for FrameBuilder {
    fn default() -> Self {
        Self::new()
    }
}

impl FrameBuilder {
    /// Build a text frame.
    pub fn build_text(text: &str, fin: bool, mask: bool) -> Vec<u8> {
        Self::build_opcode(Opcode::Text, text.as_bytes(), fin, mask)
    }

    /// Build a binary frame.
    pub fn build_binary(data: &[u8], fin: bool, mask: bool) -> Vec<u8> {
        Self::build_opcode(Opcode::Binary, data, fin, mask)
    }

    /// Build a close frame with status code and optional reason.
    pub fn build_close(status_code: u16, reason: &str, mask: bool) -> Vec<u8> {
        let mut out = Vec::new();
        Self::build_close_into(&mut out, status_code, reason, mask);
        out
    }

    /// Build a ping frame.
    pub fn build_ping(data: &[u8], mask: bool) -> Vec<u8> {
        Self::build_opcode(Opcode::Ping, data, true, mask)
    }

    /// Build a pong frame.
    pub fn build_pong(data: &[u8], mask: bool) -> Vec<u8> {
        Self::build_opcode(Opcode::Pong, data, true, mask)
    }

    /// Build a frame with arbitrary opcode and payload.
    pub fn build_opcode(opcode: Opcode, data: &[u8], fin: bool, mask: bool) -> Vec<u8> {
        let mut out = Vec::with_capacity(MAX_HEADER_LEN + data.len());
        encode_into(&mut out, opcode, data, fin, mask, false);
        out
    }

    /// Encode a frame into `out`, reusing buffer capacity (preferred for hot paths).
    pub fn build_into_opcode(
        out: &mut Vec<u8>,
        opcode: Opcode,
        data: &[u8],
        fin: bool,
        mask: bool,
        rsv1: bool,
    ) {
        encode_into(out, opcode, data, fin, mask, rsv1);
    }

    /// Encode a close frame into `out`, reusing buffer capacity.
    pub fn build_close_into(out: &mut Vec<u8>, status_code: u16, reason: &str, mask: bool) {
        out.clear();
        let payload_len = 2 + reason.len();
        let cap = MAX_HEADER_LEN + payload_len;
        if out.capacity() < cap {
            out.reserve(cap - out.capacity());
        }

        push_header(out, Opcode::Close, true, false, mask, payload_len);

        let key = if mask {
            let key = generate_masking_key();
            out.extend_from_slice(&key);
            Some(key)
        } else {
            None
        };

        let start = out.len();
        out.push((status_code >> 8) as u8);
        out.push((status_code & 0xFF) as u8);
        out.extend_from_slice(reason.as_bytes());

        if let Some(key) = key {
            apply_mask(&mut out[start..], &key);
        }
    }
}
