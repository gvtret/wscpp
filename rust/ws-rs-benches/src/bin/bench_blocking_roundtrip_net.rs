// bench_blocking_roundtrip_net — sync client for remote echo (mirrors C++ linux_socket net bench)

use std::env;
use std::time::Instant;

use ws_rs::blocking::Client;

const THROUGHPUT_BYTES: usize = 64 * 1024;
const THROUGHPUT_ITERATIONS: i32 = 100;

fn usage(argv0: &str) {
    eprintln!("Usage: {argv0} <host> [port] [samples]");
    eprintln!("  Defaults: port=19081 samples=100");
}

fn elapsed_sec(start: Instant, end: Instant) -> f64 {
    end.duration_since(start).as_secs_f64()
}

fn percentile(values: &[f64], p: f64) -> f64 {
    let mut sorted = values.to_vec();
    sorted.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let idx = (p * (sorted.len() as f64 - 1.0)) as usize;
    sorted[idx]
}

fn print_latency(name: &str, ms_samples: &[f64]) {
    let p50 = percentile(ms_samples, 0.50);
    let p99 = percentile(ms_samples, 0.99);
    println!(
        "{name:<24} p50={p50:.3} ms  p99={p99:.3} ms  (n={})",
        ms_samples.len()
    );
}

fn print_throughput(name: &str, bytes: f64, seconds: f64, iterations: i32) {
    let mbps = bytes / (1024.0 * 1024.0) / seconds;
    println!("{name:<24} {mbps:>8.2} MB/s  ({iterations} iter, {seconds:.3} s)");
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        usage(&args[0]);
        std::process::exit(1);
    }

    let host = &args[1];
    let port: u16 = args.get(2).and_then(|s| s.parse().ok()).unwrap_or(19081);
    let samples: usize = args.get(3).and_then(|s| s.parse().ok()).unwrap_or(100);

    println!("=== bench_blocking_roundtrip_net (ws-rs transport: std-blocking) ===");
    println!("target ws://{host}:{port}/ samples={samples}");

    let url = format!("ws://{host}:{port}/");

    let mut client = Client::new();
    if client.connect(&url).is_err() {
        eprintln!("connect failed");
        std::process::exit(1);
    }

    let mut latencies_ms = Vec::with_capacity(samples);
    for i in 0..samples {
        let msg = format!("ping-{i}");
        let start = Instant::now();
        if client.send_text(&msg).is_err() {
            break;
        }
        if client.recv_text().is_err() {
            break;
        }
        latencies_ms.push(elapsed_sec(start, Instant::now()) * 1000.0);
    }
    client.close().ok();

    println!(
        "ws-rs blocking net roundtrip ({} samples)",
        latencies_ms.len()
    );
    print_latency("echo_latency", &latencies_ms);

    let blob = vec![0xABu8; THROUGHPUT_BYTES];
    let tp_start = Instant::now();
    let mut tp_client = Client::new();
    if tp_client.connect(&url).is_err() {
        eprintln!("throughput connect failed");
        std::process::exit(1);
    }

    let mut tp_iterations = 0i32;
    if tp_client.send_binary(&blob).is_ok() {
        for _ in 0..THROUGHPUT_ITERATIONS {
            match tp_client.recv_binary() {
                Ok(Some(data)) if data.len() == THROUGHPUT_BYTES => {
                    tp_iterations += 1;
                    if tp_iterations < THROUGHPUT_ITERATIONS {
                        if tp_client.send_binary(&blob).is_err() {
                            break;
                        }
                    } else {
                        tp_client.close().ok();
                        break;
                    }
                }
                _ => break,
            }
        }
    }

    print_throughput(
        "echo_throughput_64k",
        THROUGHPUT_BYTES as f64 * tp_iterations as f64,
        elapsed_sec(tp_start, Instant::now()),
        tp_iterations,
    );

    if latencies_ms.len() != samples {
        std::process::exit(1);
    }
}
