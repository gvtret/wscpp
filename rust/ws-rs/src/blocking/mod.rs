//! Blocking WebSocket stack using `std::net` only (no tokio).
//!
//! Mirrors C++ `WSCPP_USE_ASIO=OFF` / `linux_socket` transport. Same RFC behaviour as the
//! async stack; methods are synchronous and block the calling thread.
//!
//! ```no_run
//! use ws_rs::blocking::Client;
//!
//! fn main() -> ws_rs::Result<()> {
//!     let mut client = Client::new();
//!     client.connect("ws://127.0.0.1:8080/")?;
//!     client.send_text("hello")?;
//!     let _ = client.recv_text()?;
//!     client.close()?;
//!     Ok(())
//! }
//! ```

/// Sync WebSocket client.
pub mod client;
/// Established sync WebSocket session.
pub mod connection;
/// Sync HTTP → WebSocket upgrade.
pub mod handshake;
/// Sync WebSocket server.
pub mod server;
/// Sync TCP/TLS stream wrapper.
pub mod transport;

#[cfg(feature = "blocking-tls")]
/// Blocking TLS handshake (rustls, no tokio).
pub mod tls;

pub use client::Client;
pub use connection::Connection;
pub use server::Server;
