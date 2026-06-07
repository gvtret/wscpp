// bench_blocking_echo_server — sync echo server for LAN benchmarks (mirrors C++ linux_socket server)

use std::env;
use std::sync::Arc;
use std::thread;

use ws_rs::blocking::server::echo_session;
use ws_rs::blocking::Server;

fn usage(argv0: &str) {
    eprintln!("Usage: {argv0} [port] [bind_host]");
    eprintln!("  Defaults: port=19081 bind=0.0.0.0");
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() > 1 && (args[1] == "-h" || args[1] == "--help") {
        usage(&args[0]);
        return;
    }

    let port: u16 = args.get(1).and_then(|s| s.parse().ok()).unwrap_or(19081);
    let bind_host = args.get(2).map(String::as_str).unwrap_or("0.0.0.0");

    let mut server = Server::new();
    if server.listen(bind_host, port).is_err() {
        eprintln!("listen {bind_host}:{port} failed");
        std::process::exit(1);
    }

    println!("ws-rs blocking echo server listening on {bind_host}:{port}");

    loop {
        let Ok(conn) = server.accept_handshake() else {
            break;
        };
        let conn = Arc::new(conn);
        thread::spawn(move || {
            let _ = echo_session(conn);
        });
    }
}
