//! Shared connection types.

use crate::frame::Opcode;

/// Endpoint role after the HTTP upgrade (client frames are masked).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Role {
    /// Outbound data frames are masked (RFC 6455 §5.3).
    Client,
    /// Outbound data frames are unmasked.
    Server,
}

/// Reassembled application message (RFC 6455 data frame, possibly from fragments).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Message {
    /// Original opcode of the first frame (`Text` or `Binary`).
    pub opcode: Opcode,
    /// Decompressed, UTF-8-validated (for `Text`) payload bytes.
    pub payload: Vec<u8>,
}
