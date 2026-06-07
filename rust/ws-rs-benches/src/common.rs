use std::sync::Arc;
use std::time::{Duration, Instant};

use tokio::net::TcpListener;
use ws_rs::server::{echo_session, Server};

pub const TRANSPORT_LABEL: &str = "tokio";

pub fn print_banner(target: &str) {
    println!("=== {target} (ws-rs transport: {TRANSPORT_LABEL}) ===");
}

pub fn print_compare_banner(target: &str) {
    println!("=== {target} ===");
}

pub fn elapsed_sec(start: Instant, end: Instant) -> f64 {
    end.duration_since(start).as_secs_f64()
}

pub fn percentile(values: &[f64], p: f64) -> f64 {
    if values.is_empty() {
        return 0.0;
    }
    let mut sorted = values.to_vec();
    sorted.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let idx = (p * (sorted.len() as f64 - 1.0)) as usize;
    sorted[idx]
}

pub fn print_throughput(name: &str, bytes: f64, seconds: f64, iterations: i32) {
    let mb = bytes / (1024.0 * 1024.0);
    let mbps = if seconds > 0.0 { mb / seconds } else { 0.0 };
    println!("{name:<24} {mbps:>8.2} MB/s  ({iterations} iter, {seconds:.3} s)");
}

pub fn print_latency(name: &str, ms_samples: &[f64]) {
    if ms_samples.is_empty() {
        println!("{name:<24} (no samples)");
        return;
    }
    let p50 = percentile(ms_samples, 0.50);
    let p99 = percentile(ms_samples, 0.99);
    println!(
        "{name:<24} p50={p50:.3} ms  p99={p99:.3} ms  (n={})",
        ms_samples.len()
    );
}

pub async fn spawn_echo_server(host: impl Into<String>, port: u16) -> tokio::task::JoinHandle<()> {
    let host = host.into();
    tokio::spawn(async move {
        let mut srv = Server::new();
        if srv.listen(&host, port).await.is_err() {
            return;
        }
        loop {
            let accept = srv.listener().unwrap().accept();
            let (stream, _) = match accept.await {
                Ok(v) => v,
                Err(_) => break,
            };
            let conn = match Server::complete_handshake(stream, None).await {
                Ok(c) => Arc::new(c),
                Err(_) => continue,
            };
            tokio::spawn(async move {
                let _ = echo_session(conn).await;
            });
        }
    })
}

pub async fn pick_free_port() -> u16 {
    let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
    listener.local_addr().unwrap().port()
}

pub async fn sleep_ms(ms: u64) {
    tokio::time::sleep(Duration::from_millis(ms)).await;
}
