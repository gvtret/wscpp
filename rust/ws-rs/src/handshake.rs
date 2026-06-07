//! RFC 6455 HTTP upgrade (§4): request/response builders and async upgrade helpers.

use std::collections::HashMap;

use base64::{engine::general_purpose::STANDARD, Engine};
use rand::RngCore;
use sha1::{Digest, Sha1};
#[cfg(feature = "async")]
use tokio::io::{AsyncRead, AsyncReadExt, AsyncWrite, AsyncWriteExt};

use crate::error::{Error, Result};
#[cfg(feature = "async")]
use crate::extensions::negotiate::{header_offers_permessage_deflate, OFFER};

const MAGIC: &str = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/// Generate a random `Sec-WebSocket-Key` (16 bytes, base64-encoded).
pub fn generate_key() -> String {
    let mut bytes = [0u8; 16];
    rand::thread_rng().fill_bytes(&mut bytes);
    STANDARD.encode(bytes)
}

/// Compute `Sec-WebSocket-Accept` from the client's key (RFC 6455 §4.2.2).
pub fn compute_accept(key: &str) -> String {
    let mut hasher = Sha1::new();
    hasher.update(key.as_bytes());
    hasher.update(MAGIC.as_bytes());
    STANDARD.encode(hasher.finalize())
}

/// Build an HTTP GET upgrade request (client → server).
pub fn build_client_request(
    host: &str,
    port: &str,
    path: &str,
    key: &str,
    extensions: Option<&str>,
) -> String {
    let mut req = format!("GET {path} HTTP/1.1\r\nHost: {host}");
    if !port.is_empty() && port != "80" && port != "443" {
        req.push(':');
        req.push_str(port);
    }
    req.push_str("\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n");
    req.push_str(&format!("Sec-WebSocket-Key: {key}\r\n"));
    req.push_str("Sec-WebSocket-Version: 13\r\n");
    if let Some(ext) = extensions {
        req.push_str(&format!("Sec-WebSocket-Extensions: {ext}\r\n"));
    }
    req.push_str("\r\n");
    req
}

/// Build an HTTP 101 Switching Protocols response (server → client).
pub fn build_server_response(accept_key: &str, extensions: Option<&str>) -> String {
    let mut resp = String::from("HTTP/1.1 101 Switching Protocols\r\n");
    resp.push_str("Upgrade: websocket\r\n");
    resp.push_str("Connection: Upgrade\r\n");
    resp.push_str(&format!("Sec-WebSocket-Accept: {accept_key}\r\n"));
    if let Some(ext) = extensions {
        resp.push_str(&format!("Sec-WebSocket-Extensions: {ext}\r\n"));
    }
    resp.push_str("\r\n");
    resp
}

/// Parse HTTP headers from a `\r\n`-terminated block; returns the request/status line.
pub fn parse_http_headers(raw: &str) -> Result<(String, HashMap<String, String>)> {
    let mut lines = raw.split("\r\n");
    let request_line = lines
        .next()
        .ok_or_else(|| Error::Handshake("empty request".into()))?
        .to_string();

    let mut headers = HashMap::new();
    for line in lines {
        if line.is_empty() {
            break;
        }
        let Some((name, value)) = line.split_once(':') else {
            continue;
        };
        let value = value.trim_start();
        headers.insert(name.trim().to_ascii_lowercase(), value.to_string());
    }
    Ok((request_line, headers))
}

/// Return true if headers look like a valid WebSocket upgrade request.
pub fn validate_client_request(headers: &HashMap<String, String>) -> bool {
    let upgrade = headers.get("upgrade").map(|s| s.to_ascii_lowercase());
    let connection = headers.get("connection").map(|s| s.to_ascii_lowercase());
    let version = headers.get("sec-websocket-version");
    let key = headers.get("sec-websocket-key");

    matches!(upgrade.as_deref(), Some(s) if s == "websocket")
        && matches!(connection.as_deref(), Some(s) if s.contains("upgrade"))
        && matches!(version.map(String::as_str), Some("13"))
        && key.is_some()
}

/// Parse and validate a client upgrade request; returns request line and headers.
pub fn parse_client_upgrade(raw: &str) -> Result<(String, HashMap<String, String>)> {
    let (line, headers) = parse_http_headers(raw)?;
    if !validate_client_request(&headers) {
        return Err(Error::Handshake("invalid upgrade request".into()));
    }
    Ok((line, headers))
}

/// Validate an HTTP 101 response (status line and required headers).
pub fn parse_server_response(raw: &str) -> Result<()> {
    if !raw.starts_with("HTTP/1.1 101") {
        return Err(Error::Handshake("expected HTTP 101".into()));
    }
    let (_, headers) = parse_http_headers(raw)?;
    let accept = headers.get("sec-websocket-accept");
    if accept.is_none() {
        return Err(Error::Handshake("missing Sec-WebSocket-Accept".into()));
    }
    Ok(())
}

/// Return true when `accept_header` matches the expected accept for `client_key`.
pub fn verify_server_accept(client_key: &str, accept_header: &str) -> bool {
    compute_accept(client_key) == accept_header
}

/// Negotiated extensions after handshake.
#[derive(Debug, Clone, Copy, Default)]
pub struct HandshakeExtensions {
    /// True when RFC 7692 permessage-deflate was negotiated.
    pub permessage_deflate: bool,
}

/// Perform client-side HTTP → WebSocket upgrade over any async stream.
#[cfg(feature = "async")]
pub async fn client_upgrade<S: AsyncRead + AsyncWrite + Unpin>(
    stream: &mut S,
    host: &str,
    port: &str,
    path: &str,
    request_deflate: bool,
) -> Result<HandshakeExtensions> {
    let key = generate_key();
    let extensions = if request_deflate { Some(OFFER) } else { None };
    let request = build_client_request(host, port, path, &key, extensions);
    stream.write_all(request.as_bytes()).await?;
    stream.flush().await?;

    let response = read_until_headers_end(stream).await?;
    let response_str = String::from_utf8_lossy(&response);
    parse_server_response(&response_str)?;

    let (_, headers) = parse_http_headers(&response_str)?;
    let accept = headers
        .get("sec-websocket-accept")
        .ok_or_else(|| Error::Handshake("missing accept header".into()))?;

    if !verify_server_accept(&key, accept) {
        return Err(Error::Handshake("accept key mismatch".into()));
    }

    let permessage_deflate = request_deflate
        && headers
            .get("sec-websocket-extensions")
            .map(|h| header_offers_permessage_deflate(h))
            .unwrap_or(false);

    Ok(HandshakeExtensions { permessage_deflate })
}

/// Perform server-side HTTP → WebSocket upgrade over any async stream.
#[cfg(feature = "async")]
pub async fn server_upgrade<S: AsyncRead + AsyncWrite + Unpin>(
    stream: &mut S,
) -> Result<HandshakeExtensions> {
    let request = read_until_headers_end(stream).await?;
    let request_str = String::from_utf8_lossy(&request);
    let (_, headers) = parse_client_upgrade(&request_str)?;
    let key = headers
        .get("sec-websocket-key")
        .ok_or_else(|| Error::Handshake("missing key".into()))?;
    let accept = compute_accept(key);

    let permessage_deflate = headers
        .get("sec-websocket-extensions")
        .map(|h| header_offers_permessage_deflate(h))
        .unwrap_or(false);
    let response_ext = if permessage_deflate {
        Some(OFFER)
    } else {
        None
    };
    let response = build_server_response(&accept, response_ext);
    stream.write_all(response.as_bytes()).await?;
    stream.flush().await?;
    Ok(HandshakeExtensions { permessage_deflate })
}

#[cfg(feature = "async")]
async fn read_until_headers_end<S: AsyncRead + Unpin>(stream: &mut S) -> Result<Vec<u8>> {
    let mut buf = Vec::new();
    let mut tmp = [0u8; 1024];
    loop {
        let n = stream.read(&mut tmp).await?;
        if n == 0 {
            return Err(Error::Handshake(
                "connection closed during handshake".into(),
            ));
        }
        buf.extend_from_slice(&tmp[..n]);
        if buf.windows(4).any(|w| w == b"\r\n\r\n") {
            return Ok(buf);
        }
        if buf.len() > 64 * 1024 {
            return Err(Error::Handshake("message too large".into()));
        }
    }
}
