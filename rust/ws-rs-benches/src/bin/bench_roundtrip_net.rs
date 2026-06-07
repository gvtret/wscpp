// bench_roundtrip_net — WebSocket echo client against a remote host (mirrors C++ bench_roundtrip_net)

use std::env;
use std::time::Instant;

use ws_rs::client::Client;
use ws_rs_benches::common::{print_banner, print_latency, print_throughput};

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

    print_banner("bench_roundtrip_net");
    println!("target ws://{host}:{port}/ samples={samples}");

    let url = format!("ws://{host}:{port}/");

    let mut client = Client::new();
    if client.connect(&url).await.is_err() {
        eprintln!("connect failed");
        std::process::exit(1);
    }

    let mut latencies_ms = Vec::with_capacity(samples);
    for i in 0..samples {
        let msg = format!("ping-{i}");
        let start = Instant::now();
        if client.send_text(&msg).await.is_err() {
            break;
        }
        if client.recv_text().await.is_err() {
            break;
        }
        latencies_ms.push(ws_rs_benches::common::elapsed_sec(start, Instant::now()) * 1000.0);
    }
    client.close().await.ok();

    println!("ws-rs net roundtrip ({} samples)", latencies_ms.len());
    print_latency("echo_latency", &latencies_ms);

    let blob = vec![0xABu8; THROUGHPUT_BYTES];
    let tp_start = Instant::now();
    let mut tp_client = Client::new();
    if tp_client.connect(&url).await.is_err() {
        eprintln!("throughput connect failed");
        std::process::exit(1);
    }

    let mut tp_iterations = 0i32;
    if tp_client.send_binary(&blob).await.is_ok() {
        loop {
            match tp_client.recv_binary().await {
                Ok(Some(data)) if data.len() == THROUGHPUT_BYTES => {
                    tp_iterations += 1;
                    if tp_iterations < THROUGHPUT_ITERATIONS {
                        if tp_client.send_binary(&blob).await.is_err() {
                            break;
                        }
                    } else {
                        tp_client.close().await.ok();
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
