use serial_test::serial;
use std::sync::Arc;

use ws_rs::client::Client;
use ws_rs::server::{echo_session, Server};

async fn pick_free_port() -> u16 {
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    listener.local_addr().unwrap().port()
}

#[serial]
#[tokio::test]
async fn deflate_text_echo() {
    let port = pick_free_port().await;
    let bind = format!("127.0.0.1:{port}");

    let server_task = tokio::spawn(async move {
        let listener = tokio::net::TcpListener::bind(&bind).await.unwrap();
        let (stream, _) = listener.accept().await.unwrap();
        let conn = Server::complete_handshake(stream, None).await.unwrap();
        echo_session(Arc::new(conn)).await.unwrap();
    });

    tokio::time::sleep(std::time::Duration::from_millis(50)).await;

    let mut client = Client::new();
    client.enable_permessage_deflate(true);
    client
        .connect(&format!("ws://127.0.0.1:{port}/"))
        .await
        .unwrap();
    client.send_text("Hello compressed").await.unwrap();
    let reply = client.recv_text().await.unwrap().unwrap();
    assert_eq!(reply, "Hello compressed");
    client.close().await.unwrap();

    server_task.abort();
}
