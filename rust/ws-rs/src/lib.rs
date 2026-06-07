//! **ws-rs** — Rust WebSocket stack with the same layering as [wscpp](https://github.com/gvtret/wscpp):
//! frame → handshake → connection → client|server.
//!
//! # Quick start (async)
//!
//! ```no_run
//! use ws_rs::client::Client;
//!
//! #[tokio::main]
//! async fn main() -> ws_rs::Result<()> {
//!     let mut client = Client::new();
//!     client.connect("ws://127.0.0.1:8080/").await?;
//!     client.send_text("hello").await?;
//!     let reply = client.recv_text().await?.unwrap_or_default();
//!     client.close().await?;
//!     Ok(())
//! }
//! ```
//!
//! # Sync stack
//!
//! Enable feature `std-blocking` (default on) and use [`blocking::Client`]
//! for a tokio-free code path — mirrors C++ `WSCPP_USE_ASIO=OFF`.
//!
//! See the [developer guide](https://github.com/gvtret/wscpp/blob/master/docs/RUST.md).
//!
//! ## Features
//!
//! | Feature | Default | Description |
//! |---------|---------|-------------|
//! | `async` | yes | Tokio-based client/server (`ws_rs::client`, `ws_rs::server`) |
//! | `tls` | yes | `wss://` via rustls (requires `async`) |
//! | `deflate` | yes | RFC 7692 permessage-deflate |
//! | `std-blocking` | yes | Sync stack via `std::net` (`ws_rs::blocking`, mirrors C++ `linux_socket`) |
//! | `blocking-tls` | yes | Sync `wss://` for `ws_rs::blocking` (rustls, no tokio) |
//! | `stress` | no | Heavy integration tests (`cargo test -p ws-rs --features stress`) |
//!
//! Minimal-deps build (no tokio): `cargo build --no-default-features --features "std-blocking,deflate,blocking-tls"`.

#![warn(missing_docs)]
#![warn(rustdoc::broken_intra_doc_links)]

pub mod close;
pub mod error;
pub mod frame;
pub mod handshake;
pub mod types;
/// UTF-8 validation for text frames (RFC 6455 §8.1).
pub mod utf8;
pub mod version;

pub mod extensions;

#[cfg(feature = "async")]
/// Async WebSocket client (`ws://` / `wss://`).
pub mod client;
#[cfg(feature = "async")]
/// Established async WebSocket session.
pub mod connection;
#[cfg(feature = "async")]
/// Async WebSocket server.
pub mod server;
#[cfg(any(feature = "tls", feature = "blocking-tls"))]
/// TLS configuration and handshake helpers (rustls).
pub mod tls;
#[cfg(feature = "async")]
/// Async TCP/TLS stream wrapper for WebSocket I/O.
pub mod transport;

#[cfg(feature = "std-blocking")]
pub mod blocking;

#[cfg(any(feature = "tls", feature = "blocking-tls"))]
pub use tls::{ClientTlsConfig, ServerTlsConfig};

pub use error::{Error, Result};
pub use types::{Message, Role};
