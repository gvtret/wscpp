# WebSocket Libraries for Rust — Comparative Analysis

Comparative catalog and benchmark results for **ws-rs** (Rust port, `feature/rust-impl`). Harness mirrors C++ `benchmarks/` (`bench_frame_parse`, `bench_masking`, `bench_roundtrip`, `bench_roundtrip_net`, compare roundtrip).

## Executive summary

| Metric | ws-rs (tokio) | ws-rs (blocking) | tokio-tungstenite | fastwebsockets | tokio-websockets |
|--------|---------------|------------------|-------------------|----------------|------------------|
| **Role** | client + server | client + server | client + server | client + server | client + server |
| **Async runtime** | tokio | none (`std::net`) | tokio | tokio / manual | tokio |
| **TLS (wss)** | yes (rustls) | yes (rustls sync) | rustls/native-tls | rustls | rustls |
| **Echo p50 (localhost)** | 0.26 ms | 0.25 ms | 0.30 ms | 0.29 ms | 0.31 ms |
| **Echo p99 (localhost)** | 0.37 ms | 0.31 ms | 0.41 ms | 0.55 ms | 0.44 ms |
| **64 KiB throughput (localhost)** | 74 MB/s | 79 MB/s | 76 MB/s | 83 MB/s | 85 MB/s |
| **Echo p50 (LAN)** | 0.35 ms | 0.40 ms | 0.37 ms | 0.36 ms | 0.38 ms |
| **Echo p99 (LAN)** | 0.51 ms | 0.51 ms | 0.67 ms | 0.60 ms | 0.67 ms |
| **64 KiB throughput (LAN)** | 30 MB/s | 30 MB/s | 30 MB/s | 30 MB/s | 30 MB/s |
| **Frame build 1 MiB (50 iter)** | 8886 MB/s | — [b] | — [b] | — [b] | — [b] |
| **Frame parse 1 MiB (50 iter)** | 19701 MB/s | — [b] | — [b] | — [b] | — [b] |
| **Mask XOR 1 MiB (400 iter)** | 49664 MB/s | — [b] | — [b] | — [b] | — [b] |
| **Bench binary size** | 3747 KB | 687 KB | 1735 KB | 2080 KB | 1493 KB |

**Footnotes:** [b] frame micro-benchmarks are ws-rs only (same as C++ `bench_frame_parse` scope).

ws-rs mirrors the **wscpp** C++ layering: `frame` → `handshake` → `connection` → `client`|`server`. Errors use `Result<T, ws_rs::Error>`.

## Methodology

Aligned with C++ [ANALYSIS.md](ANALYSIS.md) / [benchmarks/README.md](benchmarks/README.md).

### RFC gate (benchmark precondition)

| RFC | Scope | ws-rs status |
|-----|--------|--------------|
| [RFC 6455](https://www.rfc-editor.org/rfc/rfc6455) | Framing §5, handshake §4, close §7, ping/pong §5.5, UTF-8 §8.1 | **Implemented** |
| [RFC 2818](https://www.rfc-editor.org/rfc/rfc2818) | `wss://` TLS + SNI | **Implemented** (async + blocking) |
| [RFC 7692](https://www.rfc-editor.org/rfc/rfc7692) | permessage-deflate | **Implemented** |

### Benchmark scenarios

| Scenario | Harness | Notes |
|----------|---------|-------|
| Frame parse/build (1 MiB) | `bench_frame_parse` | 50 iterations; same as C++ |
| Mask/unmask throughput | `bench_masking` | 200×2 XOR + 50× masked build/parse |
| Echo latency p50/p99 (100 samples) | `bench_roundtrip` | tokio transport |
| Sync echo (localhost) | `bench_blocking_roundtrip` | mirrors C++ `WSCPP_USE_ASIO=OFF` |
| 64 KiB binary throughput | `bench_roundtrip` / `bench_blocking_roundtrip` | 100 iterations, second connection |
| Echo over LAN | `bench_echo_server` + `bench_roundtrip_net` | tokio; blocking uses `bench_blocking_*` |
| LAN compare suite | `run_remote_network_compare.sh` | 4 libraries; mirrors C++ script |
| Compare echo | `bench_*_roundtrip` | localhost; tokio-tungstenite, fastwebsockets, tokio-websockets |

**Environment (2026-06-07, post-parity):** Linux/WSL2 client, Release; localhost — `bash rust/benchmarks/run_benchmarks.sh`; LAN — echo server on Debian VM `192.168.1.165` (Rust 1.96), client local, `bash rust/benchmarks/run_remote_network_compare.sh`.

## Tier 1 — async Rust, client + server

| Library | Client | Server | TLS | Runtime | License |
|---------|--------|--------|-----|---------|---------|
| [tokio-tungstenite](https://github.com/snapview/tokio-tungstenite) | + | + | + | tokio | MIT |
| [fastwebsockets](https://github.com/denoland/fastwebsockets) | + | + | + | tokio/manual | Apache-2.0 |
| [tokio-websockets](https://github.com/GeminiWallet/tokio-websockets) | + | + | + | tokio | MIT |
| [soketto](https://github.com/paritytech/soketto) | + | + | ext. | futures | MIT/Apache-2.0 |
| **ws-rs** (this project) | + | + | + (rustls) | tokio + optional blocking | MIT |

## ws-rs positioning

- **Companion to wscpp:** same benchmark names/output and RFC §5.7 regression vectors.
- **Dual transport:** tokio async (`default`) + sync `std::net` (`ws_rs::blocking`, mirrors C++ `WSCPP_USE_ASIO=OFF`).
- **Minimal deps build:** `cargo build --no-default-features --features "std-blocking,deflate,blocking-tls"` — no tokio.
- **Cargo features:** `async`, `tls`, `deflate`, `std-blocking`, `blocking-tls` (on by default); `stress` off by default (mirrors C++ `WSCPP_ENABLE_STRESS_TESTS=OFF`).
- **Version:** `0.4.0` (`rust/VERSION`, `./scripts/bump_rust_version.sh`); independent from wscpp `1.1.0`.
- **Compare harness:** ws-rs + tokio-tungstenite + fastwebsockets + tokio-websockets (`bench_*_roundtrip`, LAN `bench_*_roundtrip_net`).

## Measured results (localhost, Release)

### Micro (`bench_frame_parse`, `bench_masking`)

| Metric | ws-rs |
|--------|-------|
| frame_build | 8886 MB/s |
| frame_parse | 19701 MB/s |
| mask_unmask_xor | 49664 MB/s |
| masked_build_parse | 1035 MB/s |

### Echo (`bench_roundtrip`, 100 samples, tokio)

| Library | p50 | p99 | 64 KiB throughput |
|---------|-----|-----|-------------------|
| ws-rs | 0.26 ms | 0.37 ms | 74 MB/s |
| tokio-tungstenite | 0.30 ms | 0.41 ms | 76 MB/s |
| fastwebsockets | 0.29 ms | 0.55 ms | 83 MB/s |
| tokio-websockets | 0.31 ms | 0.44 ms | 85 MB/s |

### Echo (`bench_blocking_roundtrip`, 100 samples, std-blocking)

| Transport | p50 | p99 | 64 KiB throughput | Binary size |
|-----------|-----|-----|-------------------|-------------|
| ws-rs blocking | 0.25 ms | 0.31 ms | 79 MB/s | 687 KB |

### Network (LAN, `run_remote_network_compare.sh`, 100 samples)

| Library | p50 | p99 | 64 KiB throughput |
|---------|-----|-----|-------------------|
| ws-rs (tokio) | 0.35 ms | 0.51 ms | 30 MB/s |
| ws-rs (blocking) | 0.40 ms | 0.51 ms | 30 MB/s |
| tokio-tungstenite | 0.37 ms | 0.67 ms | 30 MB/s |
| fastwebsockets | 0.36 ms | 0.60 ms | 30 MB/s |
| tokio-websockets | 0.38 ms | 0.67 ms | 30 MB/s |

## Changelog

| Date | Change |
|------|--------|
| 2026-06-07 | v0.4.0: C++ parity complete; blocking LAN bench; `serial_test` for integration tests |
| 2026-06-07 | Post-parity bench re-run: mask ~50 GB/s; LAN ws-rs p50 0.35 ms; `bench_blocking_roundtrip` |
| 2026-06-07 | blocking `wss://` (rustls sync); optional `stress` integration tests |
| 2026-06-07 | C++ parity: mask unroll (~50 GB/s), `build_into` buffer reuse, fragmentation, close echo/1002, `std-blocking` transport |
| 2026-06-07 | v0.3.1: UTF-8 §8.1 validation + close 1007 (ported from C++) |
| 2026-06-07 | Full LAN compare (4 libs): ws-rs p50 0.34 ms, tokio-tungstenite 33 MB/s |
| 2026-06-07 | fastwebsockets + tokio-websockets compare benches (localhost + LAN harness) |
| 2026-06-07 | v0.3.0: RFC 7692 permessage-deflate; LAN numbers (ws-rs p50 0.37 ms) |
| 2026-06-07 | v0.2.0: SemVer (`rust/VERSION`, `ws_rs::version`); LAN compare script |
| 2026-06-07 | tokio-tungstenite LAN benches (`bench_*_echo_server`, `bench_*_roundtrip_net`) |
| 2026-06-07 | `wss://` via rustls + tokio-rustls (SNI, PEM certs) |
| 2026-06-07 | C++-parity harness (`bench_*` binaries, `run_benchmarks.sh`) |
| 2026-06-07 | tokio-tungstenite compare; initial catalog |
