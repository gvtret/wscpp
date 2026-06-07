use ws_rs::frame::{FrameBuilder, Opcode, ParseResult, Parser};

#[test]
fn section_5_7_unmasked_text_hello() {
    let raw = [0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f];
    let mut p = Parser::new();
    let (result, _) = p.parse(&raw);
    assert_eq!(result, ParseResult::Complete);
    let frame = p.frame();
    assert!(frame.header.fin);
    assert_eq!(frame.header.opcode, Opcode::Text);
    assert_eq!(std::str::from_utf8(&frame.payload).unwrap(), "Hello");
}

#[test]
fn section_5_7_masked_text_hello() {
    let raw = [
        0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58,
    ];
    let mut p = Parser::new();
    let (result, _) = p.parse(&raw);
    assert_eq!(result, ParseResult::Complete);
    let frame = p.frame();
    assert!(frame.header.mask);
    assert_eq!(std::str::from_utf8(&frame.payload).unwrap(), "Hello");
}

#[test]
fn close_frame_round_trip() {
    let frame = FrameBuilder::build_close(1000, "bye", false);
    let mut p = Parser::new();
    let (result, _) = p.parse(&frame);
    assert_eq!(result, ParseResult::Complete);
    let parsed = p.frame();
    assert_eq!(parsed.header.opcode, Opcode::Close);
    assert!(parsed.payload.len() >= 2);
    let code = u16::from_be_bytes([parsed.payload[0], parsed.payload[1]]);
    assert_eq!(code, 1000);
}

#[test]
fn ping_pong_opcodes() {
    let ping = FrameBuilder::build_ping(b"hi", false);
    let pong = FrameBuilder::build_pong(b"hi", false);
    assert_eq!(ping[0] & 0x0F, Opcode::Ping as u8);
    assert_eq!(pong[0] & 0x0F, Opcode::Pong as u8);
}

#[test]
fn invalid_opcode_rejected() {
    let raw = [0x8F, 0x00];
    let mut p = Parser::new();
    let (result, _) = p.parse(&raw);
    assert_eq!(result, ParseResult::Error);
}

#[test]
fn fragmented_frames_parse_separately() {
    let text = "fragmented message";
    let frag1 = FrameBuilder::build_text(&text[..10], false, false);
    let frag2 =
        FrameBuilder::build_opcode(Opcode::Continuation, &text.as_bytes()[10..], true, false);

    let mut p = Parser::new();
    let (r1, _) = p.parse(&frag1);
    assert_eq!(r1, ParseResult::Complete);
    let f1 = p.frame();
    assert!(!f1.header.fin);
    assert_eq!(f1.header.opcode, Opcode::Text);

    p.reset();
    let (r2, _) = p.parse(&frag2);
    assert_eq!(r2, ParseResult::Complete);
    let f2 = p.frame();
    assert!(f2.header.fin);
    assert_eq!(f2.header.opcode, Opcode::Continuation);
    assert_eq!(std::str::from_utf8(&f2.payload).unwrap(), &text[10..]);
}
