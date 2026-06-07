# WebSocket Libraries for C++11 — Comparative Analysis

> Draft catalog and methodology for wscpp v1.0. Benchmark results (F2) to be filled when comparative harness is complete.

## Methodology

**Inclusion criterion:** official support for C++11 (or C callable from C++11 without requiring a newer standard).

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

_TBD — see `benchmarks/` after F1/F2 implementation._

| Library | Echo latency p50 | Throughput MB/s | Static lib size |
|---------|------------------|-----------------|-----------------|
| wscpp | — | — | — |
| websocketpp | — | — | — |
| Beast | — | — | — |

## Recommendations

| Use case | Suggestion |
|----------|------------|
| Embedded / minimal deps | wscpp, easywsclient |
| Maximum features + ecosystem | websocketpp, Boost.Beast |
| Mobile / cross-platform SDK | IXWebSocket |
| C codebase | libwebsockets |
