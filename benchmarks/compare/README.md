# Comparative benchmarks (F2)

Side-by-side scenarios for C++11 WebSocket libraries. Built when `WSCPP_BUILD_BENCHMARKS=ON`.

**Policy:** use for development smoke tests until wscpp passes the RFC gate in `ANALYSIS.md`. Re-run and update published tables after RFC 6455/2818 coverage is complete.

## Included in-tree

| Target | Library | Version | Scenarios |
|--------|---------|---------|-----------|
| `bench_roundtrip` | wscpp | current | echo latency, 64 KiB throughput |
| `bench_websocketpp_roundtrip` | websocketpp | 0.8.2 | echo latency |

Run all:

```bash
cmake -B build -DWSCPP_BUILD_BENCHMARKS=ON
cmake --build build --target benchmarks bench_websocketpp_roundtrip
./build/bin/bench_frame_parse
./build/bin/bench_masking
./build/bin/bench_roundtrip
./build/bin/bench_websocketpp_roundtrip
```

## External libraries (manual setup)

These require system packages or vendored deps; add locally following the same echo/latency pattern:

| Library | Pin | Notes |
|---------|-----|-------|
| Boost.Beast | Boost 1.83+ | Needs Boost.Asio + OpenSSL |
| IXWebSocket | 11.4.x | zlib + TLS backend |
| Simple-WebSocket-Server | tag | ASIO + OpenSSL |
| easywsclient | master | Client-only connect latency |
| libwebsockets | 4.3.x | C API baseline |

Metrics to record: echo latency p50/p99, throughput MB/s, static binary size (`ls -la`), peak RSS (`/usr/bin/time -v`).
