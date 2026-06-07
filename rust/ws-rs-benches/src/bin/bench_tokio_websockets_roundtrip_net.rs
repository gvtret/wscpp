// bench_tokio_websockets_roundtrip_net — client-only echo + throughput over LAN

use std::env;
use std::time::Instant;

use futures_util::{SinkExt, StreamExt};
use http::Uri;
use tokio_websockets::{ClientBuilder, Message};
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

    print_compare_banner("bench_tokio_websockets_roundtrip_net");
    println!("target ws://{host}:{port}/ samples={samples}");

    let uri: Uri = format!("ws://{host}:{port}/").parse().expect("uri");
    let (mut client, _) = match ClientBuilder::from_uri(uri.clone()).connect().await {
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
        if client.send(Message::text(msg)).await.is_err() {
            break;
        }
        let got = matches!(
            client.next().await,
            Some(Ok(ref reply)) if reply.is_text() || reply.is_binary()
        );
        if !got {
            break;
        }
        latencies_ms.push(ws_rs_benches::common::elapsed_sec(start, Instant::now()) * 1000.0);
    }

    println!(
        "tokio-websockets net roundtrip ({} samples)",
        latencies_ms.len()
    );
    print_latency("echo_latency", &latencies_ms);

    let blob = vec![0xABu8; THROUGHPUT_BYTES];
    let tp_start = Instant::now();
    let (mut tp_client, _) = match ClientBuilder::from_uri(uri).connect().await {
        Ok(v) => v,
        Err(_) => {
            eprintln!("throughput connect failed");
            std::process::exit(1);
        }
    };

    let mut tp_iterations = 0i32;
    if tp_client.send(Message::binary(blob.clone())).await.is_ok() {
        loop {
            match tp_client.next().await {
                Some(Ok(msg)) if msg.is_binary() && msg.as_payload().len() == THROUGHPUT_BYTES => {
                    tp_iterations += 1;
                    if tp_iterations < THROUGHPUT_ITERATIONS {
                        if tp_client.send(Message::binary(blob.clone())).await.is_err() {
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
