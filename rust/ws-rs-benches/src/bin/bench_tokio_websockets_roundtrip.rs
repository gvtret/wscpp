// bench_tokio_websockets_roundtrip — echo latency + 64 KiB throughput (localhost)

use std::time::Instant;

use futures_util::{SinkExt, StreamExt};
use http::Uri;
use tokio_websockets::{ClientBuilder, Message};
use ws_rs_benches::common::{
    pick_free_port, print_compare_banner, print_latency, print_throughput, sleep_ms,
};
use ws_rs_benches::tokio_websockets_bench::run_echo_server;

const SAMPLES: usize = 100;
const THROUGHPUT_BYTES: usize = 64 * 1024;
const THROUGHPUT_ITERATIONS: i32 = 100;

async fn connect(
    host: &str,
    port: u16,
) -> impl futures_util::StreamExt<Item = Result<Message, tokio_websockets::Error>>
       + futures_util::SinkExt<Message, Error = tokio_websockets::Error>
       + Unpin {
    let uri: Uri = format!("ws://{host}:{port}/").parse().expect("uri");
    let (client, _) = ClientBuilder::from_uri(uri)
        .connect()
        .await
        .expect("connect");
    client
}

#[tokio::main]
async fn main() {
    print_compare_banner("bench_tokio_websockets_roundtrip");

    let port = pick_free_port().await;
    tokio::spawn(async move {
        run_echo_server("127.0.0.1", port).await;
    });
    sleep_ms(50).await;

    let mut client = connect("127.0.0.1", port).await;

    let mut latencies_ms = Vec::with_capacity(SAMPLES);
    for i in 0..SAMPLES {
        let msg = format!("ping-{i}");
        let start = Instant::now();
        client.send(Message::text(msg)).await.expect("send");
        while let Some(item) = client.next().await {
            let Ok(reply) = item else { break };
            if reply.is_text() || reply.is_binary() {
                break;
            }
        }
        latencies_ms.push(ws_rs_benches::common::elapsed_sec(start, Instant::now()) * 1000.0);
    }

    println!(
        "tokio-websockets roundtrip ({} samples, port {port})",
        latencies_ms.len()
    );
    print_latency("echo_latency", &latencies_ms);

    let tp_port = pick_free_port().await;
    tokio::spawn(async move {
        run_echo_server("127.0.0.1", tp_port).await;
    });
    sleep_ms(50).await;

    let blob = vec![0xABu8; THROUGHPUT_BYTES];
    let tp_start = Instant::now();
    let mut tp_client = connect("127.0.0.1", tp_port).await;

    let mut tp_iterations = 0i32;
    tp_client
        .send(Message::binary(blob.clone()))
        .await
        .expect("tp send");
    loop {
        match tp_client.next().await {
            Some(Ok(msg)) if msg.is_binary() && msg.as_payload().len() == THROUGHPUT_BYTES => {
                tp_iterations += 1;
                if tp_iterations < THROUGHPUT_ITERATIONS {
                    tp_client
                        .send(Message::binary(blob.clone()))
                        .await
                        .expect("tp send");
                } else {
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
