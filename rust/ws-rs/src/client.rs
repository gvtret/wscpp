use std::time::Duration;

use tokio::net::TcpStream;
use url::Url;

use crate::connection::Connection;
use crate::error::{Error, Result};
use crate::frame::Opcode;
use crate::handshake::client_upgrade;
use crate::tls::{connect_tls, ClientTlsConfig};
use crate::transport::IoStream;
use crate::types::Role;

/// Async WebSocket client (`ws://` / `wss://`).
///
/// Pull-style API: call [`recv_text`](Self::recv_text) / [`connection`](Self::connection) after
/// [`connect`](Self::connect). For event-driven code, spawn a task that loops on
/// [`Connection::read_message`](crate::connection::Connection::read_message).
pub struct Client {
    conn: Option<Connection>,
    tls: Option<ClientTlsConfig>,
    request_deflate: bool,
}

impl Client {
    /// Creates a disconnected client with default TLS settings (`verify_none` for `wss://`).
    pub fn new() -> Self {
        Self {
            conn: None,
            tls: None,
            request_deflate: false,
        }
    }

    /// Request RFC 7692 permessage-deflate on connect (default off).
    pub fn enable_permessage_deflate(&mut self, enable: bool) {
        self.request_deflate = enable;
    }

    /// Override default TLS settings before `connect()` for `wss://`.
    /// Default: accept any server certificate (`verify_none`, same as C++ client).
    pub fn set_tls_config(&mut self, config: ClientTlsConfig) {
        self.tls = Some(config);
    }

    /// Connects to `url_str`, performs the HTTP upgrade, and stores the [`Connection`].
    pub async fn connect(&mut self, url_str: &str) -> Result<()> {
        let url = Url::parse(url_str).map_err(|e| Error::InvalidUrl(e.to_string()))?;

        let scheme = url.scheme();
        if scheme != "ws" && scheme != "wss" {
            return Err(Error::InvalidUrl(format!("unsupported scheme: {scheme}")));
        }
        let secure = scheme == "wss";

        let host = url
            .host_str()
            .ok_or_else(|| Error::InvalidUrl("missing host".into()))?;
        let port = url.port().map(|p| p.to_string()).unwrap_or_else(|| {
            if secure {
                "443".into()
            } else {
                "80".into()
            }
        });
        let path = if url.path().is_empty() {
            "/".to_string()
        } else {
            url.path().to_string()
        };
        let path = match url.query() {
            Some(q) => format!("{path}?{q}"),
            None => path,
        };

        let addr = format!("{host}:{port}");
        let tcp = TcpStream::connect(&addr).await?;
        tcp.set_nodelay(true).ok();

        let mut io = if secure {
            let tls_cfg = self.tls.clone().unwrap_or_else(ClientTlsConfig::insecure);
            let tls = connect_tls(tcp, host, &tls_cfg).await?;
            IoStream::from_tls_client(tls)
        } else {
            IoStream::from_tcp(tcp)
        };

        let ext = client_upgrade(&mut io, host, &port, &path, self.request_deflate).await?;
        let conn = Connection::new(io, Role::Client);
        if ext.permessage_deflate {
            conn.set_permessage_deflate(true);
        }
        self.conn = Some(conn);
        Ok(())
    }

    /// Borrow the underlying [`Connection`] after [`connect`](Self::connect).
    pub fn connection(&self) -> Option<&Connection> {
        self.conn.as_ref()
    }

    /// Send a complete UTF-8 text message (FIN = true).
    pub async fn send_text(&self, text: &str) -> Result<()> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        conn.send_text(text, true).await
    }

    /// Send a text fragment (`fin` false starts or continues a fragmented message).
    pub async fn send_text_fragment(&self, text: &str, fin: bool) -> Result<()> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        conn.send_text(text, fin).await
    }

    /// Send a continuation frame (must follow an unfinished text/binary fragment).
    pub async fn send_continuation(&self, text: &str, fin: bool) -> Result<()> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        conn.send_continuation(text.as_bytes(), fin).await
    }

    /// Send a complete binary message (FIN = true).
    pub async fn send_binary(&self, data: &[u8]) -> Result<()> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        conn.send_binary(data, true).await
    }

    /// Send a ping control frame.
    pub async fn send_ping(&self, data: &[u8]) -> Result<()> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        conn.send_ping(data).await
    }

    /// Read the next text or binary message as UTF-8 (skips ping/pong internally).
    pub async fn recv_text(&self) -> Result<Option<String>> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        loop {
            let Some(msg) = conn.read_message().await? else {
                return Ok(None);
            };
            if msg.opcode == Opcode::Text {
                return Ok(Some(
                    String::from_utf8(msg.payload)
                        .map_err(|_| Error::Protocol("invalid UTF-8".into()))?,
                ));
            }
            if msg.opcode == Opcode::Binary {
                return Ok(Some(
                    String::from_utf8(msg.payload)
                        .map_err(|_| Error::Protocol("invalid UTF-8".into()))?,
                ));
            }
        }
    }

    /// Read the next binary message (skips ping/pong internally).
    pub async fn recv_binary(&self) -> Result<Option<Vec<u8>>> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        loop {
            let Some(msg) = conn.read_message().await? else {
                return Ok(None);
            };
            if msg.opcode == Opcode::Binary {
                return Ok(Some(msg.payload));
            }
        }
    }

    /// Send close (1000) and drop the connection handle.
    pub async fn close(&mut self) -> Result<()> {
        if let Some(conn) = self.conn.as_ref() {
            conn.close(1000, "").await?;
        }
        self.conn = None;
        Ok(())
    }

    /// Return true while a connection handle is held (not yet closed).
    pub async fn is_open(&self) -> bool {
        self.conn.is_some()
    }

    /// Return true when the underlying stream uses TLS.
    pub async fn is_ssl(&self) -> bool {
        match self.conn.as_ref() {
            Some(c) => c.is_tls_connected().await,
            None => false,
        }
    }
}

impl Default for Client {
    fn default() -> Self {
        Self::new()
    }
}

/// Blocking-style roundtrip helper for benchmarks and tests.
pub async fn roundtrip_once(url: &str, payload: &str) -> Result<Duration> {
    let mut client = Client::new();
    client.connect(url).await?;
    let start = std::time::Instant::now();
    client.send_text(payload).await?;
    let _ = client.recv_text().await?;
    let elapsed = start.elapsed();
    client.close().await?;
    Ok(elapsed)
}
