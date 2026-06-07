use serial_test::serial;
use std::sync::Arc;
use std::time::Duration;

use tokio::sync::Barrier;
use ws_rs::client::Client;
use ws_rs::server::{echo_session, Server};

async fn start_echo_server(port: u16) -> tokio::task::JoinHandle<()> {
    let mut srv = Server::new();
    srv.listen("127.0.0.1", port).await.unwrap();
    tokio::spawn(async move {
        loop {
            let Ok((stream, _)) = srv.listener().unwrap().accept().await else {
                break;
            };
            let Ok(conn) = Server::complete_handshake(stream, None).await else {
                continue;
            };
            tokio::spawn(async move {
                let _ = echo_session(Arc::new(conn)).await;
            });
        }
    })
}

fn pick_port() -> u16 {
    let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
    listener.local_addr().unwrap().port()
}

#[serial]
#[tokio::test]
async fn parallel_clients_echo() {
    let port = pick_port();
    let _server = start_echo_server(port).await;
    tokio::time::sleep(Duration::from_millis(50)).await;

    const CLIENT_COUNT: usize = 10;
    let barrier = Arc::new(Barrier::new(CLIENT_COUNT));
    let mut handles = Vec::new();

    for i in 0..CLIENT_COUNT {
        let barrier = Arc::clone(&barrier);
        handles.push(tokio::spawn(async move {
            barrier.wait().await;
            let mut client = Client::new();
            let url = format!("ws://127.0.0.1:{port}/");
            client.connect(&url).await.unwrap();
            let msg = format!("client-{i}");
            client.send_text(&msg).await.unwrap();
            let reply = client.recv_text().await.unwrap().unwrap();
            assert_eq!(reply, msg);
            let _ = client.close().await;
        }));
    }

    for handle in handles {
        handle.await.unwrap();
    }
}

#[serial]
#[tokio::test]
async fn large_binary_message() {
    let port = pick_port();
    let _server = start_echo_server(port).await;
    tokio::time::sleep(Duration::from_millis(50)).await;

    let payload: Vec<u8> = (0..1024 * 1024).map(|i| (i & 0xFF) as u8).collect();

    let mut client = Client::new();
    client
        .connect(&format!("ws://127.0.0.1:{port}/"))
        .await
        .unwrap();
    client.send_binary(&payload).await.unwrap();
    let reply = client.recv_binary().await.unwrap().unwrap();
    assert_eq!(reply.len(), payload.len());
    assert_eq!(reply, payload);
    let _ = client.close().await;
}

#[serial]
#[tokio::test]
async fn rapid_small_messages() {
    let port = pick_port();
    let _server = start_echo_server(port).await;
    tokio::time::sleep(Duration::from_millis(50)).await;

    const MESSAGE_COUNT: i32 = 100;

    let mut client = Client::new();
    client
        .connect(&format!("ws://127.0.0.1:{port}/"))
        .await
        .unwrap();
    client.send_text("start").await.unwrap();

    for i in 0..MESSAGE_COUNT {
        let reply = client.recv_text().await.unwrap().unwrap();
        if i == 0 {
            assert_eq!(reply, "start");
        } else {
            assert_eq!(reply, format!("msg-{i}"));
        }
        if i + 1 < MESSAGE_COUNT {
            client.send_text(&format!("msg-{}", i + 1)).await.unwrap();
        }
    }
    let _ = client.close().await;
}
