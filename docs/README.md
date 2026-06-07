# wscpp — User Guide

Lightweight WebSocket client/server library for C++11 with optional TLS (`wss://`).

## Requirements

- C++11 compiler (GCC 4.8+, Clang 3.3+, MSVC 2015+)
- CMake 3.10+
- OpenSSL 1.0.1+
- Internet access on first configure (ASIO fetched via CMake FetchContent)

## Build and install

```bash
git clone https://gitverse.ru/project/wscpp.git
cd wscpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build   # optional
```

Options:

| Option | Default | Description |
|--------|---------|-------------|
| `WSCPP_BUILD_TESTS` | ON | Unit, integration, regression tests |
| `WSCPP_BUILD_EXAMPLES` | ON | Example programs |
| `WSCPP_BUILD_DOCS` | ON | Doxygen API reference |
| `WSCPP_ENABLE_STRESS_TESTS` | OFF | Heavy integration stress tests |

Run tests:

```bash
cd build && ctest --output-on-failure
```

## Quick start

### Server (ws://)

```cpp
#include <wscpp/server.hpp>

wscpp::server srv;
srv.set_on_connection([](std::shared_ptr<wscpp::connection> conn) {
    conn->set_on_message([conn](const std::vector<uint8_t>& data, wscpp::frame::opcode) {
        conn->send_text(std::string(data.begin(), data.end()));
    });
});
srv.listen(8080);
srv.start();
srv.join();
```

### Client (ws://)

```cpp
#include <wscpp/client.hpp>

wscpp::client cli;
cli.set_on_open([&]() { cli.send_text("hello"); });
cli.set_on_message([&](const std::vector<uint8_t>& data, wscpp::frame::opcode) {
    // handle message
    cli.close();
});
cli.connect("ws://127.0.0.1:8080/");
cli.run();
```

See [examples/README.md](../examples/README.md) for runnable programs.

## API overview

### `wscpp::client`

- `connect(url)` — `ws://` or `wss://` URL
- `send_text` / `send_binary` — send WebSocket frames
- `close(code, reason)` — graceful close
- Callbacks: `set_on_open`, `set_on_message`, `set_on_close`, `set_on_error`
- `run()` — blocking event loop

### `wscpp::server`

- `listen(port)` / `listen(host, port)`
- `set_ssl_context(shared_ptr<asio::ssl::context>)` — enable WSS
- `set_on_connection` — per-connection setup (set handlers, then server calls `activate()`)
- `start()` / `join()` / `stop()`

### `wscpp::connection`

Lower-level connection used by server callbacks: same send/close/callback API as client.

## ws:// vs wss://

| Scheme | Transport | Notes |
|--------|-----------|-------|
| `ws://` | Plain TCP | Local development, trusted networks |
| `wss://` | TLS over TCP | Production; client sets SNI hostname automatically |

For WSS server, load certificate and key into an `asio::ssl::context` and pass it to `server::set_ssl_context`. The TLS handshake runs before the WebSocket upgrade.

The client uses `verify_none` by default (suitable for self-signed certs in examples). For production, configure certificate verification via OpenSSL/ASIO on the client SSL context.

## FAQ

**Does wscpp support C++11 only?**  
Yes. The library targets C++11 without requiring newer standards.

**Does it depend on Boost?**  
No. ASIO is fetched as standalone ASIO 1.20.

**Is permessage-deflate supported?**  
Not in v1.0 (planned post-v1.0, RFC 7692).

**Can I use wscpp from multiple threads?**  
Each `client`/`server` owns an `io_context`. The connection reader runs in a background thread; callbacks may be invoked from that thread.

**How do I send ping frames?**  
Use `wscpp::frame::builder::build_ping` at the frame layer, or send application-level heartbeats via text messages.

**Why does my server handler never receive messages?**  
Install per-connection handlers inside `set_on_connection`. The server activates the reader after your callback returns.

**How do I bump the library version?**  
Run `./scripts/bump_version.sh patch|minor|major` and reconfigure CMake.

**Where is the API reference?**  
Build docs: `cmake --build build --target docs`, then open `build/docs/html/index.html`.

## Links

- [Developer guide](DEVELOPER.md)
- [Examples](../examples/README.md)
- [Comparative analysis](../ANALYSIS.md)
- [Changelog](../CHANGELOG.md)
