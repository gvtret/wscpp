// bench_tokio_tungstenite_echo_server — echo server for remote network benchmarks

use std::env;

fn usage(argv0: &str) {
    eprintln!("Usage: {argv0} [port] [bind_host]");
    eprintln!("  Defaults: port=19081 bind=0.0.0.0");
}

#[tokio::main]
async fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() > 1 && (args[1] == "-h" || args[1] == "--help") {
        usage(&args[0]);
        return;
    }

    let port: u16 = args.get(1).and_then(|s| s.parse().ok()).unwrap_or(19081);
    let bind_host = args.get(2).map(String::as_str).unwrap_or("0.0.0.0");

    ws_rs_benches::tungstenite_bench::run_echo_server(bind_host, port).await;
}
