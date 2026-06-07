// bench_roundtrip — localhost WebSocket echo latency + 64 KiB throughput (mirrors C++ bench_roundtrip)

use std::time::Instant;

use ws_rs::client::Client;
use ws_rs_benches::common::{
    pick_free_port, print_banner, print_latency, print_throughput, sleep_ms, spawn_echo_server,
};

const SAMPLES: usize = 100;
const THROUGHPUT_BYTES: usize = 64 * 1024;
const THROUGHPUT_ITERATIONS: i32 = 100;

#[tokio::main]
async fn main() {
    print_banner("bench_roundtrip");

    let port = pick_free_port().await;
    let _server = spawn_echo_server("127.0.0.1", port).await;
    sleep_ms(50).await;

    let url = format!("ws://127.0.0.1:{port}/");
    let mut client = Client::new();
    if client.connect(&url).await.is_err() {
        eprintln!("connect failed");
        std::process::exit(1);
    }

    let mut latencies_ms = Vec::with_capacity(SAMPLES);
    for i in 0..SAMPLES {
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

    println!(
        "ws-rs roundtrip ({} samples, port {port})",
        latencies_ms.len()
    );
    print_latency("echo_latency", &latencies_ms);

    let tp_port = pick_free_port().await;
    let _tp_server = spawn_echo_server("127.0.0.1", tp_port).await;
    sleep_ms(50).await;

    let tp_url = format!("ws://127.0.0.1:{tp_port}/");
    let blob = vec![0xABu8; THROUGHPUT_BYTES];

    let tp_start = Instant::now();
    let mut tp_client = Client::new();
    if tp_client.connect(&tp_url).await.is_err() {
        eprintln!("throughput connect failed");
        std::process::exit(1);
    }

    let mut tp_iterations = 0i32;
    if tp_client.send_binary(&blob).await.is_ok() {
        for _ in 0..THROUGHPUT_ITERATIONS {
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

    if latencies_ms.len() != SAMPLES {
        std::process::exit(1);
    }
}
