# Comparative benchmarks (F2)

Side-by-side scenarios for **`C++11`** WebSocket libraries. Built when `WSCPP_BUILD_BENCHMARKS=ON`.

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

### LAN compare (client local, echo server remote)

| Server target | Client target | Library |
|---------------|---------------|---------|
| `bench_echo_server` | `bench_roundtrip_net` | wscpp |
| `bench_wscpp_connect_server` | `bench_easywsclient_connect_net` | easywsclient connect |
| `bench_websocketpp_echo_server` | `bench_websocketpp_roundtrip_net` | websocketpp |
| `bench_ixwebsocket_echo_server` | `bench_ixwebsocket_roundtrip_net` | IXWebSocket |
| `bench_libwebsockets_echo_server` | `bench_libwebsockets_roundtrip_net` | libwebsockets |
| `bench_beast_echo_server` | `bench_beast_roundtrip_net` | Boost.Beast |
| `bench_sws_echo_server` | `bench_sws_roundtrip_net` | Simple-WebSocket-Server |

Run all: `bash benchmarks/run_remote_network_compare.sh` (see parent `benchmarks/README.md`).

Build and run all:

```bash
cmake -B build -DWSCPP_BUILD_BENCHMARKS=ON
cmake --build build --target benchmarks compare_benchmarks
cmake --build build --target run_benchmarks
```

Manual run:

```bash
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
| `WSCPP_USE_ASIO` | wscpp transport for `bench_roundtrip` / easywsclient server half |

## Notes

- **IXWebSocket:** do not call `WebSocket::stop()` from inside `setOnMessageCallback` (deadlock); stop from the main thread.
- **easywsclient:** URL must not have a trailing slash when the path is empty (`ws://127.0.0.1:PORT`). The bench uses a wscpp acceptor and server-side close after open to avoid connection pile-up.
- **Beast:** sync echo server/client on localhost; server binds port 0 and passes the assigned port to the client.
- **Simple-WebSocket-Server:** pinned GitLab commit with modern standalone ASIO (`ASIO_STANDALONE`); client runs in a worker thread.
- **Harness:** `bench_util.hpp` provides `pick_free_port()` and consistent banners; `run_benchmarks.sh` prints sizes after all runs.

Metrics to record: echo latency p50/p99, throughput MB/s, static binary size (`run_benchmarks.sh` footer), peak RSS (`/usr/bin/time -v`).
