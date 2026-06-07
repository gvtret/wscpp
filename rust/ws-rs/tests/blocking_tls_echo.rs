use serial_test::serial;
use std::fs;
use std::sync::Arc;
use std::thread;

use rcgen::{generate_simple_self_signed, CertifiedKey};
use ws_rs::blocking::{Client, Server};
use ws_rs::ServerTlsConfig;

fn write_pem_pair(
    dir: &std::path::Path,
    certified: &CertifiedKey,
) -> (std::path::PathBuf, std::path::PathBuf) {
    let cert_path = dir.join("server.crt");
    let key_path = dir.join("server.key");
    fs::write(&cert_path, certified.cert.pem()).unwrap();
    fs::write(&key_path, certified.key_pair.serialize_pem()).unwrap();
    (cert_path, key_path)
}

#[serial]
#[test]
fn blocking_wss_echo_insecure() {
    let certified = generate_simple_self_signed(vec!["127.0.0.1".into()]).unwrap();
    let dir = std::env::temp_dir().join(format!("ws-rs-blocking-tls-{}", std::process::id()));
    fs::create_dir_all(&dir).unwrap();
    let (cert, key) = write_pem_pair(&dir, &certified);

    let tls_cfg = ServerTlsConfig::from_pem(&cert, &key).unwrap();
    let mut srv = Server::new();
    srv.set_tls_config(tls_cfg);
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
    client.connect(&format!("wss://127.0.0.1:{port}/")).unwrap();
    client.send_text("wss-blocking").unwrap();
    let reply = client.recv_text().unwrap().unwrap();
    assert_eq!(reply, "wss-blocking");
    client.close().unwrap();

    server_thread.join().unwrap();
    let _ = fs::remove_dir_all(dir);
}
