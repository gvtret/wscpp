# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
- `ANALYSIS.md` — C++11 WebSocket library catalog and benchmark results
- `scripts/bump_version.sh`, GitVerse CI workflow
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

[Unreleased]: https://gitverse.ru/project/wscpp/compare/v1.0.0...HEAD
[1.0.0]: https://gitverse.ru/project/wscpp/compare/v0.1.0...v1.0.0
[0.1.0]: https://gitverse.ru/project/wscpp/releases/tag/v0.1.0
