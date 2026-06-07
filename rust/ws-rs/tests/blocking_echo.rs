use serial_test::serial;
use std::sync::Arc;
use std::thread;

use ws_rs::blocking::{Client, Server};

#[serial]
#[test]
fn blocking_echo_roundtrip() {
    let mut srv = Server::new();
    srv.listen("127.0.0.1", 0).unwrap();
    let port: u16 = srv
        .local_addr()
        .split(':')
        .next_back()
        .unwrap()
        .parse()
        .unwrap();

    let server_thread = thread::spawn(move || {
        let conn = Arc::new(srv.accept_handshake().unwrap());
        let conn2 = Arc::clone(&conn);
        let echo = thread::spawn(move || {
            while conn2.is_open() {
                let Ok(Some(msg)) = conn2.read_message() else {
                    break;
                };
                if msg.opcode == ws_rs::frame::Opcode::Text {
                    let text = std::str::from_utf8(&msg.payload).unwrap();
                    conn2.send_text(text, true).unwrap();
                }
            }
        });
        let _ = echo.join();
    });

    thread::sleep(std::time::Duration::from_millis(50));

    let mut client = Client::new();
    client.connect(&format!("ws://127.0.0.1:{port}/")).unwrap();
    client.send_text("ping-42").unwrap();
    let reply = client.recv_text().unwrap().unwrap();
    assert_eq!(reply, "ping-42");
    client.close().unwrap();

    server_thread.join().unwrap();
}
