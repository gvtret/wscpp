# wscpp — handoff log

Журнал прогресса: **старые записи сверху, новые снизу** (append-only). Внутри одного дня порядок следует релизам: v0.1.0 → v1.0.0 → v1.0.1 → v1.0.2 → v1.1.0 → unreleased.

## 2026-06-07 — Frame parser fix + handshake tests

**Done:** Fixed frame parser byte accounting (`consumed +=` in all sub-parsers); all 52 tests pass. Added `CHANGELOG.md`, `scripts/bump_version.sh` with `--dry-run`, `tests/unit/test_handshake.cpp` (9 tests, RFC 6455 §4.1.5 accept vector).

## 2026-06-07 — C3–G1: stress, examples, docs, Doxygen, CI

**Done:** Stress tests (3 scenarios, `WSCPP_ENABLE_STRESS_TESTS`); server TLS handshake before WS upgrade; tls_client/tls_server examples; D1 examples README; D3 `WSCPP_BUILD_BENCHMARKS`; E1/E2 user+dev docs; E3–E4 Doxygen config + public API comments; F0 ANALYSIS.md draft; G1 GitHub Actions CI workflow; root README update. 79/79 tests pass; docs target builds.

**State:** `master`, v0.1.0, examples verified ws+wss locally.

**Next:** F1 benchmarks harness; G2 repo templates; G4 release v1.0.0.

**Notes:** Server WSS path: adopt socket → TLS handshake → WS upgrade. Stress tests off in CI (`WSCPP_ENABLE_STRESS_TESTS=OFF`).

## 2026-06-07 — F1–G4: benchmarks, templates, v1.0.0

**Done:** F1 micro-benchmarks (frame/mask/roundtrip); F2 websocketpp compare; F3 ANALYSIS.md with measured results; G2 issue/PR templates + CODE_OF_CONDUCT; G4 release v1.0.0. 79/79 tests, docs target OK.

**State:** `master`, VERSION 1.0.0, tag `v1.0.0`.

**Next:** Optional F2 expansion (Beast/IXWebSocket); post-v1.0 features (permessage-deflate).

**Notes:** Roundtrip bench must not call `client.run()` — io_context is idle; use wait loop like integration tests.

## 2026-06-07 — RFC 6455 protocol completion (post-v1.0.0)

**Done:** Client outbound masking; server/client inbound mask validation; RSV/opcode checks; fragmented message reassembly; auto pong; close code validation; `send_continuation`/`send_ping`; integration tests (fragmented echo, ping). 82/82 tests pass.

**State:** `master`, VERSION 1.0.0; RFC gate ready for benchmark re-run (except RFC 7692).

**Next:** Benchmark re-run; UTF-8 text validation (RFC 6455 §8.1).

## 2026-06-07 — Benchmark re-run post-RFC gate (v1.0.1)

**Done:** Re-ran all benchmarks; updated ANALYSIS.md with post-RFC numbers; release v1.0.1. 82/82 tests pass.

**State:** `master`, VERSION 1.0.1, tag `v1.0.1`.

**Next:** UTF-8 validation; optional comparative benchmarks for external libs.

## 2026-06-07 — UTF-8 validation (RFC 6455 §8.1, v1.0.2)

**Done:** TEXT frame UTF-8 validation via `detail::is_valid_utf8()`; close 1007 on invalid payload; `fail_with_close` invokes `on_close`; unit tests (7) + integration test. Release v1.0.2. 90/90 tests pass.

**State:** `master`, VERSION 1.0.2, tag `v1.0.2`.

**Next:** F2 expansion (IXWebSocket, easywsclient, libwebsockets).

## 2026-06-07 — F2: comparative benchmarks expansion (v1.0.2)

**Done:** IXWebSocket 11.4.6 and easywsclient compare targets via FetchContent; `compare_benchmarks` CMake target; ANALYSIS.md updated with 4-library numbers. 90/90 tests pass.

**State:** `master`, VERSION 1.0.2.

**Next:** libwebsockets bench; refresh ANALYSIS.

## 2026-06-07 — F2: libwebsockets compare bench (v1.0.2)

**Done:** `bench_libwebsockets_roundtrip` via pkg-config (4.3.5); ANALYSIS/README updated. F2 harness now covers 5 libraries (+ wscpp). 90/90 tests pass.

**State:** `master`, VERSION 1.0.2.

**Next:** Finalize ANALYSIS; RFC 7692 permessage-deflate.

## 2026-06-07 — F3 + G3: final analysis and contributing sync (v1.0.2)

**Done:** ANALYSIS.md finalized (executive summary, feature matrix, limitations, recommendations); CONTRIBUTING.md updated with benchmark refresh policy. 90/90 tests pass.

**State:** `master`, VERSION 1.0.2.

**Next:** RFC 7692 permessage-deflate; optional Beast/SWS benches.

## 2026-06-07 — RFC 7692 permessage-deflate (v1.1.0)

**Done:** zlib codec, handshake negotiation, RSV1 on connection; `client::enable_permessage_deflate()`; 3 unit + 1 integration test. Release v1.1.0 (`166b43a`, tag `v1.1.0`). 94/94 tests pass.

**State:** `master`, VERSION 1.1.0, tag `v1.1.0`.

**Next:** Beast/SWS compare benches; CI zlib dep.

## 2026-06-07 — F2: Beast + Simple-WebSocket-Server compare benches (v1.1.0)

**Done:** `bench_beast_roundtrip` (Boost 1.88, sync echo); `bench_sws_roundtrip` (GitLab SWS `0e1cf67`, standalone ASIO). ANALYSIS.md updated with 7-library numbers. 94/94 tests pass.

**State:** `master`, VERSION 1.1.0.

**Next:** Linux POSIX transport; optional bench hardening.

## 2026-06-07 — Linux POSIX socket transport (v1.1.0, unreleased)

**Done:** Transport abstraction via `wscpp/net/transport.hpp`; `WSCPP_USE_ASIO` CMake flag (default ON). OFF builds `linux_socket` + OpenSSL TLS on Linux. Poll-based accept loop; `std::error_code` API. CI matrix ASIO + linux (GCC/Clang). 94/94 tests pass.

**State:** `master`, VERSION 1.1.0.

**Next:** Dual-transport benchmark harness; strict CI gates.

## 2026-06-07 — Benchmark harness refresh (v1.1.0, unreleased)

**Done:** `bench_util.hpp` (shared port/banners), `run_benchmarks_both.sh`; compare targets unified; ANALYSIS refreshed (wscpp linux vs ASIO transport); frame perf (mask unroll, outbound buffer reuse).

**State:** `master`, VERSION 1.1.0.

**Next:** LAN network benchmarks.

## 2026-06-07 — LAN compare benchmarks + doc sync (v1.1.0, unreleased)

**Done:** Full LAN compare suite (`bench_*_echo_server`, `bench_*_roundtrip_net`, `run_remote_network_compare.sh`); `ANALYSIS.md` LAN tables; doc audit (version 1.1.0, `error_code` API, 94 tests, fixed DEVELOPER targets table).

**State:** `master`, VERSION 1.1.0; LAN numbers measured on `deploy@192.168.1.165`.

**Next:** commit LAN benchmark + doc updates; optional CI job publishing LAN numbers on demand.

## 2026-06-07 — spdlog diagnostics backend (v1.1.0, unreleased)

**Done:** Optional internal logging via spdlog (`WSCPP_ENABLE_LOGGING`, default ON). Errors logged to stderr even without `on_error` callback; suppressed close/accept/shutdown failures instrumented. Public `wscpp::set_log_level()`. CI excludes third-party headers from clang-tidy.

**State:** `master`, VERSION 1.1.0.

## 2026-06-07 — R-A1: ws-rs Cargo workspace (feature/rust-impl)

**Done:** `rust/` workspace with `ws-rs` crate skeleton, `error` module, `rust/target/` gitignored.

**Verify:** `cd rust && cargo check -p ws-rs`.


## 2026-06-07 — R-B1: ws-rs frame layer (feature/rust-impl)

**Done:** RFC 6455 frame parser, builder, masking (`ws-rs/src/frame/`).

**Verify:** `cd rust && cargo check -p ws-rs`.


## 2026-06-07 — R-C2: ws-rs RFC 6455 regression (feature/rust-impl)

**Done:** Ported C++ `test_rfc6455_compliance.cpp` vectors (6 tests).

**Verify:** `cd rust && cargo test -p ws-rs --test rfc6455_compliance`.


## 2026-06-07 — R-B2: ws-rs handshake (feature/rust-impl)

**Done:** Sec-WebSocket-Key/Accept, HTTP upgrade parse/build, 4 unit tests.

**Verify:** `cd rust && cargo test -p ws-rs --test handshake`.


## 2026-06-07 — R-B4: ws-rs client/server API (feature/rust-impl)

**Done:** tokio `Connection`, `Client`, `Server`, echo session helper (ws:// only).

**Verify:** `cd rust && cargo check -p ws-rs`.


## 2026-06-07 — R-C1: ws-rs integration echo (feature/rust-impl)

**Done:** Local client↔server echo roundtrip test.

**Verify:** `cd rust && cargo test -p ws-rs --test integration_echo`.


## 2026-06-07 — R-D1: ws-rs echo examples (feature/rust-impl)

**Done:** `examples/echo_client`, `examples/echo_server` binaries.

**Verify:** `cd rust && cargo build -p echo_server -p echo_client`.


## 2026-06-07 — R-F1: ws-rs criterion benchmarks (feature/rust-impl)

**Done:** `frame_parse` and `roundtrip` criterion benches.

**Verify:** `cd rust && cargo bench -p ws-rs-benches --bench frame_parse`.


## 2026-06-07 — R-F0: Rust WebSocket catalog (feature/rust-impl)

**Done:** `ANALYSIS_RUST.md` — Tier 1/2 Rust libraries, ws-rs positioning, frame micro-benchmarks.


## 2026-06-07 — R-F3: C++ vs Rust comparison (feature/rust-impl)

**Done:** `ANALYSIS.md` section comparing wscpp v1.1.0 and ws-rs v0.1.0 (architecture, parity, benchmarks).


## 2026-06-07 — R-F2: ws-rs compare benchmarks (feature/rust-impl)

**Done:** `bench_latency` (p50/p99, 100 samples); tokio-tungstenite in `frame_parse` and `roundtrip` criterion benches; shared `ws-rs-benches` server helper.

**Verify:** `cd rust && cargo run -p ws-rs-benches --release --bin bench_latency`.


## 2026-06-07 — R-F3 update: benchmark numbers (feature/rust-impl)

**Done:** `ANALYSIS_RUST.md` echo p50/p99 + frame parse vs tungstenite; `ANALYSIS.md` C++ vs Rust table updated (ws-rs 0.32 ms p50).


## 2026-06-07 — R-G1: Rust CI job (feature/rust-impl)

**Done:** GitHub Actions `rust` job (`cargo test --workspace`); `rust/README.md`; root README link to ANALYSIS_RUST; CI triggers on `feature/rust-impl`.


## 2026-06-07 — fix: ws-rs incremental frame buffer (feature/rust-impl)

**Done:** `connection::read_frame` drains consumed bytes (fixes 64 KiB echo/throughput); `tests/throughput_echo.rs`.

**Verify:** `cd rust && cargo test -p ws-rs --test throughput_echo`.


## 2026-06-07 — R-F1 align: C++-parity benchmark harness (feature/rust-impl)

**Done:** `bench_frame_parse`, `bench_masking`, `bench_roundtrip`, `bench_echo_server`, `bench_roundtrip_net`, `bench_tokio_tungstenite_roundtrip`; `rust/benchmarks/run_benchmarks.sh`; removed criterion/bench_latency.

**Verify:** `bash rust/benchmarks/run_benchmarks.sh`.


## 2026-06-07 — docs: C++-parity Rust benchmark numbers (feature/rust-impl)

**Done:** `ANALYSIS_RUST.md`, `ANALYSIS.md`, `rust/README.md` updated for `bench_*` harness and localhost numbers.


## 2026-06-07 — R-B3: ws-rs wss TLS (feature/rust-impl)

**Done:** `wss://` via rustls/tokio-rustls; `ClientTlsConfig`/`ServerTlsConfig` (PEM, SNI, verify_none default); `transport::IoStream`; TLS→WS upgrade order; `tls_echo` tests (rcgen); `tls_client`/`tls_server` examples.

**Verify:** `cd rust && cargo test -p ws-rs --test tls_echo`.


## 2026-06-07 — R-F4: ws-rs LAN network compare (feature/rust-impl)

**Done:** `run_remote_network_compare.sh`; tokio-tungstenite `bench_*_echo_server` + `bench_*_roundtrip_net`; shared `tungstenite_bench` module.

**Verify:** `bash rust/benchmarks/run_remote_network_compare.sh` (needs reachable remote; `WSRS_BENCH_SKIP_BUILD=1` if pre-built).


## 2026-06-07 — R-G2: ws-rs SemVer versioning (feature/rust-impl)

**Done:** `rust/VERSION` 0.2.0; `scripts/bump_rust_version.sh`; `ws_rs::version` + `tests/version.rs`. Independent from wscpp `VERSION` (1.1.0).

**Verify:** `cd rust && cargo test -p ws-rs --test version`.


## 2026-06-07 — R-C3: ws-rs RFC 7692 permessage-deflate (feature/rust-impl)

**Done:** `extensions/permessage_deflate` (flate2); handshake negotiation; RSV1 on connection; `Client::enable_permessage_deflate()`; 3 unit + 1 integration test. v0.3.0.

**Verify:** `cd rust && cargo test -p ws-rs --test permessage_deflate --test deflate_echo`.


## 2026-06-07 — R-F4 run: LAN benchmark numbers (feature/rust-impl)

**Done:** Remote Rust install in `run_remote_network_compare.sh`; measured ws-rs p50=0.37 ms / 30 MB/s vs tokio-tungstenite 0.39 ms / 29 MB/s on `192.168.1.165`.

**Verify:** `bash rust/benchmarks/run_remote_network_compare.sh`.


## 2026-06-07 — R-F2: fastwebsockets + tokio-websockets compare (feature/rust-impl)

**Done:** `bench_fastwebsockets_*`, `bench_tokio_websockets_*` (localhost + LAN); `run_benchmarks.sh` / `run_remote_network_compare.sh` updated.

**Verify:** `cd rust && cargo build --release -p ws-rs-benches --bins && ./target/release/bench_fastwebsockets_roundtrip`.


## 2026-06-07 — R-F4 run: full LAN compare 4 libs (feature/rust-impl)

**Done:** `run_remote_network_compare.sh` — ws-rs, tokio-tungstenite, fastwebsockets, tokio-websockets on `192.168.1.165`; ANALYSIS_RUST.md + ANALYSIS.md LAN tables.

**Numbers:** ws-rs p50=0.338 ms / 32.29 MB/s; tokio-tungstenite p50=0.347 ms / 32.95 MB/s; fastwebsockets p50=0.333 ms; tokio-websockets p50=0.372 ms.


## 2026-06-07 — R-C4: ws-rs UTF-8 validation §8.1 (feature/rust-impl)

**Done:** Ported `wscpp/detail/utf8.hpp` validator; invalid TEXT → close 1007; 7 unit + 1 integration test. v0.3.1.

**Verify:** `cd rust && cargo test -p ws-rs --test utf8 --test invalid_utf8`.


## 2026-06-07 — R-PARITY: ws-rs C++ feature parity (feature/rust-impl)

**Done:**

| Area | C++ reference | ws-rs |
|------|---------------|-------|
| Mask XOR unroll | `detail/mask.hpp` | `frame/mask.rs` — ~50 GB/s (was ~7 GB/s) |
| Outbound buffer reuse | `build_into()` + `outbound_frame_` | `FrameBuilder::build_into*`, `Connection::outbound` |
| Fragmentation | `read_message`, `send_continuation` | `read_message()`, `send_continuation()` |
| Close handshake | echo + 1002 on protocol error | `handle_close_frame`, `fail_protocol` → 1002 |
| Close codes | `sanitize_close_code` | `close.rs` + `tests/close_codes.rs` |
| Integration tests | `FragmentedTextEcho`, `ServerPingGetsPong` | `fragmented_echo.rs`, `ping_pong.rs` |
| Minimal transport | `WSCPP_USE_ASIO=OFF` / `linux_socket` | `ws_rs::blocking` (`std::net`, no tokio) |
| Optional deps | CMake flags | Cargo features: `async`, `tls`, `deflate`, `std-blocking` |

**Verify:**

```bash
cd rust && cargo test -p ws-rs
cargo test -p ws-rs --no-default-features --features "std-blocking,deflate"
cargo build --release -p ws-rs-benches --bin bench_masking && ./target/release/bench_masking
```

**Remaining gaps vs C++:** callback API (`std::error_code` style stays async-pull in Rust); integration tests should run with `--test-threads=1` (port races in parallel).


## 2026-06-07 — R-PARITY-2: blocking wss + stress tests (feature/rust-impl)

**Done:** `blocking-tls` feature — sync `wss://` via rustls `StreamOwned`; `tests/blocking_tls_echo.rs`. Ported C++ stress scenarios (`parallel_clients_echo`, `large_binary_message`, `rapid_small_messages`) behind optional `stress` feature. TLS config split to `tls/config.rs` (shared async + blocking).

**Verify:**

```bash
cd rust && cargo test -p ws-rs -- --test-threads=1
cargo test -p ws-rs --features stress --test stress
```


## 2026-06-07 — R-F5: post-parity benchmarks + CI (feature/rust-impl)

**Done:** Re-ran localhost + LAN benches after mask/fragmentation optimizations. Added `bench_blocking_roundtrip` (p50=0.25 ms, 79 MB/s, 687 KB binary). CI Rust job: `cargo test --workspace -- --test-threads=1`. `blocking::Client::recv_binary()`.

**LAN (192.168.1.165):** ws-rs p50=0.367 ms / 30.08 MB/s; fastwebsockets p50=0.348 ms; tokio-tungstenite 26.01 MB/s.

**Verify:** `bash rust/benchmarks/run_benchmarks.sh` && `bash rust/benchmarks/run_remote_network_compare.sh`.


## 2026-06-07 — R-F6: blocking LAN + v0.4.0 (feature/rust-impl)

**Done:** `bench_blocking_echo_server` + `bench_blocking_roundtrip_net` in LAN harness. `serial_test` on network integration tests (parallel `cargo test` OK). Bump `rust/VERSION` → **0.4.0**.

**LAN blocking:** p50=0.399 ms / 29.85 MB/s (on par with tokio ws-rs 0.345 ms).

**Verify:** `bash rust/benchmarks/run_remote_network_compare.sh` && `cd rust && cargo test -p ws-rs`.


## 2026-06-07 — R-DOC: Rust docs + fmt/clippy CI (feature/rust-impl)

**Done:** [docs/RUST.md](RUST.md) developer guide; updated [rust/README.md](../rust/README.md); `scripts/ci/check-rust.sh`; `rust/rustfmt.toml`; workspace `lints`; CI fmt+clippy+doc; rustdoc on public API (`Client`, `Server`, `Connection`, `frame`, `blocking`).

**Verify:** `./scripts/ci/check-rust.sh`


## 2026-06-07 — R-DOC-2: full public rustdoc + missing_docs (feature/rust-impl)

**Done:** Documented all ~160 public items (`Client`/`Connection`/`Server`, `frame`, `handshake`, `blocking/*`, `transport`, TLS helpers, extensions). Fixed crate-level doc in `lib.rs` (Features table inside `//!`). Enabled `#![warn(missing_docs)]` — enforced via CI `RUSTFLAGS="-D warnings"`.

**Verify:** `./scripts/ci/check-rust.sh`


## 2026-06-07 — Merge feature/rust-impl → master

**Done:** Rebased ws-rs workspace onto `master` (includes spdlog logging + CI third-party exclusions). Squashed 31 rust commits into one merge commit. `rust/ws-rs` v0.4.0; CI Rust job on `master` only; docs synced (README, CHANGELOG, RUST.md, DEVELOPER.md).

**Verify:** `./scripts/ci/check-rust.sh` && `cd build && ctest --output-on-failure`

**State:** `master`, wscpp 1.1.0 + ws-rs 0.4.0.

**Next:** optional tag `ws-rs-v0.4.0`; batch wscpp release when unreleased C++ items are tagged.

## 2026-06-07 — R-PERF: ws-rs speed optimizations (feature/rust-impl)

**Done:** Implemented known Rust speed optimizations (uncommitted, on `feature/rust-impl`):
- `[profile.release]` in `rust/Cargo.toml`: `lto = "fat"`, `codegen-units = 1`, `opt-level = 3`, `strip = true` (tests stay on dev profile, panic=unwind preserved).
- Frame builder single-copy encoder (`frame/builder.rs` `encode_into`/`push_header`): payload copied once into `out`, masked in place; routed `build_into_opcode`, `build_close_into`, `build_*` through it (was up to 3 payload copies).
- `Parser::take_frame()` (`mem::take` payload, no clone); used by both connections' `read_frame`.
- Persistent cursor read buffer (`ReadState` in async + blocking `Connection`): no per-call `vec![0;4096]`/`Vec::new()`, 64 KiB read chunk, cursor avoids O(n) `drain` per byte; also fixes latent loss of frames pipelined after a completed frame in one `read()`.
- `send_data_frame` uses `Cow<[u8]>` — no `to_vec()` on the uncompressed path (async + blocking).
- Fragmented reassembly delivers via `mem::take` instead of `frag.buffer.clone()`.

**Measured (localhost, noisy micro windows):** `frame_build` ~9.3 → ~13–17 GB/s; `mask_unmask_xor` ~48 → ~54–60 GB/s. Echo latency/throughput within localhost noise (network-bound).

**Verify:** `cd rust && cargo test -p ws-rs -- --test-threads=1` (all pass) `&& cargo clippy --workspace --all-features` (clean) `&& cargo build --release -p ws-rs-benches --bins`.

**Next:** re-run LAN compare (`run_remote_network_compare.sh`) for fresh numbers; optionally document `RUSTFLAGS="-C target-cpu=native"` for mask auto-vectorization; bump `rust/VERSION` if releasing.

## 2026-06-07 — R-PERF docs: analysis + article updated (feature/rust-impl)

**Done:** Documented the v0.4.x speed pass with fair before/after numbers (warm medians, same methodology — measured baseline by `git stash`-ing the changes and rebuilding).
- `ANALYSIS_RUST.md`: new "Speed optimizations (v0.4.x)" section; before/after micro table; executive summary + echo tables updated (`frame_build` 8886→16139, mask 49664→62646, 64 KiB 74→78 tokio / 79→84 blocking); footnote [c] that `frame_parse` is memcpy-bound/unchanged (±15 % WSL2 noise); changelog row.
- `ANALYSIS.md`: C++ vs Rust localhost table refreshed (ws-rs build 16139, parse 17146, 64 KiB 78) + footnote [e]; changelog row.
- `.cursor/habr-websocket-cpp-rust.md`: Part 2/Part 3 throughput tables, 3-column micro table, new "Что дал проход оптимизаций ws-rs (v0.4.x)" subsection, ws-rs pluses updated.

**Notes:** Latency p50 unchanged (network/syscall-bound). `frame_parse` reported lower than old doc (19701→17146) but parser path untouched — labelled as noise/memcpy-bound, not a regression.

**Next:** still uncommitted; commit on `feature/rust-impl` when asked; optional LAN re-run + `rust/VERSION` bump.
