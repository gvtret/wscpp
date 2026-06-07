// bench_masking — RFC 6455 mask/unmask throughput (mirrors C++ bench_masking)

use std::time::Instant;

use ws_rs::frame::{apply_mask, FrameBuilder, ParseResult, Parser};
use ws_rs_benches::common::print_throughput;

fn main() {
    const PAYLOAD_SIZE: usize = 1024 * 1024;
    const MASK_ITERATIONS: i32 = 200;
    const ROUNDTRIP_ITERATIONS: i32 = 50;
    const KEY: [u8; 4] = [0x37, 0xfa, 0x21, 0x3d];

    let mut payload = vec![0u8; PAYLOAD_SIZE];
    for (i, b) in payload.iter_mut().enumerate() {
        *b = (i & 0xFF) as u8;
    }

    let mask_start = Instant::now();
    for _ in 0..MASK_ITERATIONS {
        apply_mask(&mut payload, &KEY);
        apply_mask(&mut payload, &KEY);
    }
    let mask_sec = ws_rs_benches::common::elapsed_sec(mask_start, Instant::now());
    print_throughput(
        "mask_unmask_xor",
        PAYLOAD_SIZE as f64 * MASK_ITERATIONS as f64 * 2.0,
        mask_sec,
        MASK_ITERATIONS * 2,
    );

    let roundtrip_start = Instant::now();
    for _ in 0..ROUNDTRIP_ITERATIONS {
        let frame = FrameBuilder::build_binary(&payload, true, true);
        let mut parser = Parser::new();
        if parser.parse(&frame).0 != ParseResult::Complete {
            eprintln!("masked parse failed");
            std::process::exit(1);
        }
    }
    let rt_sec = ws_rs_benches::common::elapsed_sec(roundtrip_start, Instant::now());
    print_throughput(
        "masked_build_parse",
        PAYLOAD_SIZE as f64 * ROUNDTRIP_ITERATIONS as f64,
        rt_sec,
        ROUNDTRIP_ITERATIONS,
    );
}
