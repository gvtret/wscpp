use serial_test::serial;
use std::sync::Arc;

use tokio::sync::oneshot;

use ws_rs::client::Client;
use ws_rs::frame::{FrameBuilder, Opcode};
use ws_rs::server::Server;

fn parse_close_code(payload: &[u8]) -> u16 {
    if payload.len() < 2 {
        return 1000;
    }
    u16::from_be_bytes([payload[0], payload[1]])
}

#[serial]
#[tokio::test]
async fn invalid_utf8_text_closes_with_1007() {
    let mut srv = Server::new();
    srv.listen("127.0.0.1", 0).await.unwrap();
    let port: u16 = srv
        .local_addr()
        .split(':')
        .next_back()
        .unwrap()
        .parse()
        .unwrap();

    let (client_ready_tx, client_ready_rx) = oneshot::channel::<()>();

    let server_task = tokio::spawn(async move {
        let (stream, _) = srv.listener().unwrap().accept().await.unwrap();
        let conn = Arc::new(Server::complete_handshake(stream, None).await.unwrap());

        client_ready_rx.await.unwrap();

        let invalid = [0xC0u8, 0x80];
        let frame = FrameBuilder::build_opcode(Opcode::Text, &invalid, true, false);
        conn.send_raw(&frame).await.unwrap();

        let close_frame = conn.read_frame().await.unwrap().expect("close frame");
        assert_eq!(close_frame.header.opcode, Opcode::Close);
        parse_close_code(&close_frame.payload)
    });

    let client_task = tokio::spawn(async move {
        let mut client = Client::new();
        client
            .connect(&format!("ws://127.0.0.1:{port}/"))
            .await
            .unwrap();
        client_ready_tx.send(()).ok();

        let conn = client.connection().unwrap();
        loop {
            let Some(frame) = conn.read_frame().await.unwrap() else {
                break;
            };
            if !conn
                .handle_incoming(frame.header.opcode, &frame.payload)
                .await
                .unwrap()
            {
                break;
            }
        }
    });

    let (close_code, client_res) = tokio::join!(server_task, client_task);
    client_res.unwrap();
    assert_eq!(close_code.unwrap(), 1007);
}
