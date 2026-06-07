use ws_rs::utf8::is_valid_utf8;

#[test]
fn accepts_ascii() {
    assert!(is_valid_utf8(b"Hello"));
}

#[test]
fn accepts_valid_multibyte() {
    assert!(is_valid_utf8(&[0xC2, 0xA9]));
}

#[test]
fn rejects_overlong_encoding() {
    assert!(!is_valid_utf8(&[0xC0, 0x80]));
}

#[test]
fn rejects_surrogate_half() {
    assert!(!is_valid_utf8(&[0xED, 0xA0, 0x80]));
}

#[test]
fn rejects_truncated_sequence() {
    assert!(!is_valid_utf8(&[0xE2, 0x82]));
}

#[test]
fn rejects_invalid_lead_byte() {
    assert!(!is_valid_utf8(&[0xFF]));
}

#[test]
fn empty_is_valid() {
    assert!(is_valid_utf8(&[]));
}
