# wscpp — handoff log

## 2026-06-07 — Frame parser fix + handshake tests

**Done:** Fixed frame parser byte accounting (`consumed +=` in all sub-parsers); all 52 tests pass. Added `CHANGELOG.md`, `scripts/bump_version.sh` with `--dry-run`, `tests/unit/test_handshake.cpp` (9 tests, RFC 6455 §4.1.5 accept vector).
**State:** Branch `master`, VERSION `0.1.0`, build green (`ctest` 61/61 with handshake tests). Many uncommitted changes from A1–C2 work.
**Next:** Git commits per plan steps (A1–A4, B1–B2); then B3 TLS audit, B4 client/server unit tests.
**Notes:** Parser bug was `consumed = N` instead of `consumed += N` — broke masked frames and partial parse.
