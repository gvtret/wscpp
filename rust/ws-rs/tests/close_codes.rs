use ws_rs::close::{is_valid_close_code, parse_close_payload, sanitize_close_code};

#[test]
fn valid_close_codes() {
    assert!(is_valid_close_code(1000));
    assert!(is_valid_close_code(1007));
    assert!(is_valid_close_code(1011));
    assert!(!is_valid_close_code(999));
    assert!(!is_valid_close_code(1006));
    assert!(!is_valid_close_code(5000));
}

#[test]
fn sanitize_invalid_peer_code() {
    assert_eq!(sanitize_close_code(1006), 1002);
}

#[test]
fn parse_close_payload_roundtrip() {
    let payload = [0x03, 0xE8]; // 1000
    let (code, reason) = parse_close_payload(&payload);
    assert_eq!(code, 1000);
    assert!(reason.is_empty());
}
