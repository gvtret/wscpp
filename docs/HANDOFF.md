# wscpp — handoff log

## 2026-06-07 — Frame parser fix + handshake tests

**Done:** Fixed frame parser byte accounting (`consumed +=` in all sub-parsers); all 52 tests pass. Added `CHANGELOG.md`, `scripts/bump_version.sh` with `--dry-run`, `tests/unit/test_handshake.cpp` (9 tests, RFC 6455 §4.1.5 accept vector).
## 2026-06-07 — C3–G1: stress, examples, docs, Doxygen, CI

**Done:** Stress tests (3 scenarios, `WSCPP_ENABLE_STRESS_TESTS`); server TLS handshake before WS upgrade; tls_client/tls_server examples; D1 examples README; D3 `WSCPP_BUILD_BENCHMARKS`; E1/E2 user+dev docs; E3–E4 Doxygen config + public API comments; F0 ANALYSIS.md draft; G1 `.gitverse/workflows/ci.yml`; root README update. 79/79 tests pass; docs target builds.
**State:** `main`, v0.1.0, examples verified ws+wss locally.
**Next:** F1 benchmarks harness; G2 repo templates; G4 release v1.0.0.
**Notes:** Server WSS path: adopt socket → TLS handshake → WS upgrade. Stress tests off in CI (`WSCPP_ENABLE_STRESS_TESTS=OFF`).

## 2026-06-07 — F1–G4: benchmarks, templates, v1.0.0

**Done:** F1 micro-benchmarks (frame/mask/roundtrip); F2 websocketpp compare; F3 ANALYSIS.md with measured results; G2 issue/PR templates + CODE_OF_CONDUCT; G4 release v1.0.0. 79/79 tests, docs target OK.
**State:** `main`, VERSION 1.0.0, tag `v1.0.0`.
**Next:** Optional F2 expansion (Beast/IXWebSocket); post-v1.0 features (permessage-deflate).
**Notes:** Roundtrip bench must not call `client.run()` — io_context is idle; use wait loop like integration tests.

## 2026-06-07 — RFC 6455 protocol completion (post-v1.0.0)

**Done:** Client outbound masking; server/client inbound mask validation; RSV/opcode checks; fragmented message reassembly; auto pong; close code validation; `send_continuation`/`send_ping`; integration tests (fragmented echo, ping). 82/82 tests pass.
**State:** `main`, VERSION 1.0.0; RFC gate ready for benchmark re-run (except RFC 7692).
**Next:** Optional UTF-8 text validation (RFC 6455 §8.1); F2 expansion (Beast/IXWebSocket).

## 2026-06-07 — Benchmark re-run post-RFC gate

**Done:** Re-ran all benchmarks; updated ANALYSIS.md with post-RFC numbers; release v1.0.1. 82/82 tests pass.
**State:** `main`, VERSION 1.0.1, tag `v1.0.1`.
**Next:** UTF-8 validation; optional comparative benchmarks for external libs.
