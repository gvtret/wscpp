// bench_fastwebsockets_roundtrip — echo latency + 64 KiB throughput (localhost)

use std::time::Instant;

use ws_rs_benches::common::{
    pick_free_port, print_compare_banner, print_latency, print_throughput, sleep_ms,
};
use ws_rs_benches::fastwebsockets_bench::{
    connect, recv_data, run_echo_server, send_binary, send_text,
};

const SAMPLES: usize = 100;
const THROUGHPUT_BYTES: usize = 64 * 1024;
const THROUGHPUT_ITERATIONS: i32 = 100;

#[tokio::main]
async fn main() {
    print_compare_banner("bench_fastwebsockets_roundtrip");

    let port = pick_free_port().await;
    tokio::spawn(async move {
        run_echo_server("127.0.0.1", port).await;
    });
    sleep_ms(50).await;

    let mut ws = connect("127.0.0.1", port).await.expect("connect");

    let mut latencies_ms = Vec::with_capacity(SAMPLES);
    for i in 0..SAMPLES {
        let msg = format!("ping-{i}");
        let start = Instant::now();
        send_text(&mut ws, &msg).await.expect("send");
        let _ = recv_data(&mut ws).await.expect("recv");
        latencies_ms.push(ws_rs_benches::common::elapsed_sec(start, Instant::now()) * 1000.0);
    }

    println!(
        "fastwebsockets roundtrip ({} samples, port {port})",
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
    let mut tp_ws = connect("127.0.0.1", tp_port).await.expect("tp connect");

    let mut tp_iterations = 0i32;
    send_binary(&mut tp_ws, &blob).await.expect("tp send");
    loop {
        let data = recv_data(&mut tp_ws).await.expect("tp recv");
        if data.len() == THROUGHPUT_BYTES {
            tp_iterations += 1;
            if tp_iterations < THROUGHPUT_ITERATIONS {
                send_binary(&mut tp_ws, &blob).await.expect("tp send");
            } else {
                break;
            }
        } else {
            break;
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
