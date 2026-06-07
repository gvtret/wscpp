# ws-rs (Rust)

RFC 6455 WebSocket client/server — Rust companion to [wscpp](../README.md).

**Version:** `0.4.0` (`rust/VERSION`). Bump: `./scripts/bump_rust_version.sh patch|minor|major`.

**Developer guide:** [docs/RUST.md](../docs/RUST.md) (architecture, features, fmt/clippy/test, releases).

## Quick start

```bash
cd rust
cargo test --workspace -- --test-threads=1
```

### Async (`ws://`)

```bash
cargo run -p echo_server --release -- 8080
cargo run -p echo_client --release -- ws://127.0.0.1:8080/ hello
```

```rust
use ws_rs::client::Client;

#[tokio::main]
async fn main() -> ws_rs::Result<()> {
    let mut client = Client::new();
    client.connect("ws://127.0.0.1:8080/").await?;
    client.send_text("ping").await?;
    let _ = client.recv_text().await?;
    client.close().await?;
    Ok(())
}
```

### Blocking (`std::net`, no tokio)

```rust
use ws_rs::blocking::Client;

fn main() -> ws_rs::Result<()> {
    let mut client = Client::new();
    client.connect("ws://127.0.0.1:8080/")?;
    client.send_text("ping")?;
    let _ = client.recv_text()?;
    client.close()?;
    Ok(())
}
```

### TLS (`wss://`)

```bash
# From repo root: bash scripts/gen-test-tls-certs.sh
cargo run -p tls_server --release -- --cert ../tests/tls/fixtures/server.crt \
    --key ../tests/tls/fixtures/server.key --port 8443
cargo run -p tls_client --release -- wss://127.0.0.1:8443/ hello
```

`ClientTlsConfig` / `ServerTlsConfig` — PEM loading; default client uses `verify_none` (same as C++).

`Client::enable_permessage_deflate(true)` — RFC 7692 (negotiated when server accepts).

## Quality checks

```bash
./scripts/ci/check-rust.sh    # fmt + clippy (-D warnings) + test + doc
cargo doc -p ws-rs --no-deps --open
```

## Benchmarks

Same targets as C++ `benchmarks/` (see [benchmarks/README.md](../benchmarks/README.md)):

| Binary | Purpose |
|--------|---------|
| `bench_frame_parse` | 1 MiB frame build + parse |
| `bench_masking` | mask/unmask throughput |
| `bench_roundtrip` | tokio echo p50/p99 + 64 KiB |
| `bench_blocking_roundtrip` | sync `std::net` echo (C++ linux_socket parity) |
| `bench_echo_server` / `bench_roundtrip_net` | LAN tokio |
| `bench_blocking_echo_server` / `bench_blocking_roundtrip_net` | LAN blocking |
| `bench_*_roundtrip` | compare peers (tokio-tungstenite, fastwebsockets, tokio-websockets) |

```bash
bash benchmarks/run_benchmarks.sh
bash benchmarks/run_remote_network_compare.sh
```

Numbers: [ANALYSIS_RUST.md](../ANALYSIS_RUST.md).
