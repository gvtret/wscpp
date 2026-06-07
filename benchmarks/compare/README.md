# Comparative benchmarks (F2)

Side-by-side scenarios for C++11 WebSocket libraries. Built when `WSCPP_BUILD_BENCHMARKS=ON`.

**Policy:** publish numbers only after the RFC gate in `ANALYSIS.md` is green (RFC 6455 + UTF-8 §8.1 + basic WSS).

## Included in-tree (FetchContent)

| Target | Library | Version | Scenarios |
|--------|---------|---------|-----------|
| `bench_roundtrip` | wscpp | current | echo latency, 64 KiB throughput |
| `bench_websocketpp_roundtrip` | websocketpp | 0.8.2 | echo latency |
| `bench_ixwebsocket_roundtrip` | IXWebSocket | 11.4.6 | echo latency, 64 KiB throughput |
| `bench_libwebsockets_roundtrip` | libwebsockets | 4.3.x | echo latency (pkg-config) |
| `bench_easywsclient_connect` | easywsclient | master | connect latency (client-only) |

Build all:

```bash
cmake -B build -DWSCPP_BUILD_BENCHMARKS=ON
cmake --build build --target benchmarks compare_benchmarks
./build/bin/bench_frame_parse
./build/bin/bench_masking
./build/bin/bench_roundtrip
./build/bin/bench_websocketpp_roundtrip
./build/bin/bench_ixwebsocket_roundtrip
./build/bin/bench_libwebsockets_roundtrip
./build/bin/bench_easywsclient_connect
```

CMake options (default ON when compare is enabled):

| Option | Effect |
|--------|--------|
| `WSCPP_COMPARE_IXWEBSOCKET` | IXWebSocket roundtrip bench |
| `WSCPP_COMPARE_EASYWSCLIENT` | easywsclient connect bench |
| `WSCPP_COMPARE_LIBWEBSOCKETS` | libwebsockets roundtrip bench (needs `libwebsockets-dev`) |

## External libraries (manual setup)

These require system packages or vendored deps; add locally following the same echo/latency pattern:

| Library | Pin | Notes |
|---------|-----|-------|
| Boost.Beast | Boost 1.83+ | Needs Boost.Asio + OpenSSL |
| Simple-WebSocket-Server | tag | ASIO + OpenSSL |

Metrics to record: echo latency p50/p99, throughput MB/s, static binary size (`ls -la`), peak RSS (`/usr/bin/time -v`).

## Notes

- **IXWebSocket:** do not call `WebSocket::stop()` from inside `setOnMessageCallback` (deadlock); stop from the main thread.
- **easywsclient:** URL must not have a trailing slash when the path is empty (`ws://127.0.0.1:PORT`). The bench uses a wscpp acceptor and server-side close after open to avoid connection pile-up.
