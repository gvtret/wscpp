use std::net::{TcpListener, TcpStream};
use std::sync::Arc;
use std::thread;

use crate::blocking::connection::Connection;
use crate::blocking::handshake::server_upgrade;
use crate::blocking::transport::StdStream;
use crate::error::{Error, Result};
use crate::frame::Opcode;
use crate::types::Role;

#[cfg(feature = "blocking-tls")]
use crate::blocking::tls::accept_tls;
#[cfg(feature = "blocking-tls")]
use crate::tls::ServerTlsConfig;

/// Synchronous WebSocket server (poll-based accept).
pub struct Server {
    listener: Option<TcpListener>,
    addr: String,
    #[cfg(feature = "blocking-tls")]
    tls: Option<ServerTlsConfig>,
}

impl Server {
    /// Creates an unbound server.
    pub fn new() -> Self {
        Self {
            listener: None,
            addr: String::new(),
            #[cfg(feature = "blocking-tls")]
            tls: None,
        }
    }

    #[cfg(feature = "blocking-tls")]
    /// Enable WSS — TLS handshake runs before the WebSocket upgrade.
    pub fn set_tls_config(&mut self, config: ServerTlsConfig) {
        self.tls = Some(config);
    }

    /// Binds `host:port` and stores the listening address in [`local_addr`](Self::local_addr).
    pub fn listen(&mut self, host: &str, port: u16) -> Result<()> {
        let addr = format!("{host}:{port}");
        let listener = TcpListener::bind(&addr).map_err(Error::from)?;
        self.addr = listener.local_addr().map_err(Error::from)?.to_string();
        self.listener = Some(listener);
        Ok(())
    }

    /// Bound address (`host:port`) after [`listen`](Self::listen).
    pub fn local_addr(&self) -> &str {
        &self.addr
    }

    /// Accept one TCP connection and complete the WebSocket handshake.
    pub fn accept_handshake(&self) -> Result<Connection> {
        let (stream, _) = self
            .listener
            .as_ref()
            .ok_or_else(|| Error::Protocol("server not listening".into()))?
            .accept()
            .map_err(Error::from)?;
        Self::complete_handshake(stream, self.tls.as_ref())
    }

    /// Run TLS (if configured) and WebSocket upgrade on an accepted TCP stream.
    pub fn complete_handshake(
        stream: TcpStream,
        #[cfg(feature = "blocking-tls")] tls: Option<&ServerTlsConfig>,
        #[cfg(not(feature = "blocking-tls"))] _tls: Option<&()>,
    ) -> Result<Connection> {
        let mut io = {
            #[cfg(feature = "blocking-tls")]
            {
                if let Some(cfg) = tls {
                    accept_tls(stream, cfg)?
                } else {
                    StdStream::from_tcp(stream)
                }
            }
            #[cfg(not(feature = "blocking-tls"))]
            {
                StdStream::from_tcp(stream)
            }
        };
        let ext = server_upgrade(&mut io)?;
        let conn = Connection::new(io, Role::Server);
        if ext.permessage_deflate {
            conn.set_permessage_deflate(true);
        }
        Ok(conn)
    }
}

impl Default for Server {
    fn default() -> Self {
        Self::new()
    }
}

/// Echo text and binary messages until the peer closes (sync).
pub fn echo_session(conn: Arc<Connection>) -> Result<()> {
    while conn.is_open() {
        let Some(msg) = conn.read_message()? else {
            break;
        };
        match msg.opcode {
            Opcode::Text => {
                conn.send_text(
                    std::str::from_utf8(&msg.payload)
                        .map_err(|_| Error::Protocol("invalid UTF-8".into()))?,
                    true,
                )?;
            }
            Opcode::Binary => {
                conn.send_binary(&msg.payload, true)?;
            }
            _ => {}
        }
    }
    Ok(())
}

/// Poll-based accept loop (mirrors C++ linux_socket server).
pub fn serve_echo(listener: &TcpListener) -> Result<()> {
    for stream in listener.incoming() {
        let stream = stream.map_err(Error::from)?;
        let conn = Arc::new(Server::complete_handshake(stream, None)?);
        let conn2 = Arc::clone(&conn);
        thread::spawn(move || {
            let _ = echo_session(conn2);
        });
    }
    Ok(())
}
