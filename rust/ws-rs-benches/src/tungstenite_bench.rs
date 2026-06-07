use futures_util::{SinkExt, StreamExt};
use tokio::net::TcpListener;
use tokio_tungstenite::accept_async;

pub async fn run_echo_server(bind_host: &str, port: u16) {
    let addr = format!("{bind_host}:{port}");
    let listener = TcpListener::bind(&addr).await.expect("bind");
    println!("tokio-tungstenite echo server listening on {addr}");

    loop {
        let (stream, _) = match listener.accept().await {
            Ok(v) => v,
            Err(_) => break,
        };
        tokio::spawn(async move {
            let Ok(mut ws) = accept_async(stream).await else {
                return;
            };
            while let Some(msg) = ws.next().await {
                match msg {
                    Ok(m) => {
                        if ws.send(m).await.is_err() {
                            break;
                        }
                    }
                    Err(_) => break,
                }
            }
        });
    }
}
