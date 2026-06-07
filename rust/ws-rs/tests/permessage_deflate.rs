use ws_rs::extensions::permessage_deflate::{
    compress_message, decompress_message, header_offers_permessage_deflate, OFFER,
};

#[test]
fn hello_rfc7692_vector() {
    let compressed = compress_message(b"Hello").unwrap();
    let expected = [0xf2, 0x48, 0xcd, 0xc9, 0xc9, 0x07, 0x00];
    assert_eq!(compressed.as_slice(), expected);
}

#[test]
fn round_trip_text() {
    let input = "The quick brown fox jumps over the lazy dog. \
                 RFC 7692 permessage-deflate test payload.";
    let compressed = compress_message(input.as_bytes()).unwrap();
    let plain = decompress_message(&compressed).unwrap();
    assert_eq!(String::from_utf8(plain).unwrap(), input);
}

#[test]
fn extension_header_detection() {
    assert!(header_offers_permessage_deflate(
        "permessage-deflate; client_no_context_takeover"
    ));
    assert!(!header_offers_permessage_deflate(""));
    assert!(OFFER.contains("permessage-deflate"));
}
