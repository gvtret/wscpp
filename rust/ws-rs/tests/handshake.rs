use ws_rs::handshake::{
    build_client_request, build_server_response, compute_accept, generate_key,
    parse_client_upgrade, verify_server_accept,
};

#[test]
fn accept_key_rfc6455_example() {
    let key = "dGhlIHNhbXBsZSBub25jZQ==";
    let accept = compute_accept(key);
    assert_eq!(accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

#[test]
fn client_request_and_server_response() {
    let key = generate_key();
    let req = build_client_request("example.com", "80", "/chat", &key, None);
    assert!(req.starts_with("GET /chat HTTP/1.1"));
    assert!(req.contains("Upgrade: websocket"));
    assert!(req.contains(&format!("Sec-WebSocket-Key: {key}")));

    let accept = compute_accept(&key);
    let resp = build_server_response(&accept, None);
    assert!(resp.starts_with("HTTP/1.1 101"));
    assert!(resp.contains(&format!("Sec-WebSocket-Accept: {accept}")));
}

#[test]
fn request_response_accept_roundtrip() {
    let key = generate_key();
    let req = build_client_request("127.0.0.1", "8080", "/", &key, None);
    let (_, headers) = parse_client_upgrade(&req).unwrap();
    let client_key = headers.get("sec-websocket-key").unwrap();
    let accept = compute_accept(client_key);
    let resp = build_server_response(&accept, None);
    assert!(verify_server_accept(&key, &accept));
    assert!(resp.contains(&accept));
}

#[test]
fn parse_upgrade_request() {
    let key = "dGhlIHNhbXBsZSBub25jZQ==";
    let raw = format!(
        "GET / HTTP/1.1\r\n\
         Host: localhost\r\n\
         Upgrade: websocket\r\n\
         Connection: Upgrade\r\n\
         Sec-WebSocket-Key: {key}\r\n\
         Sec-WebSocket-Version: 13\r\n\
         \r\n"
    );
    let (_, headers) = parse_client_upgrade(&raw).unwrap();
    assert_eq!(
        headers.get("sec-websocket-version").map(String::as_str),
        Some("13")
    );
    let accept = compute_accept(key);
    assert!(verify_server_accept(key, &accept));
}
