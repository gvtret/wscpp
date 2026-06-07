# Examples

WebSocket echo examples for **wscpp**.

## Build

```bash
cmake -B build -DWSCPP_BUILD_EXAMPLES=ON
cmake --build build
```

Binaries: `build/bin/wscpp_example_client`, `wscpp_example_server`, `wscpp_example_tls_client`, `wscpp_example_tls_server`.

## ws:// echo (local)

Terminal 1 — start server:

```bash
./build/bin/wscpp_example_server 8080
```

Terminal 2 — connect client:

```bash
./build/bin/wscpp_example_client ws://127.0.0.1:8080/
```

The server echoes each text message with an `Echo:` prefix.

## wss:// echo (TLS)

Generate a self-signed certificate (once):

```bash
openssl req -x509 -newkey rsa:2048 \
  -keyout key.pem -out cert.pem -days 365 -nodes \
  -subj "/CN=localhost"
```

Terminal 1 — TLS server:

```bash
./build/bin/wscpp_example_tls_server --port 8443 --cert cert.pem --key key.pem
```

Terminal 2 — TLS client (self-signed cert; verification disabled in client):

```bash
./build/bin/wscpp_example_tls_client wss://127.0.0.1:8443/
```

## Public echo service

```bash
./build/bin/wscpp_example_client ws://echo.websocket.org/
```

Note: external services may rate-limit or change behaviour; prefer local examples for development.
