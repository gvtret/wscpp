# Comparative benchmarks (F2)

Side-by-side scenarios for C++11 WebSocket libraries. Built when `WSCPP_BUILD_BENCHMARKS=ON`.

**Policy:** publish numbers only after the RFC gate in `ANALYSIS.md` is green (RFC 6455 + UTF-8 §8.1 + basic WSS).

## Included in-tree

| Target | Library | Version | Scenarios |
|--------|---------|---------|-----------|
| `bench_roundtrip` | wscpp | current | echo latency, 64 KiB throughput |
| `bench_websocketpp_roundtrip` | websocketpp | 0.8.2 | echo latency |
| `bench_ixwebsocket_roundtrip` | IXWebSocket | 11.4.6 | echo latency, 64 KiB throughput |
| `bench_libwebsockets_roundtrip` | libwebsockets | 4.3.x | echo latency (pkg-config) |
| `bench_easywsclient_connect` | easywsclient | master | connect latency (client-only) |
| `bench_beast_roundtrip` | Boost.Beast | Boost 1.70+ | echo latency, 64 KiB throughput |
| `bench_sws_roundtrip` | Simple-WebSocket-Server | GitLab `0e1cf67` | echo latency |

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
./build/bin/bench_beast_roundtrip
./build/bin/bench_sws_roundtrip
```

CMake options (default ON when compare is enabled):

| Option | Effect |
|--------|--------|
| `WSCPP_COMPARE_IXWEBSOCKET` | IXWebSocket roundtrip bench |
| `WSCPP_COMPARE_EASYWSCLIENT` | easywsclient connect bench |
| `WSCPP_COMPARE_LIBWEBSOCKETS` | libwebsockets roundtrip bench (needs `libwebsockets-dev`) |
| `WSCPP_COMPARE_BEAST` | Boost.Beast roundtrip bench (needs `libboost-system-dev`) |
| `WSCPP_COMPARE_SWS` | Simple-WebSocket-Server roundtrip bench (FetchContent + standalone ASIO) |

## Notes

- **IXWebSocket:** do not call `WebSocket::stop()` from inside `setOnMessageCallback` (deadlock); stop from the main thread.
- **easywsclient:** URL must not have a trailing slash when the path is empty (`ws://127.0.0.1:PORT`). The bench uses a wscpp acceptor and server-side close after open to avoid connection pile-up.
- **Beast:** sync echo server/client on localhost; server binds port 0 and passes the assigned port to the client.
- **Simple-WebSocket-Server:** pinned GitLab commit with modern standalone ASIO (`ASIO_STANDALONE`); client runs in a worker thread.

Metrics to record: echo latency p50/p99, throughput MB/s, static binary size (`ls -la`), peak RSS (`/usr/bin/time -v`).
