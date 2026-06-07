use std::sync::Arc;

use tokio::net::{TcpListener, TcpStream};
use tokio::sync::Mutex;

use crate::connection::Connection;
use crate::error::{Error, Result};
use crate::frame::Opcode;
use crate::handshake::server_upgrade;
use crate::tls::{accept_tls, ServerTlsConfig};
use crate::transport::IoStream;
use crate::types::Role;

/// Async WebSocket server: bind, accept TCP/TLS, complete handshake per connection.
pub struct Server {
    listener: Option<TcpListener>,
    addr: String,
    tls: Option<ServerTlsConfig>,
}

impl Server {
    /// Creates an unbound server.
    pub fn new() -> Self {
        Self {
            listener: None,
            addr: String::new(),
            tls: None,
        }
    }

    /// Enable WSS — TLS handshake runs before the WebSocket upgrade (RFC 2818).
    pub fn set_tls_config(&mut self, config: ServerTlsConfig) {
        self.tls = Some(config);
    }

    /// Binds `host:port` and stores the listening address in [`local_addr`](Self::local_addr).
    pub async fn listen(&mut self, host: &str, port: u16) -> Result<()> {
        let addr = format!("{host}:{port}");
        let listener = TcpListener::bind(&addr).await?;
        self.addr = listener.local_addr()?.to_string();
        self.listener = Some(listener);
        Ok(())
    }

    /// Bound address (`host:port`) after [`listen`](Self::listen).
    pub fn local_addr(&self) -> &str {
        &self.addr
    }

    /// Borrow the tokio [`TcpListener`] (errors if not listening).
    pub fn listener(&self) -> Result<&TcpListener> {
        self.listener
            .as_ref()
            .ok_or_else(|| Error::Protocol("server not listening".into()))
    }

    /// Accept one TCP connection and complete the WebSocket handshake.
    pub async fn accept_handshake(&self) -> Result<Connection> {
        let (stream, _) = self.listener()?.accept().await?;
        Self::complete_handshake(stream, self.tls.as_ref()).await
    }

    /// Run TLS (if configured) and WebSocket upgrade on an accepted TCP stream.
    pub async fn complete_handshake(
        stream: TcpStream,
        tls: Option<&ServerTlsConfig>,
    ) -> Result<Connection> {
        stream.set_nodelay(true).ok();

        let mut io = if let Some(cfg) = tls {
            let tls_stream = accept_tls(stream, cfg).await?;
            IoStream::from_tls_server(tls_stream)
        } else {
            IoStream::from_tcp(stream)
        };

        let ext = server_upgrade(&mut io).await?;
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

/// Simple echo handler for one connection (used in examples and benches).
pub async fn echo_session(conn: Arc<Connection>) -> Result<()> {
    while conn.is_open().await {
        let Some(msg) = conn.read_message().await? else {
            break;
        };
        match msg.opcode {
            Opcode::Text => {
                conn.send_text(
                    std::str::from_utf8(&msg.payload)
                        .map_err(|_| Error::Protocol("invalid UTF-8".into()))?,
                    true,
                )
                .await?;
            }
            Opcode::Binary => {
                conn.send_binary(&msg.payload, true).await?;
            }
            Opcode::Ping | Opcode::Pong | Opcode::Close | Opcode::Continuation => {}
        }
    }
    Ok(())
}

/// Run echo server until `stop` is set to true.
pub async fn run_echo_server(host: &str, port: u16, stop: Arc<Mutex<bool>>) -> Result<()> {
    let mut server = Server::new();
    server.listen(host, port).await?;

    loop {
        if *stop.lock().await {
            break;
        }
        let accept = server.listener()?.accept();
        let (stream, _) = tokio::select! {
            res = accept => res?,
            _ = tokio::time::sleep(std::time::Duration::from_millis(50)) => continue,
        };
        let conn = match Server::complete_handshake(stream, None).await {
            Ok(c) => Arc::new(c),
            Err(_) => continue,
        };
        tokio::spawn(async move {
            let _ = echo_session(conn).await;
        });
    }
    Ok(())
}
