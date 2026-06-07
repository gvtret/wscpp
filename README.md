# wscpp

Lightweight WebSocket stack for **`C++11`** with optional TLS (`wss://`).

> **Experimental / AI-built.** wscpp is an experimental library, not a production-grade product. The entire codebase was created with AI coding agents (LLMs); humans curate tests, CI, and releases, but you should expect gaps, API churn, and undiscovered bugs. Do not use in production without independent review.

[![CI](https://github.com/gvtret/wscpp/actions/workflows/ci.yml/badge.svg)](https://github.com/gvtret/wscpp/actions/workflows/ci.yml)

## Features

- **`C++11`**, no Boost required (standalone ASIO 1.20 via FetchContent, or Linux POSIX sockets)
- OpenSSL TLS for secure WebSocket
- Client and server with callback-based API; `std::error_code` on I/O (no exceptions)
- RFC 6455 framing + RFC 7692 permessage-deflate (opt-in)
- CMake build, SemVer versioning, Doxygen API docs

**Version:** 1.1.0

## Quick build

```bash
git clone https://github.com/gvtret/wscpp.git
cd wscpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ctest --output-on-failure
```

## Documentation

| Document | Description |
|----------|-------------|
| [User guide](docs/README.md) | Install, quick start, FAQ |
| [Developer guide](docs/DEVELOPER.md) | Architecture, testing, releases |
| [Examples](examples/README.md) | ws:// and wss:// echo programs |
| [Analysis](ANALYSIS.md) | **`C++11`** WebSocket library comparison |
| [Changelog](CHANGELOG.md) | Release history |
| [AGENTS.md](AGENTS.md) | Guidelines for AI coding agents working on this repo |
| API reference | `cmake --build build --target docs` → `build/docs/html/index.html` |

## Examples

```bash
# Terminal 1
./build/bin/wscpp_example_server 8080

# Terminal 2
./build/bin/wscpp_example_client ws://127.0.0.1:8080/
```

See [examples/README.md](examples/README.md) for TLS (wss://) setup.

## License

MIT — see [LICENSE](LICENSE).
