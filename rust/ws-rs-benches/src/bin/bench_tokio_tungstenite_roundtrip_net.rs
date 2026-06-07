// bench_tokio_tungstenite_roundtrip_net — client-only echo + throughput over LAN

use std::env;
use std::time::Instant;

use futures_util::{SinkExt, StreamExt};
use tokio_tungstenite::{connect_async, tungstenite::Message};
use ws_rs_benches::common::{print_compare_banner, print_latency, print_throughput};

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

    print_compare_banner("bench_tokio_tungstenite_roundtrip_net");
    println!("target ws://{host}:{port}/ samples={samples}");

    let url = format!("ws://{host}:{port}/");

    let (mut ws, _) = match connect_async(&url).await {
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
        if ws.send(Message::Text(msg)).await.is_err() {
            break;
        }
        if ws.next().await.is_none() {
            break;
        }
        latencies_ms.push(ws_rs_benches::common::elapsed_sec(start, Instant::now()) * 1000.0);
    }
    let _ = ws.close(None).await;

    println!(
        "tokio-tungstenite net roundtrip ({} samples)",
        latencies_ms.len()
    );
    print_latency("echo_latency", &latencies_ms);

    let blob = vec![0xABu8; THROUGHPUT_BYTES];
    let tp_start = Instant::now();
    let (mut tp_ws, _) = match connect_async(&url).await {
        Ok(v) => v,
        Err(_) => {
            eprintln!("throughput connect failed");
            std::process::exit(1);
        }
    };

    let mut tp_iterations = 0i32;
    if tp_ws.send(Message::Binary(blob.clone())).await.is_ok() {
        loop {
            match tp_ws.next().await {
                Some(Ok(Message::Binary(data))) if data.len() == THROUGHPUT_BYTES => {
                    tp_iterations += 1;
                    if tp_iterations < THROUGHPUT_ITERATIONS {
                        if tp_ws.send(Message::Binary(blob.clone())).await.is_err() {
                            break;
                        }
                    } else {
                        let _ = tp_ws.close(None).await;
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
