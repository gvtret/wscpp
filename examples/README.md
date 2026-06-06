# Examples

This directory contains example implementations of wscpp client and server.

## Building examples

```bash
cmake -DWSCPP_BUILD_EXAMPLES=ON ..
make
```

## Running examples

### Client example

```bash
./bin/wscpp_example_client ws://echo.websocket.org
```

### Server example

```bash
./bin/wscpp_example_server 8080
```

Then connect to `ws://localhost:8080` from a WebSocket client.
