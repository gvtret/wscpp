use serial_test::serial;
use std::fs;
use std::sync::Arc;

use rcgen::{generate_simple_self_signed, CertifiedKey};
use ws_rs::client::Client;
use ws_rs::server::{echo_session, Server};
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
#[tokio::test]
async fn wss_echo_insecure_client() {
    let certified = generate_simple_self_signed(vec!["localhost".into()]).unwrap();
    let dir = std::env::temp_dir().join(format!("ws-rs-tls-{}", std::process::id()));
    fs::create_dir_all(&dir).unwrap();
    let (cert, key) = write_pem_pair(&dir, &certified);

    let tls_cfg = ServerTlsConfig::from_pem(&cert, &key).unwrap();
    let mut srv = Server::new();
    srv.set_tls_config(tls_cfg.clone());
    srv.listen("127.0.0.1", 0).await.unwrap();
    let port: u16 = srv
        .local_addr()
        .split(':')
        .next_back()
        .unwrap()
        .parse()
        .unwrap();
    let tls_handshake = tls_cfg.clone();

    let server = tokio::spawn(async move {
        loop {
            let accept = srv.listener().unwrap().accept();
            let (stream, _) = match accept.await {
                Ok(v) => v,
                Err(_) => break,
            };
            let conn = match Server::complete_handshake(stream, Some(&tls_handshake)).await {
                Ok(c) => Arc::new(c),
                Err(_) => continue,
            };
            tokio::spawn(async move {
                let _ = echo_session(conn).await;
            });
        }
    });

    tokio::time::sleep(std::time::Duration::from_millis(50)).await;

    let url = format!("wss://127.0.0.1:{port}/");
    let mut client = Client::new();
    client.connect(&url).await.unwrap();
    assert!(client.is_ssl().await);
    client.send_text("wss-ping").await.unwrap();
    let reply = client.recv_text().await.unwrap().unwrap();
    assert_eq!(reply, "wss-ping");
    client.close().await.unwrap();

    server.abort();
    let _ = fs::remove_dir_all(dir);
}

#[serial]
#[tokio::test]
async fn wss_echo_default_insecure() {
    let certified = generate_simple_self_signed(vec!["127.0.0.1".into()]).unwrap();
    let dir = std::env::temp_dir().join(format!("ws-rs-tls2-{}", std::process::id()));
    fs::create_dir_all(&dir).unwrap();
    let (cert, key) = write_pem_pair(&dir, &certified);

    let tls_cfg = ServerTlsConfig::from_pem(&cert, &key).unwrap();
    let mut srv = Server::new();
    srv.set_tls_config(tls_cfg.clone());
    srv.listen("127.0.0.1", 0).await.unwrap();
    let port: u16 = srv
        .local_addr()
        .split(':')
        .next_back()
        .unwrap()
        .parse()
        .unwrap();
    let tls_handshake = tls_cfg;

    tokio::spawn(async move {
        loop {
            let (stream, _) = match srv.listener().unwrap().accept().await {
                Ok(v) => v,
                Err(_) => break,
            };
            let conn = match Server::complete_handshake(stream, Some(&tls_handshake)).await {
                Ok(c) => Arc::new(c),
                Err(_) => continue,
            };
            tokio::spawn(async move {
                let _ = echo_session(conn).await;
            });
        }
    });

    tokio::time::sleep(std::time::Duration::from_millis(50)).await;

    let mut client = Client::new();
    client
        .connect(&format!("wss://127.0.0.1:{port}/"))
        .await
        .unwrap();
    client.send_text("verified").await.unwrap();
    assert_eq!(client.recv_text().await.unwrap().unwrap(), "verified");

    let _ = fs::remove_dir_all(dir);
}
