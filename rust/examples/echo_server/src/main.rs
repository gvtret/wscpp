use std::env;
use std::sync::Arc;

use tokio::sync::Mutex;
use ws_rs::server::{echo_session, Server};

#[tokio::main]
async fn main() -> ws_rs::Result<()> {
    let port: u16 = env::args()
        .nth(1)
        .and_then(|s| s.parse().ok())
        .unwrap_or(8080);
    let host = env::args()
        .nth(2)
        .unwrap_or_else(|| "127.0.0.1".to_string());

    let mut server = Server::new();
    server.listen(&host, port).await?;
    eprintln!("echo server ws://{host}:{port}/");

    let stop = Arc::new(Mutex::new(false));
    loop {
        if *stop.lock().await {
            break;
        }
        let accept = server.listener()?.accept();
        let (stream, _) = tokio::select! {
            res = accept => res?,
            _ = tokio::time::sleep(std::time::Duration::from_millis(100)) => continue,
        };
        let conn = match Server::complete_handshake(stream, None).await {
            Ok(c) => Arc::new(c),
            Err(e) => {
                eprintln!("handshake error: {e}");
                continue;
            }
        };
        tokio::spawn(async move {
            if let Err(e) = echo_session(conn).await {
                eprintln!("session error: {e}");
            }
        });
    }
    Ok(())
}
