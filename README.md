# wscpp

Lightweight WebSocket stack for **C++11** with optional TLS (`wss://`).

[![CI](https://gitverse.ru/project/wscpp/badges/main/pipeline.svg)](https://gitverse.ru/project/wscpp)

## Features

- C++11, no Boost required (standalone ASIO 1.20 via FetchContent)
- OpenSSL TLS for secure WebSocket
- Client and server with callback-based API
- RFC 6455 frame layer with regression test vectors
- CMake build, SemVer versioning, Doxygen API docs

**Version:** 0.1.0 (pre-1.0 development)

## Quick build

```bash
git clone https://gitverse.ru/project/wscpp.git
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
| [Analysis](ANALYSIS.md) | C++11 WebSocket library comparison |
| [Changelog](CHANGELOG.md) | Release history |
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
