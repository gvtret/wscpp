mod config;

#[cfg(feature = "tls")]
mod async_io;

pub use config::{ClientTlsConfig, ServerTlsConfig};

#[cfg(feature = "tls")]
pub use async_io::{accept_tls, connect_tls};
