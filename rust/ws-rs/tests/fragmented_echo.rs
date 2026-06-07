use serial_test::serial;
use std::sync::Arc;

use ws_rs::client::Client;
use ws_rs::server::{echo_session, Server};

#[serial]
#[tokio::test]
async fn fragmented_text_echo() {
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
        let conn = Server::complete_handshake(stream, None).await.unwrap();
        echo_session(Arc::new(conn)).await.unwrap();
    });

    tokio::time::sleep(std::time::Duration::from_millis(50)).await;

    let mut client = Client::new();
    client
        .connect(&format!("ws://127.0.0.1:{port}/"))
        .await
        .unwrap();
    client.send_text_fragment("Hello ", false).await.unwrap();
    client.send_continuation("World", true).await.unwrap();
    let reply = client.recv_text().await.unwrap().unwrap();
    assert_eq!(reply, "Hello World");
    client.close().await.unwrap();

    server_task.abort();
}
