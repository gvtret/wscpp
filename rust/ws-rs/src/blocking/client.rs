use std::net::TcpStream;

use url::Url;

use crate::blocking::connection::Connection;
use crate::blocking::handshake::client_upgrade;
use crate::blocking::transport::StdStream;
use crate::error::{Error, Result};
use crate::frame::Opcode;
use crate::types::Role;

#[cfg(feature = "blocking-tls")]
use crate::blocking::tls::connect_tls;
#[cfg(feature = "blocking-tls")]
use crate::tls::ClientTlsConfig;

/// Synchronous WebSocket client (`ws://` / `wss://` with feature `blocking-tls`).
pub struct Client {
    conn: Option<Connection>,
    request_deflate: bool,
    #[cfg(feature = "blocking-tls")]
    tls: Option<ClientTlsConfig>,
}

impl Client {
    /// Creates a disconnected client with default TLS settings when `blocking-tls` is enabled.
    pub fn new() -> Self {
        Self {
            conn: None,
            request_deflate: false,
            #[cfg(feature = "blocking-tls")]
            tls: None,
        }
    }

    /// Override default TLS settings before `connect()` for `wss://`.
    #[cfg(feature = "blocking-tls")]
    pub fn set_tls_config(&mut self, config: ClientTlsConfig) {
        self.tls = Some(config);
    }

    /// Request RFC 7692 permessage-deflate on connect (default off).
    pub fn enable_permessage_deflate(&mut self, enable: bool) {
        self.request_deflate = enable;
    }

    /// Connect to `url_str`, perform the HTTP upgrade, and store the [`Connection`].
    pub fn connect(&mut self, url_str: &str) -> Result<()> {
        let url = Url::parse(url_str).map_err(|e| Error::InvalidUrl(e.to_string()))?;

        let scheme = url.scheme();
        if scheme != "ws" && scheme != "wss" {
            return Err(Error::InvalidUrl(format!("unsupported scheme: {scheme}")));
        }
        let secure = scheme == "wss";
        #[cfg(not(feature = "blocking-tls"))]
        if secure {
            return Err(Error::InvalidUrl(
                "wss:// requires feature blocking-tls".into(),
            ));
        }

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
        let tcp = TcpStream::connect(&addr).map_err(Error::from)?;

        let mut io = if secure {
            #[cfg(feature = "blocking-tls")]
            {
                let tls_cfg = self.tls.clone().unwrap_or_else(ClientTlsConfig::insecure);
                connect_tls(tcp, host, &tls_cfg)?
            }
            #[cfg(not(feature = "blocking-tls"))]
            {
                unreachable!()
            }
        } else {
            StdStream::from_tcp(tcp)
        };

        let ext = client_upgrade(&mut io, host, &port, &path, self.request_deflate)?;
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
    pub fn send_text(&self, text: &str) -> Result<()> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        conn.send_text(text, true)
    }

    /// Send a text fragment (`fin` false starts or continues a fragmented message).
    pub fn send_text_fragment(&self, text: &str, fin: bool) -> Result<()> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        conn.send_text(text, fin)
    }

    /// Send a continuation frame (must follow an unfinished text/binary fragment).
    pub fn send_continuation(&self, text: &str, fin: bool) -> Result<()> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        conn.send_continuation(text.as_bytes(), fin)
    }

    /// Send a complete binary message (FIN = true).
    pub fn send_binary(&self, data: &[u8]) -> Result<()> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        conn.send_binary(data, true)
    }

    /// Read the next binary message (skips ping/pong internally).
    pub fn recv_binary(&self) -> Result<Option<Vec<u8>>> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        loop {
            let Some(msg) = conn.read_message()? else {
                return Ok(None);
            };
            if msg.opcode == Opcode::Binary {
                return Ok(Some(msg.payload));
            }
        }
    }

    /// Read the next text or binary message as UTF-8 (skips ping/pong internally).
    pub fn recv_text(&self) -> Result<Option<String>> {
        let conn = self
            .conn
            .as_ref()
            .ok_or(Error::Handshake("not connected".into()))?;
        loop {
            let Some(msg) = conn.read_message()? else {
                return Ok(None);
            };
            if msg.opcode == Opcode::Text || msg.opcode == Opcode::Binary {
                return Ok(Some(
                    String::from_utf8(msg.payload)
                        .map_err(|_| Error::Protocol("invalid UTF-8".into()))?,
                ));
            }
        }
    }

    /// Send close (1000) and drop the connection handle.
    pub fn close(&mut self) -> Result<()> {
        if let Some(conn) = self.conn.as_ref() {
            conn.close(1000, "")?;
        }
        self.conn = None;
        Ok(())
    }
}

impl Default for Client {
    fn default() -> Self {
        Self::new()
    }
}
