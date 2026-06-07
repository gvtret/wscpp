// bench_tokio_tungstenite_roundtrip — echo latency + 64 KiB throughput (mirrors bench_ixwebsocket_roundtrip)

use std::time::Instant;

use futures_util::{SinkExt, StreamExt};
use tokio_tungstenite::{connect_async, tungstenite::Message};
use ws_rs_benches::common::{
    pick_free_port, print_compare_banner, print_latency, print_throughput, sleep_ms,
};

const SAMPLES: usize = 100;
const THROUGHPUT_BYTES: usize = 64 * 1024;
const THROUGHPUT_ITERATIONS: i32 = 100;

async fn spawn_tungstenite_echo_server(port: u16) -> tokio::task::JoinHandle<()> {
    tokio::spawn(async move {
        ws_rs_benches::tungstenite_bench::run_echo_server("127.0.0.1", port).await;
    })
}

#[tokio::main]
async fn main() {
    print_compare_banner("bench_tokio_tungstenite_roundtrip");

    let port = pick_free_port().await;
    let _server = spawn_tungstenite_echo_server(port).await;
    sleep_ms(50).await;

    let url = format!("ws://127.0.0.1:{port}/");
    let (mut ws, _) = connect_async(&url).await.expect("connect");

    let mut latencies_ms = Vec::with_capacity(SAMPLES);
    for i in 0..SAMPLES {
        let msg = format!("ping-{i}");
        let start = Instant::now();
        ws.send(Message::Text(msg)).await.expect("send");
        let _ = ws.next().await.expect("recv");
        latencies_ms.push(ws_rs_benches::common::elapsed_sec(start, Instant::now()) * 1000.0);
    }
    let _ = ws.close(None).await;

    println!(
        "tokio-tungstenite roundtrip ({} samples, port {port})",
        latencies_ms.len()
    );
    print_latency("echo_latency", &latencies_ms);

    let tp_port = pick_free_port().await;
    let _tp_server = spawn_tungstenite_echo_server(tp_port).await;
    sleep_ms(50).await;

    let tp_url = format!("ws://127.0.0.1:{tp_port}/");
    let blob = vec![0xABu8; THROUGHPUT_BYTES];

    let tp_start = Instant::now();
    let (mut tp_ws, _) = connect_async(&tp_url).await.expect("tp connect");

    let mut tp_iterations = 0i32;
    tp_ws
        .send(Message::Binary(blob.clone()))
        .await
        .expect("tp send");
    loop {
        match tp_ws.next().await {
            Some(Ok(Message::Binary(data))) if data.len() == THROUGHPUT_BYTES => {
                tp_iterations += 1;
                if tp_iterations < THROUGHPUT_ITERATIONS {
                    tp_ws
                        .send(Message::Binary(blob.clone()))
                        .await
                        .expect("tp send");
                } else {
                    let _ = tp_ws.close(None).await;
                    break;
                }
            }
            _ => break,
        }
    }
    let tp_end = Instant::now();

    print_throughput(
        "echo_throughput_64k",
        THROUGHPUT_BYTES as f64 * tp_iterations as f64,
        ws_rs_benches::common::elapsed_sec(tp_start, tp_end),
        tp_iterations,
    );
}
