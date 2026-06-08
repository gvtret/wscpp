# WebSocket Libraries for `C++11` — Comparative Analysis

Comparative catalog and benchmark results for **wscpp** (v1.1.0). Numbers below were measured after the RFC gate (6455 + UTF-8 §8.1 + basic WSS) was green.

## Executive summary

| Metric | wscpp (linux) | wscpp (ASIO) | websocketpp | IXWebSocket | libwebsockets | Beast | SWS | easywsclient |
|--------|---------------|--------------|-------------|-------------|---------------|-------|-----|--------------|
| **Role** | client + server | client + server | client + server | client + server | client + server | client + server | client + server | client only |
| **`C++11`, no Boost** | yes | yes (needs ASIO) | yes (needs ASIO) | yes | C API | no (Boost) | yes (ASIO) | yes |
| **Echo p50 (localhost)** | 0.25 ms | 0.25 ms | 0.31 ms | 0.28 ms | 0.26 ms | 0.25 ms | 0.28 ms | — [b] |
| **Echo p50 (LAN)** | 0.34 ms | 0.40 ms | 0.32 ms | 0.42 ms | 0.33 ms | 0.32 ms | 0.37 ms | — [b] |
| **Echo / connect p99 (localhost)** | 0.36 ms | 0.32 ms | 0.59 ms | 0.68 ms | 0.40 ms | 0.32 ms | 0.46 ms | 1.88 ms connect |
| **Echo / connect p99 (LAN)** | 1.24 ms | 0.77 ms | 2.01 ms | 0.85 ms | 3.55 ms | 0.86 ms | 0.61 ms | 30 ms connect |
| **64 KiB throughput (localhost)** | 92 MB/s | 82 MB/s | — [a] | 62 MB/s | — [a] | 68 MB/s | — [a] | — [b] |
| **64 KiB throughput (LAN)** | 27 MB/s | 28 MB/s | — [a] | 31 MB/s | — [a] | 29 MB/s | — [a] | — [b] |
| **Bench binary size** | 281 KB | 383 KB | 687 KB | 473 KB | 126 KB* | 668 KB | 675 KB | 370 KB |

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
| Echo latency over LAN | `bench_*_roundtrip_net` + `bench_*_echo_server` | client local, echo server remote; `run_remote_network_compare.sh` |
| 64 KiB binary throughput | `bench_roundtrip`, `bench_ixwebsocket_roundtrip` | 100 iterations |
| Connect latency | `bench_easywsclient_connect` | TCP + WS handshake, client-only |

**Limitations:** single connection per run; no TLS in compare suite; libwebsockets bench needs `libwebsockets-dev`; Beast bench needs `libboost-system-dev`. LAN compare needs a reachable remote host (default `deploy@192.168.1.165`).

**Environment (2026-06-07):** Linux/WSL2 client (GCC 15), Release; localhost benches on `127.0.0.1`; LAN benches — echo server on Debian 13 VM at `192.168.1.165` (GCC 14, ICMP RTT ~0.5–0.7 ms); no TLS in compare benches.

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

**Disclaimer:** wscpp is an **experimental**, **AI-implemented** library (see [README.md](README.md)). Benchmark and feature tables describe current code, not a maturity guarantee.

**Strengths:**

- **`C++11`**-only, no Boost; reproducible builds via FetchContent ASIO or linux-only OpenSSL path
- Dual transport: linux POSIX (281 KB bench) or ASIO (383 KB bench) vs 473–687 KB peers
- Explicit layering: frame → connection → client/server; all I/O returns `std::error_code`
- RFC 6455 regression vectors and 94 automated tests in-tree

**Trade-offs:**

- permessage-deflate (RFC 7692) with zlib; opt-in via `client::enable_permessage_deflate()`
- Client cert verification defaults to `verify_none` for developer ergonomics
- Fewer extensions and examples than websocketpp / Beast / libwebsockets

## Benchmark results

### wscpp — transport comparison (`bench_roundtrip`)

Measured with the same harness; only CMake flag `WSCPP_USE_ASIO` differs.

| Transport | Echo p50 | Echo p99 | 64 KiB throughput | Binary size |
|-----------|----------|----------|-------------------|-------------|
| **linux POSIX** (`WSCPP_USE_ASIO=OFF`) | 0.25 ms | 0.36 ms | 92 MB/s | 281 KB |
| **ASIO** (`WSCPP_USE_ASIO=ON`) | 0.25 ms | 0.32 ms | 82 MB/s | 383 KB |

Micro-benchmarks (`bench_frame_parse`, `bench_masking`) are transport-agnostic (frame layer only). Masking uses 64/32/8/4-byte XOR unrolling (`detail/mask.hpp`); connection reuses an outbound frame buffer.

| Scenario | Throughput |
|----------|------------|
| Frame build | ~15 GB/s |
| Frame parse | ~22 GB/s |
| Mask/unmask XOR | ~70 GB/s |
| Masked build+parse | ~4.6 GB/s |

### Network benchmark (LAN)

Echo server on **`deploy@192.168.1.165`** (Debian 13, GCC 14, QEMU VM, 8 vCPU); client on the WSL2 bench host (GCC 15). ICMP RTT ~0.5–0.7 ms. Plain `ws://`, port 19081, 100 samples. Harness: `benchmarks/run_remote_network_compare.sh`.

#### wscpp dual transport (LAN)

| Transport | Echo p50 | Echo p99 | 64 KiB throughput |
|-----------|----------|----------|-------------------|
| **linux POSIX** (`WSCPP_USE_ASIO=OFF`) | 0.34 ms | 1.24 ms | 27 MB/s |
| **ASIO** (`WSCPP_USE_ASIO=ON`) | 0.40 ms | 0.77 ms | 28 MB/s |

#### Third-party compare (LAN)

| Library | Echo p50 | Echo p99 | 64 KiB throughput |
|---------|----------|----------|-------------------|
| **websocketpp** 0.8.2 | 0.32 ms | 2.01 ms | — [a] |
| **IXWebSocket** 11.4.6 | 0.42 ms | 0.85 ms | 31 MB/s |
| **libwebsockets** 4.3.5 | 0.33 ms | 3.55 ms | — [a] |
| **Boost.Beast** (Boost 1.88) | 0.32 ms | 0.86 ms | 29 MB/s |
| **Simple-WebSocket-Server** | 0.37 ms | 0.61 ms | — [a] |
| **easywsclient** (connect) | 1.50 ms | 30 ms | — [b] |

Compared to localhost (same code, same day): echo p50 rises by ~0.07–0.17 ms; 64 KiB throughput drops from ~62–92 MB/s to ~27–31 MB/s — dominated by real NIC/stack and two-machine scheduling, not framing micro-ops.

Reproduce:

```bash
bash benchmarks/run_remote_network_compare.sh
# re-run benchmarks only (skip rsync/build):
WSCPP_BENCH_SKIP_BUILD=1 bash benchmarks/run_remote_network_compare.sh
```

Override host: `WSCPP_BENCH_REMOTE`, `WSCPP_BENCH_HOST`, `WSCPP_BENCH_SAMPLES`.

Per-library targets: `compare_net_servers` (remote), `compare_net_clients` (local). Micro-benchmarks (`bench_frame_parse`, `bench_masking`) remain CPU-local — they do not use the network harness.

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
| Embedded / minimal deps, **`C++11`** | **wscpp** linux transport (281 KB echo binary, no ASIO) |
| Cross-platform ASIO parity | **wscpp** with `WSCPP_USE_ASIO=ON` |
| No exceptions in app code | **wscpp** (`std::error_code` API) |
| Client-only, smallest code | **easywsclient** (~600 LOC; blocking) |
| Maximum features + extensions | **websocketpp**, **Boost.Beast** |
| Lowest localhost echo (this setup) | wscpp, websocketpp, IXWebSocket, libwebsockets (within ~0.1 ms p50) |
| Mobile / cross-platform SDK | **IXWebSocket** |
| C codebase / mature ops stack | **libwebsockets** |
| Need permessage-deflate now | **wscpp** v1.1+, IXWebSocket, Beast, libwebsockets |

## C++ vs Rust implementations

Side-by-side comparison of **wscpp** (C++11, v1.1.0) and **ws-rs** (Rust, `feature/rust-impl` v0.3.1). Full Rust peer catalog: [ANALYSIS_RUST.md](ANALYSIS_RUST.md). Versions are **independent** SemVer lines (`VERSION` vs `rust/VERSION`).

### Architecture

| Layer | wscpp (C++) | ws-rs (Rust) |
|-------|-------------|--------------|
| Framing | `wscpp::frame::parser` / `builder` | `ws_rs::frame::Parser` / `FrameBuilder` |
| Handshake | `wscpp::handshake` | `ws_rs::handshake` |
| Connection | `wscpp::connection` (thread + transport) | `ws_rs::connection` (tokio `TcpStream`) |
| Public API | `wscpp::client`, `wscpp::server` | `ws_rs::client::Client`, `ws_rs::server::Server` |
| Errors | `std::error_code` | `Result<T, ws_rs::Error>` |
| I/O runtime | ASIO or POSIX poll | tokio async |

### Feature parity (current)

| Feature | wscpp | ws-rs |
|---------|-------|-------|
| RFC 6455 framing | yes | yes |
| Opening handshake | yes | yes |
| ws:// echo client/server | yes | yes |
| wss:// TLS | yes (OpenSSL) | yes (rustls) |
| permessage-deflate (RFC 7692) | yes (v1.1.0) | yes (v0.3.0) |
| Dual transport (ASIO / linux) | yes | n/a (tokio only) |
| Regression vectors §5.7 | 6 tests | 6 tests (ported) |

### Localhost benchmarks (indicative)

| Scenario | wscpp (linux) | wscpp (ASIO) | ws-rs |
|----------|---------------|--------------|-------|
| Echo p50 (text ping, 100 samples) | 0.25 ms | 0.25 ms | 0.31 ms |
| Echo p99 | 0.36 ms | 0.32 ms | 0.68 ms |
| 64 KiB throughput (100 iter) | 92 MB/s | 82 MB/s | 78 MB/s |
| Frame build 1 MiB (50 iter) | — [d] | — [d] | 16139 MB/s [e] |
| Frame parse 1 MiB (50 iter) | — [d] | — [d] | 17146 MB/s [e] |

**Footnotes:** [d] C++ reports echo only in `bench_roundtrip`; 1 MiB frame numbers from `bench_frame_parse` (ws-rs uses same harness). See [ANALYSIS_RUST.md](ANALYSIS_RUST.md). [e] ws-rs v0.4.x speed pass — single-copy frame encoder roughly doubled `frame_build`; `frame_parse` is memcpy-bound and unchanged (WSL2 noise).

### LAN benchmarks (Rust peers, `run_remote_network_compare.sh`)

| Library | p50 | p99 | 64 KiB throughput |
|---------|-----|-----|-------------------|
| ws-rs | 0.34 ms | 0.80 ms | 32 MB/s |
| tokio-tungstenite | 0.35 ms | 0.46 ms | 33 MB/s |
| fastwebsockets | 0.33 ms | 0.47 ms | 31 MB/s |
| tokio-websockets | 0.37 ms | 0.63 ms | 30 MB/s |

Echo server on `192.168.1.165`, client local (WSL2); same topology as C++ LAN suite.

### When to use which

| Goal | Recommendation |
|------|----------------|
| Embedded C++11, minimal binary | **wscpp** linux transport |
| Existing C++ ASIO codebase | **wscpp** ASIO or **websocketpp** |
| Rust + tokio service | **tokio-tungstenite** (mature) or **ws-rs** (RFC parity with wscpp) |
| Cross-language RFC regression suite | **wscpp** + **ws-rs** share §5.7 vectors |

## Changelog

| Date | Change |
|------|--------|
| 2026-06-07 | ws-rs v0.4.x speed pass: fat LTO, single-copy encoder, cursor read buffer; `frame_build` ~8.4→16.1 GB/s, 64 KiB echo 73→78 MB/s |
| 2026-06-07 | Rust LAN compare (4 libraries on 192.168.1.165) |
| 2026-06-07 | ws-rs C++-parity benchmark harness + numbers |
| 2026-06-07 | ws-rs echo + tokio-tungstenite compare numbers |
| 2026-06-07 | C++ vs Rust comparison (ws-rs on `feature/rust-impl`) |
| 2026-06-07 | Initial catalog (F0) |
| 2026-06-07 | Post-RFC gate numbers; wscpp + websocketpp |
| 2026-06-07 | UTF-8 §8.1; v1.0.2 baseline |
| 2026-06-07 | Dual transport benches; `error_code` API; `run_benchmarks_both.sh` |
| 2026-06-07 | Frame perf: word-at-a-time masking, zero-copy build; updated micro + echo numbers |
| 2026-06-07 | Frame perf: uint64 mask unroll, reused outbound buffer, UTF-8 ASCII fast path |
| 2026-06-07 | LAN network benchmark (`bench_echo_server`, `bench_roundtrip_net`, `run_remote_network.sh`) |
| 2026-06-07 | Full LAN compare suite (`bench_*_echo_server`, `bench_*_roundtrip_net`, `run_remote_network_compare.sh`) |
