use std::io::{self, Read, Write};
use std::net::TcpStream;

#[cfg(feature = "blocking-tls")]
use rustls::{ClientConnection, ServerConnection, StreamOwned};

/// Sync TCP/TLS stream (mirrors C++ `linux_socket`).
pub enum StdStream {
    /// Plain TCP connection.
    Tcp(TcpStream),
    #[cfg(feature = "blocking-tls")]
    /// Client-side TLS (outbound `wss://`).
    TlsClient(StreamOwned<ClientConnection, TcpStream>),
    #[cfg(feature = "blocking-tls")]
    /// Server-side TLS (inbound WSS).
    TlsServer(StreamOwned<ServerConnection, TcpStream>),
}

impl StdStream {
    /// Connect to `addr` and enable TCP_NODELAY.
    pub fn connect(addr: &str) -> io::Result<Self> {
        let inner = TcpStream::connect(addr)?;
        inner.set_nodelay(true).ok();
        Ok(Self::Tcp(inner))
    }

    /// Wrap a connected TCP stream (sets TCP_NODELAY).
    pub fn from_tcp(stream: TcpStream) -> Self {
        stream.set_nodelay(true).ok();
        Self::Tcp(stream)
    }

    #[cfg(feature = "blocking-tls")]
    /// Wrap a client TLS stream after handshake.
    pub fn from_tls_client(stream: StreamOwned<ClientConnection, TcpStream>) -> Self {
        stream.sock.set_nodelay(true).ok();
        Self::TlsClient(stream)
    }

    #[cfg(feature = "blocking-tls")]
    /// Wrap a server TLS stream after handshake.
    pub fn from_tls_server(stream: StreamOwned<ServerConnection, TcpStream>) -> Self {
        stream.sock.set_nodelay(true).ok();
        Self::TlsServer(stream)
    }

    /// Read up to `buf.len()` bytes (blocking).
    pub fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        match self {
            Self::Tcp(s) => s.read(buf),
            #[cfg(feature = "blocking-tls")]
            Self::TlsClient(s) => s.read(buf),
            #[cfg(feature = "blocking-tls")]
            Self::TlsServer(s) => s.read(buf),
        }
    }

    /// Write all bytes (blocking).
    pub fn write_all(&mut self, buf: &[u8]) -> io::Result<()> {
        match self {
            Self::Tcp(s) => s.write_all(buf),
            #[cfg(feature = "blocking-tls")]
            Self::TlsClient(s) => s.write_all(buf),
            #[cfg(feature = "blocking-tls")]
            Self::TlsServer(s) => s.write_all(buf),
        }
    }

    /// Flush buffered writes (blocking).
    pub fn flush(&mut self) -> io::Result<()> {
        match self {
            Self::Tcp(s) => s.flush(),
            #[cfg(feature = "blocking-tls")]
            Self::TlsClient(s) => s.flush(),
            #[cfg(feature = "blocking-tls")]
            Self::TlsServer(s) => s.flush(),
        }
    }

    /// Return true for TLS client or server streams.
    pub fn is_tls(&self) -> bool {
        match self {
            Self::Tcp(_) => false,
            #[cfg(feature = "blocking-tls")]
            Self::TlsClient(_) | Self::TlsServer(_) => true,
        }
    }
}

impl Read for StdStream {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        match self {
            Self::Tcp(s) => s.read(buf),
            #[cfg(feature = "blocking-tls")]
            Self::TlsClient(s) => s.read(buf),
            #[cfg(feature = "blocking-tls")]
            Self::TlsServer(s) => s.read(buf),
        }
    }
}

impl Write for StdStream {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        match self {
            Self::Tcp(s) => s.write(buf),
            #[cfg(feature = "blocking-tls")]
            Self::TlsClient(s) => s.write(buf),
            #[cfg(feature = "blocking-tls")]
            Self::TlsServer(s) => s.write(buf),
        }
    }

    fn flush(&mut self) -> io::Result<()> {
        match self {
            Self::Tcp(s) => s.flush(),
            #[cfg(feature = "blocking-tls")]
            Self::TlsClient(s) => s.flush(),
            #[cfg(feature = "blocking-tls")]
            Self::TlsServer(s) => s.flush(),
        }
    }
}
