# WebSocket Libraries for `C++11` — Comparative Analysis

Comparative catalog and benchmark results for **wscpp** (v1.1.0). Numbers below were measured after the RFC gate (6455 + UTF-8 §8.1 + basic WSS) was green.

## Executive summary

| | wscpp (linux) | wscpp (ASIO) | websocketpp | IXWebSocket | libwebsockets | Beast | SWS | easywsclient |
|---|---------------|--------------|-------------|-------------|---------------|-------|-----|--------------|
| **Role** | client + server | client + server | client + server | client + server | client + server | client + server | client + server | client only |
| **`C++11`, no Boost** | yes | yes (needs ASIO) | yes (needs ASIO) | yes | C API | no (Boost) | yes (ASIO) | yes |
| **Echo p50 (localhost)** | 0.26 ms | 0.30 ms | 0.31 ms | 0.28 ms | 0.26 ms | 0.25 ms | 0.28 ms | — [b] |
| **Echo / connect p99** | 0.41 ms | 0.57 ms | 0.59 ms | 0.68 ms | 0.40 ms | 0.32 ms | 0.46 ms | 1.88 ms connect |
| **64 KiB throughput** | 76 MB/s | 71 MB/s | — [a] | 62 MB/s | — [a] | 68 MB/s | — [a] | — [b] |
| **Bench binary size** | 278 KB | 379 KB | 687 KB | 473 KB | 126 KB* | 668 KB | 675 KB | 370 KB |

**Executive summary footnotes:** [a] compare target measures echo only (no 64 KiB loop in `bench_*_roundtrip`). [b] client-only — harness runs connect latency, not echo (`bench_easywsclient_connect`). Full tag list: [Table legend](#table-legend).

wscpp ships two transports on Linux: **POSIX sockets + OpenSSL** (`WSCPP_USE_ASIO=OFF`, default for minimal footprint) and **standalone ASIO** (`WSCPP_USE_ASIO=ON`). Echo latency is in the same band for both; linux transport yields a smaller `bench_roundtrip` binary (~27% less than ASIO).

Public API uses `std::error_code` throughout — no exceptions.

## Methodology

**Inclusion criterion:** official support for **`C++11`** (or C callable from **`C++11`** without requiring a newer standard).

### RFC gate (benchmark precondition)

| RFC | Scope | Status |
|-----|--------|--------|
| [RFC 6455](https://www.rfc-editor.org/rfc/rfc6455) | Framing (§5), handshake (§4), close (§7), ping/pong (§5.5), fragmentation, UTF-8 (§8.1) | **Implemented** |
| [RFC 2818](https://www.rfc-editor.org/rfc/rfc2818) | `wss://` TLS + SNI | **Implemented** |
| [RFC 7692](https://www.rfc-editor.org/rfc/rfc7692) | permessage-deflate | **Implemented** (v1.1.0) |

### Table legend

| Symbol | Meaning |
|--------|---------|
| **—** | Missing value — see bracket tag in the cell or footnote below the table |
| **+** | Supported (Tier 2/3 capability columns) |
| **yes / no** | Measured or stated for wscpp / peers (Feature matrix) |

**Footnote tags (capability & catalog tables):**

| Tag | Meaning |
|-----|---------|
| **[a]** | **Not measured** — no 64 KiB throughput loop in the in-tree compare target (echo-only `bench_*_roundtrip`; see `benchmarks/compare/README.md`). |
| **[b]** | **Not measured (client-only)** — no echo roundtrip bench; connect latency is reported on a separate row (`bench_easywsclient_connect`). |
| **[c]** | **N/A — no server** — library is client-only; server role or server-side TLS termination is out of scope. |
| **[d]** | **N/A — no client** — library is server-only or framing-only without a client API. |
| **[e]** | **N/A — no native TLS** — library does not implement TLS/WSS; plain TCP/WebSocket only. |

### Comparison dimensions

| Dimension | Description |
|-----------|-------------|
| Client / Server | Full-duplex roles |
| TLS | Native or via dependency |
| Async model | ASIO, POSIX poll, callbacks |
| Dependencies | Boost, OpenSSL, zlib |
| Footprint | Static binary size of bench target |
| Error handling | wscpp: `std::error_code`; peers vary |

### Benchmark scenarios

| Scenario | Harness target | Notes |
|----------|----------------|-------|
| Frame parse/build (1 MiB) | `bench_frame_parse` | wscpp only |
| Mask/unmask throughput | `bench_masking` | wscpp only |
| Echo latency p50/p99 (100 samples) | `bench_roundtrip`, `bench_*_roundtrip` | localhost; run wscpp twice (linux + ASIO) |
| 64 KiB binary throughput | `bench_roundtrip`, `bench_ixwebsocket_roundtrip` | 100 iterations |
| Connect latency | `bench_easywsclient_connect` | TCP + WS handshake, client-only |

**Limitations:** localhost only; single machine; no concurrent connections; no TLS in compare suite; libwebsockets bench needs `libwebsockets-dev`; Beast bench needs `libboost-system-dev`.

**Environment (2026-06-07):** Linux/WSL2, Release, GCC 15, `127.0.0.1`, no TLS in compare benches.

## Tier 1 — native C++, **`C++11`**, client + server

| Library | **`C++11`** | Client | Server | TLS | Async | Dependencies | License |
|---------|-------|--------|--------|-----|-------|--------------|---------|
| [websocketpp](https://github.com/zaphoyd/websocketpp) | yes | + | + | + | ASIO | ASIO, OpenSSL | BSD-3 |
| [Boost.Beast](https://github.com/boostorg/beast) | yes | + | + | + | Boost.Asio | Boost, OpenSSL | BSL-1.0 |
| [IXWebSocket](https://github.com/machinezone/IXWebSocket) | yes | + | + | + | threads | zlib, OpenSSL/MbedTLS | BSD-3 |
| [Simple-WebSocket-Server](https://github.com/eidheim/Simple-WebSocket-Server) | yes | + | + | + | ASIO | ASIO, OpenSSL | MIT |
| [cpprestsdk](https://github.com/microsoft/cpprestsdk) | yes | + | + | + | PPL | Boost.Asio, OpenSSL | MIT |
| [Qt WebSockets](https://doc.qt.io/qt-5/qtwebsockets-index.html) | yes | + | + | + | Qt loop | Qt5/6 | LGPL |
| **wscpp** (this project) | yes | + | + | + | ASIO or POSIX | ASIO† or OpenSSL only | MIT |

† ASIO via FetchContent when `WSCPP_USE_ASIO=ON`.

## Tier 2 — minimal / specialized **`C++11`**

| Library | **`C++11`** | Client | Server | TLS | Notes |
|---------|-------|--------|--------|-----|-------|
| [easywsclient](https://github.com/dhbaird/easywsclient) | yes | + | — [c] | — [e] | ~600 LOC, blocking client |
| [socket.io-clientpp](https://github.com/ebshimizu/socket.io-clientpp) | yes | + | — [c] | + | Socket.IO client, not a generic WS server |

## Tier 3 — C libraries usable from **`C++11`**

| Library | Client | Server | TLS | Notes |
|---------|--------|--------|-----|-------|
| [libwebsockets](https://github.com/warmcat/libwebsockets) | + | + | + | Production C stack |
| [wslay](https://github.com/tatsuhiro-t/wslay) | framing | framing | ext. | RFC6455 framing only |
| [libwebsock](https://github.com/payden/libwebsock) | — [d] | + | — [e] | Simple server; no client API |
| [libwsclient](https://github.com/payden/libwsclient) | + | — [c] | — [e] | Simple client; no server API |

Tier 2/3 capability dashes use tags [c]–[e] from [Table legend](#table-legend).

## Tier 4 — excluded (minimum standard > **`C++11`**)

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
| permessage-deflate | yes (v1.1.0) | yes | yes | yes | yes |
| FetchContent-friendly | yes (ASIO) | yes | yes | no (Boost) | partial |
| Header-only option | no (static lib) | yes | no | no | no (C lib) |
| error_code API (no throw) | yes | no | partial | partial | C errno |

## wscpp positioning

**Strengths:**

- **`C++11`**-only, no Boost; reproducible builds via FetchContent ASIO or linux-only OpenSSL path
- Dual transport: linux POSIX (278 KB bench) or ASIO (379 KB bench) vs 473–687 KB peers
- Explicit layering: frame → connection → client|server; all I/O returns `std::error_code`
- RFC 6455 regression vectors and 90 automated tests in-tree

**Trade-offs:**

- permessage-deflate (RFC 7692) with zlib; opt-in via `client::enable_permessage_deflate()`
- Client cert verification defaults to `verify_none` for developer ergonomics
- Fewer extensions and examples than websocketpp / Beast / libwebsockets

## Benchmark results

### wscpp — transport comparison (`bench_roundtrip`)

Measured with the same harness; only CMake flag `WSCPP_USE_ASIO` differs.

| Transport | Echo p50 | Echo p99 | 64 KiB throughput | Binary size |
|-----------|----------|----------|-------------------|-------------|
| **linux POSIX** (`WSCPP_USE_ASIO=OFF`) | 0.26 ms | 0.41 ms | 76 MB/s | 278 KB |
| **ASIO** (`WSCPP_USE_ASIO=ON`) | 0.30 ms | 0.57 ms | 71 MB/s | 379 KB |

Micro-benchmarks (`bench_frame_parse`, `bench_masking`) are transport-agnostic (frame layer only).

| Scenario | Throughput |
|----------|------------|
| Frame build | ~2.3 GB/s |
| Frame parse | ~27 GB/s (hot cache) |
| Mask/unmask XOR | ~2.2 GB/s |
| Masked build+parse | ~877 MB/s |

### Third-party compare (automated harness, ASIO build tree)

| Library | Echo p50 | Echo p99 | 64 KiB throughput | Binary size |
|---------|----------|----------|-------------------|-------------|
| **websocketpp** 0.8.2 | 0.31 ms | 0.59 ms | — [a] | 687 KB |
| **IXWebSocket** 11.4.6 | 0.28 ms | 0.68 ms | 62 MB/s | 473 KB |
| **libwebsockets** 4.3.5 | 0.26 ms | 0.40 ms | — [a] | 126 KB* |
| **Boost.Beast** (Boost 1.88) | 0.25 ms | 0.32 ms | 68 MB/s | 668 KB |
| **Simple-WebSocket-Server** | 0.28 ms | 0.46 ms | — [a] | 675 KB |
| **easywsclient** (connect) | 1.17 ms | 1.88 ms | — [b] | 370 KB |

\* libwebsockets bench binary links the system shared library (`libwebsockets.so`); size is not directly comparable to statically linked peers.

**Third-party footnotes:** [a] echo-only compare target — throughput column omitted by design. [b] see Executive summary [b]; row label is **connect**, not echo.

easywsclient row measures full TCP + WebSocket handshake (blocking API); not comparable to echo latency rows.

### Reproduce

**Both wscpp transports + compare:**

```bash
bash benchmarks/run_benchmarks_both.sh
```

**Single build:**

```bash
# linux transport (default minimal)
cmake -B build-linux -DWSCPP_BUILD_BENCHMARKS=ON -DWSCPP_USE_ASIO=OFF
cmake --build build-linux --target run_benchmarks

# ASIO transport
cmake -B build-asio -DWSCPP_BUILD_BENCHMARKS=ON -DWSCPP_USE_ASIO=ON
cmake --build build-asio --target run_benchmarks
```

## Recommendations

| Use case | Suggestion |
|----------|------------|
| Embedded / minimal deps, **`C++11`** | **wscpp** linux transport (278 KB echo binary, no ASIO) |
| Cross-platform ASIO parity | **wscpp** with `WSCPP_USE_ASIO=ON` |
| No exceptions in app code | **wscpp** (`std::error_code` API) |
| Client-only, smallest code | **easywsclient** (~600 LOC; blocking) |
| Maximum features + extensions | **websocketpp**, **Boost.Beast** |
| Lowest localhost echo (this setup) | wscpp, websocketpp, IXWebSocket, libwebsockets (within ~0.1 ms p50) |
| Mobile / cross-platform SDK | **IXWebSocket** |
| C codebase / mature ops stack | **libwebsockets** |
| Need permessage-deflate now | **wscpp** v1.1+, IXWebSocket, Beast, libwebsockets |

## Changelog

| Date | Change |
|------|--------|
| 2026-06-07 | Initial catalog (F0) |
| 2026-06-07 | Post-RFC gate numbers; wscpp + websocketpp |
| 2026-06-07 | UTF-8 §8.1; v1.0.2 baseline |
| 2026-06-07 | Dual transport benches; `error_code` API; `run_benchmarks_both.sh` |
