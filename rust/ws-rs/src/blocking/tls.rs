use std::net::TcpStream;

use rustls::pki_types::ServerName;
use rustls::{ClientConnection, ServerConnection, StreamOwned};

use crate::blocking::transport::StdStream;
use crate::error::{Error, Result};
use crate::tls::{ClientTlsConfig, ServerTlsConfig};

/// Perform a client TLS handshake over `tcp` (blocking rustls).
pub fn connect_tls(
    tcp: TcpStream,
    server_name: &str,
    config: &ClientTlsConfig,
) -> Result<StdStream> {
    let name = ServerName::try_from(server_name)
        .map_err(|_| Error::Tls(format!("invalid SNI hostname: {server_name}")))?
        .to_owned();
    let conn = ClientConnection::new(config.config.clone(), name)
        .map_err(|e| Error::Tls(e.to_string()))?;
    let mut tls = StreamOwned::new(conn, tcp);
    while tls.conn.is_handshaking() {
        tls.conn
            .complete_io(&mut tls.sock)
            .map_err(|e| Error::Tls(e.to_string()))?;
    }
    Ok(StdStream::from_tls_client(tls))
}

/// Perform a server TLS handshake over `tcp` (blocking rustls).
pub fn accept_tls(tcp: TcpStream, config: &ServerTlsConfig) -> Result<StdStream> {
    let conn =
        ServerConnection::new(config.config.clone()).map_err(|e| Error::Tls(e.to_string()))?;
    let mut tls = StreamOwned::new(conn, tcp);
    while tls.conn.is_handshaking() {
        tls.conn
            .complete_io(&mut tls.sock)
            .map_err(|e| Error::Tls(e.to_string()))?;
    }
    Ok(StdStream::from_tls_server(tls))
}
