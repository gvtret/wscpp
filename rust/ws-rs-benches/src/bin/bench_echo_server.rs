// bench_echo_server — WebSocket echo server for remote network benchmarks

use std::env;
use std::sync::Arc;

use ws_rs::server::{echo_session, Server};

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

    let mut server = Server::new();
    if server.listen(bind_host, port).await.is_err() {
        eprintln!("listen {bind_host}:{port} failed");
        std::process::exit(1);
    }

    println!("ws-rs echo server listening on {bind_host}:{port}");

    loop {
        let accept = server.listener().unwrap().accept();
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
}
