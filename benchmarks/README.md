# Benchmarks

Micro-benchmarks and comparative latency tests. Build with `-DWSCPP_BUILD_BENCHMARKS=ON`.

## When to run

**Conduct publishable comparative benchmarks after all RFC-scoped features are implemented and tested** (RFC 6455 core, RFC 2818 for WSS). The current harness is useful during development; numbers in `ANALYSIS.md` are marked preliminary until a post-RFC re-run.

See `ANALYSIS.md` → *When to benchmark* for the gate checklist.

## Targets

| Binary | Purpose |
|--------|---------|
| `bench_frame_parse` | 1 MiB frame build + parse throughput |
| `bench_masking` | Mask/unmask and masked round-trip |
| `bench_roundtrip` | Local echo latency (wscpp) |
| `bench_websocketpp_roundtrip` | Echo latency vs websocketpp 0.8.2 |

```bash
cmake -B build -DWSCPP_BUILD_BENCHMARKS=ON
cmake --build build --target benchmarks bench_websocketpp_roundtrip
./build/bin/bench_frame_parse
./build/bin/bench_masking
./build/bin/bench_roundtrip
./build/bin/bench_websocketpp_roundtrip
```

Comparative setup for other libraries: `compare/README.md`.
