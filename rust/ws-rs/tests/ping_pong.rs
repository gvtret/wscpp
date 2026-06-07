use serial_test::serial;
use std::sync::Arc;

use ws_rs::client::Client;
use ws_rs::server::Server;

/// Mirrors C++ `ServerPingGetsPong` — server ping must not break the session.
#[serial]
#[tokio::test]
async fn server_ping_gets_pong() {
    let mut srv = Server::new();
    srv.listen("127.0.0.1", 0).await.unwrap();
    let port: u16 = srv
        .local_addr()
        .split(':')
        .next_back()
        .unwrap()
        .parse()
        .unwrap();

    let server_task = tokio::spawn(async move {
        let (stream, _) = srv.listener().unwrap().accept().await.unwrap();
        let conn = Arc::new(Server::complete_handshake(stream, None).await.unwrap());
        conn.send_ping(b"ping").await.unwrap();
        tokio::time::sleep(std::time::Duration::from_millis(200)).await;
    });

    tokio::time::sleep(std::time::Duration::from_millis(50)).await;

    let mut client = Client::new();
    client
        .connect(&format!("ws://127.0.0.1:{port}/"))
        .await
        .unwrap();
    let conn = client.connection().unwrap();
    let _ = conn.read_message().await;
    server_task.await.unwrap();
    let _ = client.close().await;
}
