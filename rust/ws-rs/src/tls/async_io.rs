use rustls::pki_types::ServerName;
use tokio::net::TcpStream;
use tokio_rustls::client::TlsStream as ClientTlsStream;
use tokio_rustls::server::TlsStream as ServerTlsStream;
use tokio_rustls::{TlsAcceptor, TlsConnector};

use super::{ClientTlsConfig, ServerTlsConfig};
use crate::error::{Error, Result};

/// Perform a client TLS handshake over `stream` (async, tokio-rustls).
pub async fn connect_tls(
    stream: TcpStream,
    server_name: &str,
    config: &ClientTlsConfig,
) -> Result<ClientTlsStream<TcpStream>> {
    let name = ServerName::try_from(server_name)
        .map_err(|_| Error::Tls(format!("invalid SNI hostname: {server_name}")))?
        .to_owned();
    let connector = TlsConnector::from(config.config.clone());
    connector
        .connect(name, stream)
        .await
        .map_err(|e| Error::Tls(e.to_string()))
}

/// Perform a server TLS handshake over `stream` (async, tokio-rustls).
pub async fn accept_tls(
    stream: TcpStream,
    config: &ServerTlsConfig,
) -> Result<ServerTlsStream<TcpStream>> {
    let acceptor = TlsAcceptor::from(config.config.clone());
    acceptor
        .accept(stream)
        .await
        .map_err(|e| Error::Tls(e.to_string()))
}
