# wscpp — handoff log

## 2026-06-07 — Frame parser fix + handshake tests

**Done:** Fixed frame parser byte accounting (`consumed +=` in all sub-parsers); all 52 tests pass. Added `CHANGELOG.md`, `scripts/bump_version.sh` with `--dry-run`, `tests/unit/test_handshake.cpp` (9 tests, RFC 6455 §4.1.5 accept vector).
## 2026-06-07 — C3–G1: stress, examples, docs, Doxygen, CI

**Done:** Stress tests (3 scenarios, `WSCPP_ENABLE_STRESS_TESTS`); server TLS handshake before WS upgrade; tls_client/tls_server examples; D1 examples README; D3 `WSCPP_BUILD_BENCHMARKS`; E1/E2 user+dev docs; E3–E4 Doxygen config + public API comments; F0 ANALYSIS.md draft; G1 `.gitverse/workflows/ci.yml`; root README update. 79/79 tests pass; docs target builds.
**State:** `main`, v0.1.0, examples verified ws+wss locally.
**Next:** F1 benchmarks harness; G2 repo templates; G4 release v1.0.0.
**Notes:** Server WSS path: adopt socket → TLS handshake → WS upgrade. Stress tests off in CI (`WSCPP_ENABLE_STRESS_TESTS=OFF`).
