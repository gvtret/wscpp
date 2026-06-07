// bench_frame_parse — parse/build throughput for 1 MiB binary payload (mirrors C++ bench_frame_parse)

use std::time::Instant;

use ws_rs::frame::{FrameBuilder, ParseResult, Parser};
use ws_rs_benches::common::print_throughput;

fn main() {
    const PAYLOAD_SIZE: usize = 1024 * 1024;
    const ITERATIONS: i32 = 50;

    let mut payload = vec![0u8; PAYLOAD_SIZE];
    for (i, b) in payload.iter_mut().enumerate() {
        *b = (i & 0xFF) as u8;
    }

    let build_start = Instant::now();
    let mut frame = Vec::new();
    for _ in 0..ITERATIONS {
        frame = FrameBuilder::build_binary(&payload, true, false);
    }
    let build_sec = ws_rs_benches::common::elapsed_sec(build_start, Instant::now());
    print_throughput(
        "frame_build",
        PAYLOAD_SIZE as f64 * ITERATIONS as f64,
        build_sec,
        ITERATIONS,
    );

    let parse_start = Instant::now();
    for _ in 0..ITERATIONS {
        let mut parser = Parser::new();
        let (result, _) = parser.parse(&frame);
        if result != ParseResult::Complete || parser.frame().payload.len() != PAYLOAD_SIZE {
            eprintln!("parse failed");
            std::process::exit(1);
        }
    }
    let parse_sec = ws_rs_benches::common::elapsed_sec(parse_start, Instant::now());
    print_throughput(
        "frame_parse",
        PAYLOAD_SIZE as f64 * ITERATIONS as f64,
        parse_sec,
        ITERATIONS,
    );
}
