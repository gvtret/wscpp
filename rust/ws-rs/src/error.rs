//! Error types for **ws-rs** I/O and protocol handling.

use std::io;

use thiserror::Error;

/// WebSocket stack error (protocol, handshake, TLS, I/O).
#[derive(Debug, Error)]
pub enum Error {
    /// RFC 6455 protocol violation or local policy (e.g. invalid UTF-8).
    #[error("protocol error: {0}")]
    Protocol(String),

    /// HTTP upgrade handshake failure.
    #[error("handshake error: {0}")]
    Handshake(String),

    /// Connection already closed or peer closed without close frame.
    #[error("connection closed")]
    Closed,

    /// Malformed or unsupported `ws://` / `wss://` URL.
    #[error("invalid URL: {0}")]
    InvalidUrl(String),

    /// rustls or certificate loading failure.
    #[error("TLS error: {0}")]
    Tls(String),

    /// Underlying TCP/TLS I/O error.
    #[error("I/O error")]
    Io(#[from] io::Error),
}

/// Convenience result type used across **ws-rs**.
pub type Result<T> = std::result::Result<T, Error>;
