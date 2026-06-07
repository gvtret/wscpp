# WebSocket Libraries for C++11 — Comparative Analysis

> Draft catalog and methodology for wscpp. Comparative numbers below are **preliminary** — see [When to benchmark](#when-to-benchmark).

## Methodology

**Inclusion criterion:** official support for C++11 (or C callable from C++11 without requiring a newer standard).

### When to benchmark

**Run comparative benchmarks only after RFC-mandated behaviour is implemented and covered by tests.** Otherwise numbers reflect an incomplete stack, not the final product.

Gate checklist (non-exhaustive):

| RFC | Scope | Status for benchmarking |
|-----|--------|-------------------------|
| [RFC 6455](https://www.rfc-editor.org/rfc/rfc6455) | Framing (§5), handshake (§4), close (§7), ping/pong (§5.5), fragmentation | **Implemented** — masking, reassembly, control frames; UTF-8 validation optional |
| [RFC 2818](https://www.rfc-editor.org/rfc/rfc2818) | `wss://` TLS + server identity (SNI) | **Implemented** — basic WSS path + SNI |
| [RFC 7692](https://www.rfc-editor.org/rfc/rfc7692) | permessage-deflate | Post-v1.0; exclude from v1.0 baseline |

The harness in `benchmarks/` is kept for regression and iteration, but **ANALYSIS.md results must be refreshed** after the RFC checklist is green. Until then, treat existing tables as smoke-test baselines only.

**Comparison dimensions:**

| Dimension | Description |
|-----------|-------------|
| Client / Server | Full-duplex roles |
| TLS | Native or via dependency |
| Async model | ASIO, threads, callbacks |
| Dependencies | Boost, OpenSSL, zlib |
| Footprint | Binary size, compile time (measured in F2) |

**Benchmark scenarios (planned):**

- Frame parse/build throughput (1 MiB)
- Mask/unmask throughput
- Localhost connect + echo latency (p50/p99)
- Throughput (MB/s) for large messages

## Tier 1 — native C++, C++11, client + server

| Library | C++11 | Client | Server | TLS | Async | Dependencies | License |
|---------|-------|--------|--------|-----|-------|--------------|---------|
| [websocketpp](https://github.com/zaphoyd/websocketpp) | yes | + | + | + | ASIO | ASIO, OpenSSL | BSD-3 |
| [Boost.Beast](https://github.com/boostorg/beast) | yes | + | + | + | Boost.Asio | Boost, OpenSSL | BSL-1.0 |
| [IXWebSocket](https://github.com/machinezone/IXWebSocket) | yes | + | + | + | threads | zlib, OpenSSL/MbedTLS | BSD-3 |
| [Simple-WebSocket-Server](https://github.com/eidheim/Simple-WebSocket-Server) | yes | + | + | + | ASIO | ASIO, OpenSSL | MIT |
| [cpprestsdk](https://github.com/microsoft/cpprestsdk) | yes | + | + | + | PPL | Boost.Asio, OpenSSL | MIT |
| [Qt WebSockets](https://doc.qt.io/qt-5/qtwebsockets-index.html) | yes | + | + | + | Qt loop | Qt5/6 | LGPL |
| **wscpp** (this project) | yes | + | + | + | ASIO | standalone ASIO, OpenSSL | MIT |

## Tier 2 — minimal / specialized C++11

| Library | C++11 | Client | Server | TLS | Notes |
|---------|-------|--------|--------|-----|-------|
| [easywsclient](https://github.com/dhbaird/easywsclient) | yes | + | — | — | ~600 LOC client |
| [socket.io-clientpp](https://github.com/ebshimizu/socket.io-clientpp) | yes | + | — | + | Socket.IO, not raw WS |

## Tier 3 — C libraries usable from C++11

| Library | Client | Server | TLS | Notes |
|---------|--------|--------|-----|-------|
| [libwebsockets](https://github.com/warmcat/libwebsockets) | + | + | + | Production C stack |
| [wslay](https://github.com/tatsuhiro-t/wslay) | framing | framing | ext. | RFC6455 framing only |
| [libwebsock](https://github.com/payden/libwebsock) | — | + | — | Simple server |
| [libwsclient](https://github.com/payden/libwsclient) | + | — | — | Simple client |

## Tier 4 — excluded (minimum standard > C++11)

| Library | Min standard | Reason |
|---------|--------------|--------|
| uWebSockets | C++17 | README requirement |
| Poco Net 1.13+ | C++17 | Release notes |
| seasocks | C++14 | Toolchain requirement |

## wscpp positioning (qualitative)

**Strengths:**

- C++11-only, no Boost
- Small static library; explicit layering (frame / connection / client|server)
- Standalone ASIO via FetchContent — reproducible builds
- RFC 6455 regression vectors in-tree

**Trade-offs:**

- No permessage-deflate in v1.0
- Client certificate verification defaults to permissive (`verify_none`) for developer ergonomics
- Fewer features than websocketpp/Beast (by design — lightweight scope)

## Benchmark results

> **Preliminary (v1.0.0).** Measured before full RFC coverage review. Do not use for release positioning until re-run per [When to benchmark](#when-to-benchmark).

Measured on Linux/WSL2, Release build, GCC 15, local echo (100 samples, 64 KiB throughput test).

| Library | Echo p50 | Echo p99 | 64 KiB throughput | `bench_*` binary |
|---------|----------|----------|---------------------|------------------|
| **wscpp** | 0.25 ms | 0.44 ms | 81 MB/s | 371 KB |
| **websocketpp** 0.8.2 | 0.38 ms | 0.85 ms | — | 704 KB |

**wscpp micro-benchmarks** (1 MiB payload, single-threaded):

| Scenario | Throughput |
|----------|------------|
| Frame build | ~2.2 GB/s |
| Frame parse | ~21 GB/s (hot cache, same buffer) |
| Mask/unmask XOR | ~2.1 GB/s |
| Masked build+parse | ~870 MB/s |

Run locally: `cmake -B build -DWSCPP_BUILD_BENCHMARKS=ON && cmake --build build --target benchmarks bench_websocketpp_roundtrip`

Libraries requiring manual install (Beast, IXWebSocket, libwebsockets) — see `benchmarks/compare/README.md`.

## Recommendations

| Use case | Suggestion |
|----------|------------|
| Embedded / minimal deps | wscpp, easywsclient |
| Maximum features + ecosystem | websocketpp, Boost.Beast |
| Mobile / cross-platform SDK | IXWebSocket |
| C codebase | libwebsockets |
