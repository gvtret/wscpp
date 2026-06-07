# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Internal logging backend (spdlog, `WSCPP_ENABLE_LOGGING`, default ON): all library errors including previously suppressed I/O/accept paths; public `wscpp::set_log_level()`
- LAN comparative benchmarks: `bench_*_echo_server`, `bench_*_roundtrip_net`, `run_remote_network_compare.sh`
- Linux POSIX socket transport (`WSCPP_USE_ASIO=OFF`) with OpenSSL TLS on Linux
- `wscpp::errc` / `std::error_code` throughout public API (no exceptions in library I/O)
- TLS integration tests with local PKI generation (`scripts/gen-test-tls-certs.sh`)
- Dual-transport benchmark harness (`run_benchmarks_both.sh`, `bench_util.hpp`)

### Fixed
- Linux server accept loop blocking and concurrent socket writes causing flaky integration tests
- Client WSS connect order (TCP connect before TLS enable on linux path)

### Changed
- `ANALYSIS.md` dual-transport and LAN benchmark tables; table legend for missing values
- Integration tests default CTest timeout 30s

## [1.1.0] - 2026-06-07

### Added
- RFC 7692 permessage-deflate extension (zlib); server auto-accepts client offer
- `client::enable_permessage_deflate()` opt-in; RSV1 compress/decompress on TEXT/BINARY
- Unit tests (RFC 7692 "Hello" vector) and integration `DeflateTextEcho`
- CMake option `WSCPP_ENABLE_DEFLATE` (default ON, requires zlib)

## [1.0.2] - 2026-06-07

### Added
- UTF-8 validation for TEXT frames (RFC 6455 §8.1); invalid payload closes with code 1007
- `detail::is_valid_utf8()` helper and unit tests
- Integration test `InvalidUtf8TextClosesWith1007`

### Fixed
- `fail_with_close()` now invokes `on_close` callback (local protocol errors)

## [1.0.1] - 2026-06-07

### Added
- RFC 6455 client masking, fragmented message reassembly, auto pong, protocol validation
- `send_continuation`, `send_ping`, `connection_role` API
- Integration tests for fragmented echo and server ping

### Changed
- `ANALYSIS.md` benchmark results refreshed after RFC gate (post-masking baseline)

## [1.0.0] - 2026-06-07

First stable release: full WebSocket client/server stack, TLS, tests, docs, and benchmarks.

### Added

- WebSocket client and server (`ws://`, `wss://`) with callback API
- RFC 6455 frame parser/builder, handshake module (Sec-WebSocket-Key/Accept)
- Connection layer with background reader thread
- Standalone ASIO 1.20 (FetchContent) and OpenSSL TLS integration
- SNI hostname for WSS clients (RFC 2818)
- Examples: echo client/server, TLS client/server with self-signed cert
- Unit, integration, regression, and optional stress tests (79 tests)
- Micro-benchmarks: frame parse/build, masking, echo latency
- Comparative benchmark vs websocketpp 0.8.2
- User guide (`docs/README.md`), developer guide (`docs/DEVELOPER.md`)
- Mandatory Doxygen API documentation (`cmake --build build --target docs`)
- `ANALYSIS.md` — **`C++11`** WebSocket library catalog and benchmark results
- `scripts/bump_version.sh`, GitHub Actions CI workflow
- Issue/PR templates and Code of Conduct

### Fixed

- Frame parser byte accounting for masked and partial frames
- Server TLS handshake before WebSocket upgrade
- Server `activate()` after `on_connection` callback
- Connection `close()` deadlock when invoked from reader thread
- CMake: all library sources linked; examples parent CMakeLists
- SemVer macros generated from `VERSION` file

## [0.1.0] - 2026-06-06

### Added

- Initial WebSocket stack scaffold

[Unreleased]: https://github.com/gvtret/wscpp/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/gvtret/wscpp/compare/v1.0.2...v1.1.0
[1.0.2]: https://github.com/gvtret/wscpp/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/gvtret/wscpp/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/gvtret/wscpp/compare/v0.1.0...v1.0.0
[0.1.0]: https://github.com/gvtret/wscpp/releases/tag/v0.1.0
