// bench_fastwebsockets_roundtrip_net — client-only echo + throughput over LAN

use std::env;
use std::time::Instant;

use ws_rs_benches::common::{print_compare_banner, print_latency, print_throughput};
use ws_rs_benches::fastwebsockets_bench::{connect, recv_data, send_binary, send_text};

const THROUGHPUT_BYTES: usize = 64 * 1024;
const THROUGHPUT_ITERATIONS: i32 = 100;

fn usage(argv0: &str) {
    eprintln!("Usage: {argv0} <host> [port] [samples]");
    eprintln!("  Defaults: port=19081 samples=100");
}

#[tokio::main]
async fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        usage(&args[0]);
        std::process::exit(1);
    }

    let host = &args[1];
    let port: u16 = args.get(2).and_then(|s| s.parse().ok()).unwrap_or(19081);
    let samples: usize = args.get(3).and_then(|s| s.parse().ok()).unwrap_or(100);

    print_compare_banner("bench_fastwebsockets_roundtrip_net");
    println!("target ws://{host}:{port}/ samples={samples}");

    let mut ws = match connect(host, port).await {
        Ok(v) => v,
        Err(_) => {
            eprintln!("connect failed");
            std::process::exit(1);
        }
    };

    let mut latencies_ms = Vec::with_capacity(samples);
    for i in 0..samples {
        let msg = format!("ping-{i}");
        let start = Instant::now();
        if send_text(&mut ws, &msg).await.is_err() {
            break;
        }
        if recv_data(&mut ws).await.is_err() {
            break;
        }
        latencies_ms.push(ws_rs_benches::common::elapsed_sec(start, Instant::now()) * 1000.0);
    }

    println!(
        "fastwebsockets net roundtrip ({} samples)",
        latencies_ms.len()
    );
    print_latency("echo_latency", &latencies_ms);

    let blob = vec![0xABu8; THROUGHPUT_BYTES];
    let tp_start = Instant::now();
    let mut tp_ws = match connect(host, port).await {
        Ok(v) => v,
        Err(_) => {
            eprintln!("throughput connect failed");
            std::process::exit(1);
        }
    };

    let mut tp_iterations = 0i32;
    if send_binary(&mut tp_ws, &blob).await.is_ok() {
        loop {
            match recv_data(&mut tp_ws).await {
                Ok(data) if data.len() == THROUGHPUT_BYTES => {
                    tp_iterations += 1;
                    if tp_iterations < THROUGHPUT_ITERATIONS {
                        if send_binary(&mut tp_ws, &blob).await.is_err() {
                            break;
                        }
                    } else {
                        break;
                    }
                }
                _ => break,
            }
        }
    }
    let tp_end = Instant::now();

    print_throughput(
        "echo_throughput_64k",
        THROUGHPUT_BYTES as f64 * tp_iterations as f64,
        ws_rs_benches::common::elapsed_sec(tp_start, tp_end),
        tp_iterations,
    );

    if latencies_ms.len() != samples {
        std::process::exit(1);
    }
}
