use futures_util::{SinkExt, StreamExt};
use tokio::net::TcpListener;
use tokio_websockets::{Config, Limits, ServerBuilder};

pub async fn run_echo_server(bind_host: &str, port: u16) {
    let addr = format!("{bind_host}:{port}");
    let listener = TcpListener::bind(&addr).await.expect("bind");
    println!("tokio-websockets echo server listening on {addr}");

    loop {
        let (conn, _) = match listener.accept().await {
            Ok(v) => v,
            Err(_) => break,
        };
        tokio::spawn(async move {
            let Ok((_request, mut server)) = ServerBuilder::new()
                .config(Config::default().frame_size(usize::MAX))
                .limits(Limits::unlimited())
                .accept(conn)
                .await
            else {
                return;
            };
            while let Some(item) = server.next().await {
                match item {
                    Ok(msg) if msg.is_text() || msg.is_binary() => {
                        if server.send(msg).await.is_err() {
                            break;
                        }
                    }
                    Ok(_) => {}
                    Err(_) => break,
                }
            }
        });
    }
}
