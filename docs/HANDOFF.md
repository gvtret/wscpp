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
