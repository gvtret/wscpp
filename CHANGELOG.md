# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Handshake module with RFC 6455 Sec-WebSocket-Key/Accept (SHA-1 + Base64)
- Integration test: local client/server echo
- Regression tests for RFC 6455 section 5.7 frame examples
- `scripts/bump_version.sh` for SemVer bumps

### Fixed
- Frame parser: accumulate `consumed` bytes correctly across header, mask key, and payload states
- Frame parser: set `payload_remaining_` when entering payload state
- Connection reader thread and server `activate()` ordering
- CMake: link all library sources; examples parent CMakeLists
- SemVer macros generated from `VERSION` file

## [0.1.0] - 2026-06-06

### Added
- Initial WebSocket stack: client, server, connection, frame parser/builder
- Standalone ASIO 1.20 integration via FetchContent
- OpenSSL TLS support (`ssl_context`, wss-ready socket layer)
- Unit tests for version, ASIO, OpenSSL, frames, connection
- Example client and server programs

[Unreleased]: https://github.com/example/wscpp/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/example/wscpp/releases/tag/v0.1.0
