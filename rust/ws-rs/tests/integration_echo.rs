use serial_test::serial;
use std::sync::Arc;

use tokio::sync::Mutex;
use ws_rs::client::Client;
use ws_rs::server::{echo_session, Server};

#[serial]
#[tokio::test]
async fn echo_roundtrip() {
    let mut srv = Server::new();
    srv.listen("127.0.0.1", 0).await.unwrap();
    let addr = srv.local_addr().to_string();
    let port: u16 = addr.split(':').next_back().unwrap().parse().unwrap();
    let stop = Arc::new(Mutex::new(false));
    let stop_srv = stop.clone();

    let server_task = tokio::spawn(async move {
        loop {
            if *stop_srv.lock().await {
                break;
            }
            let accept = srv.listener().unwrap().accept();
            let (stream, _) = tokio::select! {
                res = accept => match res {
                    Ok(v) => v,
                    Err(_) => break,
                },
                _ = tokio::time::sleep(std::time::Duration::from_millis(50)) => continue,
            };
            let conn = match Server::complete_handshake(stream, None).await {
                Ok(c) => Arc::new(c),
                Err(_) => continue,
            };
            tokio::spawn(async move {
                let _ = echo_session(conn).await;
            });
        }
    });

    tokio::time::sleep(std::time::Duration::from_millis(50)).await;

    let url = format!("ws://127.0.0.1:{port}/");
    let mut client = Client::new();
    client.connect(&url).await.unwrap();
    client.send_text("ping").await.unwrap();
    let reply = client.recv_text().await.unwrap().unwrap();
    assert_eq!(reply, "ping");
    client.close().await.unwrap();

    *stop.lock().await = true;
    server_task.abort();
}
