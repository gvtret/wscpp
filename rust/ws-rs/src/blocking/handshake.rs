use std::io::{Read, Write};

use crate::error::{Error, Result};
use crate::extensions::negotiate::{header_offers_permessage_deflate, OFFER};
use crate::handshake::{
    build_client_request, build_server_response, compute_accept, generate_key,
    parse_client_upgrade, parse_http_headers, parse_server_response, verify_server_accept,
    HandshakeExtensions,
};

fn read_until_headers_end(stream: &mut impl Read) -> Result<Vec<u8>> {
    let mut buf = Vec::new();
    let mut tmp = [0u8; 1024];
    loop {
        let n = stream.read(&mut tmp).map_err(Error::from)?;
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

/// Perform client-side HTTP → WebSocket upgrade over a blocking stream.
pub fn client_upgrade(
    stream: &mut (impl Read + Write),
    host: &str,
    port: &str,
    path: &str,
    request_deflate: bool,
) -> Result<HandshakeExtensions> {
    let key = generate_key();
    let extensions = if request_deflate { Some(OFFER) } else { None };
    let request = build_client_request(host, port, path, &key, extensions);
    stream.write_all(request.as_bytes()).map_err(Error::from)?;
    stream.flush().map_err(Error::from)?;

    let response = read_until_headers_end(stream)?;
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

/// Perform server-side HTTP → WebSocket upgrade over a blocking stream.
pub fn server_upgrade(stream: &mut (impl Read + Write)) -> Result<HandshakeExtensions> {
    let request = read_until_headers_end(stream)?;
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
    stream.write_all(response.as_bytes()).map_err(Error::from)?;
    stream.flush().map_err(Error::from)?;
    Ok(HandshakeExtensions { permessage_deflate })
}
