//! RFC 6455 frame parsing and building (§5).
//!
//! Low-level API; most applications use [`crate::connection::Connection`] instead.

mod builder;
mod mask;
mod parser;

/// Incremental frame encoder (supports [`build_into`](FrameBuilder::build_into) reuse).
pub use builder::FrameBuilder;
/// XOR mask/unmask helper (RFC 6455 §5.3).
pub use mask::apply_mask;
/// Parsed WebSocket frame.
pub use parser::Frame;
/// Decoded frame header fields.
pub use parser::FrameHeader;
/// Frame opcode (RFC 6455 §5.2).
pub use parser::Opcode;
/// Result of feeding bytes into [`Parser`].
pub use parser::ParseResult;
/// Incremental RFC 6455 frame parser.
pub use parser::Parser;
