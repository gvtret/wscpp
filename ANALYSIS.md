# WebSocket Libraries for C++11 — Comparative Analysis

Comparative catalog and benchmark results for **wscpp** (v1.0.2). Numbers below were measured after the RFC gate (6455 + UTF-8 §8.1 + basic WSS) was green.

## Executive summary

| | wscpp | websocketpp | IXWebSocket | easywsclient |
|---|-------|-------------|-------------|--------------|
| **Role** | client + server | client + server | client + server | client only |
| **C++11, no Boost** | yes | yes (needs ASIO) | yes | yes |
| **Echo p50 (localhost)** | 0.26 ms | 0.26 ms | 0.24 ms | — |
| **Echo / connect p99** | 0.34 ms | 0.61 ms | 0.37 ms | 1.78 ms connect |
| **64 KiB throughput** | 84 MB/s | — | 66 MB/s | — |
| **Bench binary size** | 379 KB | 708 KB | 485 KB | 370 KB |

On localhost echo, wscpp, websocketpp, and IXWebSocket are in the same latency band. wscpp offers the smallest full client+server footprint among measured stacks without sacrificing RFC 6455 coverage in v1.0.2.

## Methodology

**Inclusion criterion:** official support for C++11 (or C callable from C++11 without requiring a newer standard).

### RFC gate (benchmark precondition)

| RFC | Scope | Status |
|-----|--------|--------|
| [RFC 6455](https://www.rfc-editor.org/rfc/rfc6455) | Framing (§5), handshake (§4), close (§7), ping/pong (§5.5), fragmentation, UTF-8 (§8.1) | **Implemented** |
| [RFC 2818](https://www.rfc-editor.org/rfc/rfc2818) | `wss://` TLS + SNI | **Implemented** |
| [RFC 7692](https://www.rfc-editor.org/rfc/rfc7692) | permessage-deflate | Post-v1.0; excluded from baseline |

### Comparison dimensions

| Dimension | Description |
|-----------|-------------|
| Client / Server | Full-duplex roles |
| TLS | Native or via dependency |
| Async model | ASIO, threads, callbacks |
| Dependencies | Boost, OpenSSL, zlib |
| Footprint | Static binary size of bench target |

### Benchmark scenarios

| Scenario | Harness target | Notes |
|----------|------------------|-------|
| Frame parse/build (1 MiB) | `bench_frame_parse` | wscpp only |
| Mask/unmask throughput | `bench_masking` | wscpp only |
| Echo latency p50/p99 (100 samples) | `bench_*_roundtrip` | localhost, text ping |
| 64 KiB binary throughput | `bench_roundtrip`, `bench_ixwebsocket_roundtrip` | 100 iterations |
| Connect latency | `bench_easywsclient_connect` | TCP + WS handshake, client-only |

**Environment (2026-06-07):** Linux/WSL2, Release, GCC 15, `127.0.0.1`, no TLS in compare benches.

**Limitations:** localhost only; single machine; no concurrent connections; no TLS in compare suite; Beast / Simple-WebSocket-Server / libwebsockets require manual setup (see `benchmarks/compare/README.md`).

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
| [easywsclient](https://github.com/dhbaird/easywsclient) | yes | + | — | — | ~600 LOC, blocking client |
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

## Feature matrix (qualitative)

| Feature | wscpp | websocketpp | IXWebSocket | Beast | libwebsockets |
|---------|-------|-------------|-------------|-------|---------------|
| Client masking (RFC 6455) | yes | yes | yes | yes | yes |
| Fragment reassembly | yes | yes | yes | yes | yes |
| Auto pong | yes | yes | yes | yes | yes |
| UTF-8 text validation | yes | yes | yes | yes | varies |
| permessage-deflate | no (v1.0) | yes | yes | yes | yes |
| FetchContent-friendly | yes (ASIO) | yes | yes | no (Boost) | partial |
| Header-only option | no (static lib) | yes | no | no | no (C lib) |

## wscpp positioning

**Strengths:**

- C++11-only, no Boost; reproducible builds via FetchContent ASIO
- Smallest measured full-stack bench binary (379 KB vs 485–708 KB peers)
- Explicit layering: frame → connection → client|server
- RFC 6455 regression vectors and 90 automated tests in-tree

**Trade-offs:**

- No permessage-deflate in v1.0 (RFC 7692 planned post-v1.0)
- Client cert verification defaults to `verify_none` for developer ergonomics
- Fewer extensions and examples than websocketpp / Beast / libwebsockets

## Benchmark results

### Comparative echo / connect (automated harness)

| Library | Echo p50 | Echo p99 | 64 KiB throughput | Binary size |
|---------|----------|----------|-------------------|-------------|
| **wscpp** | 0.26 ms | 0.34 ms | 84 MB/s | 379 KB |
| **websocketpp** 0.8.2 | 0.26 ms | 0.61 ms | — | 708 KB |
| **IXWebSocket** 11.4.6 | 0.24 ms | 0.37 ms | 66 MB/s | 485 KB |
| **easywsclient** (connect) | 1.16 ms | 1.78 ms | — | 370 KB |

easywsclient row measures full TCP + WebSocket handshake (blocking API); not comparable to echo latency rows.

### wscpp micro-benchmarks (1 MiB, single-threaded)

| Scenario | Throughput |
|----------|------------|
| Frame build | ~2.3 GB/s |
| Frame parse | ~27 GB/s (hot cache) |
| Mask/unmask XOR | ~2.2 GB/s |
| Masked build+parse | ~877 MB/s |

### Reproduce

```bash
cmake -B build -DWSCPP_BUILD_BENCHMARKS=ON
cmake --build build --target benchmarks compare_benchmarks
./build/bin/bench_roundtrip
./build/bin/bench_websocketpp_roundtrip
./build/bin/bench_ixwebsocket_roundtrip
./build/bin/bench_easywsclient_connect
./build/bin/bench_frame_parse
./build/bin/bench_masking
```

Manual compare targets (Beast, Simple-WebSocket-Server, libwebsockets): `benchmarks/compare/README.md`.

## Recommendations

| Use case | Suggestion |
|----------|------------|
| Embedded / minimal deps, C++11 | **wscpp** (379 KB echo binary, no Boost) |
| Client-only, smallest code | **easywsclient** (~600 LOC; blocking) |
| Maximum features + extensions | **websocketpp**, **Boost.Beast** |
| Lowest localhost echo (this setup) | wscpp, websocketpp, IXWebSocket (within ~0.1 ms p50) |
| Mobile / cross-platform SDK | **IXWebSocket** |
| C codebase / mature ops stack | **libwebsockets** |
| Need permessage-deflate now | IXWebSocket, Beast, libwebsockets (not wscpp v1.0) |

## Changelog

| Date | Change |
|------|--------|
| 2026-06-07 | Initial catalog (F0) |
| 2026-06-07 | Post-RFC gate numbers; wscpp + websocketpp |
| 2026-06-07 | UTF-8 §8.1; v1.0.2 baseline |
| 2026-06-07 | F2 expansion: IXWebSocket, easywsclient; F3 final analysis |
