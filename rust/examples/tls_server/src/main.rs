use std::env;
use std::sync::Arc;

use ws_rs::server::{echo_session, Server};
use ws_rs::ServerTlsConfig;

fn usage(argv0: &str) {
    eprintln!("Usage: {argv0} --cert <path> --key <path> [--port PORT] [--host HOST]");
}

#[tokio::main]
async fn main() -> ws_rs::Result<()> {
    let args: Vec<String> = env::args().collect();
    let mut cert = String::new();
    let mut key = String::new();
    let mut port: u16 = 8443;
    let mut host = "127.0.0.1".to_string();

    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "--cert" => {
                i += 1;
                cert = args.get(i).cloned().unwrap_or_default();
            }
            "--key" => {
                i += 1;
                key = args.get(i).cloned().unwrap_or_default();
            }
            "--port" => {
                i += 1;
                port = args.get(i).and_then(|s| s.parse().ok()).unwrap_or(8443);
            }
            "--host" => {
                i += 1;
                host = args.get(i).cloned().unwrap_or(host);
            }
            "-h" | "--help" => {
                usage(&args[0]);
                return Ok(());
            }
            other => {
                eprintln!("unknown arg: {other}");
                usage(&args[0]);
                std::process::exit(1);
            }
        }
        i += 1;
    }

    if cert.is_empty() || key.is_empty() {
        usage(&args[0]);
        std::process::exit(1);
    }

    let tls = ServerTlsConfig::from_pem(&cert, &key)?;
    let mut server = Server::new();
    server.set_tls_config(tls.clone());
    server.listen(&host, port).await?;
    eprintln!("wss echo server wss://{host}:{port}/");

    loop {
        let (stream, _) = server.listener()?.accept().await?;
        let conn = match Server::complete_handshake(stream, Some(&tls)).await {
            Ok(c) => Arc::new(c),
            Err(e) => {
                eprintln!("handshake error: {e}");
                continue;
            }
        };
        tokio::spawn(async move {
            let _ = echo_session(conn).await;
        });
    }
}
