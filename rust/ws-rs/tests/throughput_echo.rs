use serial_test::serial;
use std::sync::Arc;

use ws_rs::client::Client;
use ws_rs::server::{echo_session, Server};

#[serial]
#[tokio::test]
async fn binary_64k_echo() {
    let mut srv = Server::new();
    srv.listen("127.0.0.1", 0).await.unwrap();
    let port: u16 = srv
        .local_addr()
        .split(':')
        .next_back()
        .unwrap()
        .parse()
        .unwrap();

    let server = tokio::spawn(async move {
        let (stream, _) = srv.listener().unwrap().accept().await.unwrap();
        let conn = Arc::new(Server::complete_handshake(stream, None).await.unwrap());
        echo_session(conn).await.unwrap();
    });

    let url = format!("ws://127.0.0.1:{port}/");
    let mut client = Client::new();
    client.connect(&url).await.unwrap();
    let blob = vec![0xABu8; 64 * 1024];
    client.send_binary(&blob).await.unwrap();
    let data = client.recv_binary().await.unwrap().expect("binary frame");
    assert_eq!(data.len(), blob.len());
    client.close().await.unwrap();
    server.abort();
}
