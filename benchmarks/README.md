# Benchmarks

Micro-benchmarks and comparative latency tests. Build with `-DWSCPP_BUILD_BENCHMARKS=ON`.

## When to run

**Conduct publishable comparative benchmarks after all RFC-scoped features are implemented and tested** (RFC 6455 core, RFC 2818 for WSS). Numbers in `ANALYSIS.md` are refreshed via `run_benchmarks`.

## Targets

| Binary | Purpose |
|--------|---------|
| `bench_frame_parse` | 1 MiB frame build + parse throughput |
| `bench_masking` | Mask/unmask and masked round-trip |
| `bench_roundtrip` | Local echo latency + 64 KiB throughput (wscpp) |
| `bench_echo_server` | Echo server for remote/LAN benchmarks (bind + port args) |
| `bench_roundtrip_net` | Client-only echo latency + 64 KiB vs remote host |
| `bench_*_roundtrip` | Compare echo latency vs other libraries |

### wscpp transport backends

`bench_roundtrip` reports the active backend in its banner:

| `WSCPP_USE_ASIO` | Transport | Notes |
|------------------|-----------|-------|
| ON (default) | standalone ASIO 1.20 | FetchContent when benchmarks enabled |
| OFF (Linux) | POSIX sockets + OpenSSL | Smaller `bench_roundtrip` binary, no ASIO in lib |

Compare targets (websocketpp, IXWebSocket, …) always use their own stack; only wscpp-linked targets (`bench_roundtrip`, `bench_easywsclient_connect`) follow `WSCPP_USE_ASIO`.

## Build and run

```bash
cmake -B build -DWSCPP_BUILD_BENCHMARKS=ON
cmake --build build --target benchmarks compare_benchmarks
cmake --build build --target run_benchmarks
# both wscpp transports: bash benchmarks/run_benchmarks_both.sh   # executes all binaries in build/bin
```

Or manually:

```bash
./build/bin/bench_frame_parse
./build/bin/bench_masking
./build/bin/bench_roundtrip
```

Linux socket transport:

```bash
cmake -B build-linux -DWSCPP_USE_ASIO=OFF -DWSCPP_BUILD_BENCHMARKS=ON
cmake --build build-linux --target run_benchmarks
```

Shared helpers: `bench_common.hpp` (stats), `bench_util.hpp` (`pick_free_port`, banners).

### Remote / LAN roundtrip

Full comparative suite (wscpp both transports + websocketpp, IXWebSocket, libwebsockets, Beast, SWS, easywsclient connect). Echo servers run on a remote host; clients run locally:

```bash
bash benchmarks/run_remote_network_compare.sh
WSCPP_BENCH_SKIP_BUILD=1 bash benchmarks/run_remote_network_compare.sh   # re-run only
```

CMake targets: `compare_net_servers` (remote side), `compare_net_clients` (local side).

Environment overrides: `WSCPP_BENCH_REMOTE` (default `deploy@192.168.1.165`), `WSCPP_BENCH_HOST`, `WSCPP_BENCH_PORT`, `WSCPP_BENCH_SAMPLES`, `WSCPP_USE_ASIO`, `WSCPP_BENCH_SKIP_BUILD`.

Micro-benchmarks (`bench_frame_parse`, `bench_masking`) are CPU-only and stay on localhost.

Comparative setup: `compare/README.md`.
