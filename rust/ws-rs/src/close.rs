//! RFC 6455 §7.4 close code validation (mirrors C++ `connection::is_valid_close_code`).

/// Returns whether `code` is allowed in an outbound CLOSE frame (RFC 6455 §7.4.1).
pub fn is_valid_close_code(code: u16) -> bool {
    if code < 1000 {
        return false;
    }
    if (1000..=1003).contains(&code) {
        return true;
    }
    if (1004..=1006).contains(&code) {
        return false;
    }
    if (1007..=1011).contains(&code) {
        return true;
    }
    if (1012..=2999).contains(&code) {
        return false;
    }
    code <= 4999
}

/// Maps invalid outbound codes to **1002** (protocol error).
pub fn sanitize_close_code(code: u16) -> u16 {
    if is_valid_close_code(code) {
        code
    } else {
        1002
    }
}

/// Parses a CLOSE frame payload into `(code, reason)`; invalid peer codes become **1002**.
pub fn parse_close_payload(payload: &[u8]) -> (u16, String) {
    if payload.len() < 2 {
        return (1000, String::new());
    }
    let code = u16::from_be_bytes([payload[0], payload[1]]);
    let reason = String::from_utf8_lossy(&payload[2..]).into_owned();
    (sanitize_close_code(code), reason)
}
