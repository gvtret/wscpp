use std::io;
use std::pin::Pin;
use std::task::{Context, Poll};

use tokio::io::{AsyncRead, AsyncReadExt, AsyncWrite, AsyncWriteExt, ReadBuf};
use tokio::net::TcpStream;
use tokio_rustls::client::TlsStream as ClientTlsStream;
use tokio_rustls::server::TlsStream as ServerTlsStream;

/// Plain TCP or TLS-wrapped stream for WebSocket I/O.
pub enum IoStream {
    /// Plain TCP connection.
    Tcp(TcpStream),
    /// Client-side TLS (outbound `wss://`).
    TlsClient(ClientTlsStream<TcpStream>),
    /// Server-side TLS (inbound WSS).
    TlsServer(ServerTlsStream<TcpStream>),
}

impl IoStream {
    /// Wrap a connected TCP stream.
    pub fn from_tcp(stream: TcpStream) -> Self {
        Self::Tcp(stream)
    }

    /// Wrap a client TLS stream after handshake.
    pub fn from_tls_client(stream: ClientTlsStream<TcpStream>) -> Self {
        Self::TlsClient(stream)
    }

    /// Wrap a server TLS stream after handshake.
    pub fn from_tls_server(stream: ServerTlsStream<TcpStream>) -> Self {
        Self::TlsServer(stream)
    }

    /// Read up to `buf.len()` bytes (async).
    pub async fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        match self {
            Self::Tcp(s) => s.read(buf).await,
            Self::TlsClient(s) => s.read(buf).await,
            Self::TlsServer(s) => s.read(buf).await,
        }
    }

    /// Write all bytes (async).
    pub async fn write_all(&mut self, buf: &[u8]) -> io::Result<()> {
        match self {
            Self::Tcp(s) => s.write_all(buf).await,
            Self::TlsClient(s) => s.write_all(buf).await,
            Self::TlsServer(s) => s.write_all(buf).await,
        }
    }

    /// Flush buffered writes (async).
    pub async fn flush(&mut self) -> io::Result<()> {
        match self {
            Self::Tcp(s) => s.flush().await,
            Self::TlsClient(s) => s.flush().await,
            Self::TlsServer(s) => s.flush().await,
        }
    }

    /// Return true for TLS client or server streams.
    pub fn is_tls(&self) -> bool {
        !matches!(self, Self::Tcp(_))
    }
}

impl AsyncRead for IoStream {
    fn poll_read(
        mut self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &mut ReadBuf<'_>,
    ) -> Poll<io::Result<()>> {
        match &mut *self {
            Self::Tcp(s) => Pin::new(s).poll_read(cx, buf),
            Self::TlsClient(s) => Pin::new(s).poll_read(cx, buf),
            Self::TlsServer(s) => Pin::new(s).poll_read(cx, buf),
        }
    }
}

impl AsyncWrite for IoStream {
    fn poll_write(
        mut self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &[u8],
    ) -> Poll<io::Result<usize>> {
        match &mut *self {
            Self::Tcp(s) => Pin::new(s).poll_write(cx, buf),
            Self::TlsClient(s) => Pin::new(s).poll_write(cx, buf),
            Self::TlsServer(s) => Pin::new(s).poll_write(cx, buf),
        }
    }

    fn poll_flush(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        match &mut *self {
            Self::Tcp(s) => Pin::new(s).poll_flush(cx),
            Self::TlsClient(s) => Pin::new(s).poll_flush(cx),
            Self::TlsServer(s) => Pin::new(s).poll_flush(cx),
        }
    }

    fn poll_shutdown(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        match &mut *self {
            Self::Tcp(s) => Pin::new(s).poll_shutdown(cx),
            Self::TlsClient(s) => Pin::new(s).poll_shutdown(cx),
            Self::TlsServer(s) => Pin::new(s).poll_shutdown(cx),
        }
    }
}
