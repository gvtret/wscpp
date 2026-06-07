#!/usr/bin/env bash
# LAN comparative benchmarks for ws-rs: echo servers on remote host, clients locally.
# Mirrors benchmarks/run_remote_network_compare.sh (C++ wscpp).
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
remote_user_host="${WSRS_BENCH_REMOTE:-${WSCPP_BENCH_REMOTE:-deploy@192.168.1.165}}"
remote_host="${WSRS_BENCH_HOST:-${WSCPP_BENCH_HOST:-192.168.1.165}}"
remote_dir="${WSRS_BENCH_REMOTE_DIR:-${WSCPP_BENCH_REMOTE_DIR:-/home/deploy/ws-rs-bench}}"
port="${WSRS_BENCH_PORT:-${WSCPP_BENCH_PORT:-19081}}"
samples="${WSRS_BENCH_SAMPLES:-${WSCPP_BENCH_SAMPLES:-100}}"
skip_build="${WSRS_BENCH_SKIP_BUILD:-${WSCPP_BENCH_SKIP_BUILD:-0}}"
local_target_dir="${WSRS_BENCH_LOCAL_TARGET:-$root/target/release}"
remote_target_dir="${WSRS_BENCH_REMOTE_TARGET:-$remote_dir/target/release}"
pid_file="/tmp/wsrs-bench-server.pid"
server_log="/tmp/wsrs-bench-server.log"
results_tsv="${WSRS_BENCH_RESULTS:-/tmp/wsrs-net-bench.tsv}"

bench_bins=(
    bench_echo_server
    bench_roundtrip_net
    bench_blocking_echo_server
    bench_blocking_roundtrip_net
    bench_tokio_tungstenite_echo_server
    bench_tokio_tungstenite_roundtrip_net
    bench_fastwebsockets_echo_server
    bench_fastwebsockets_roundtrip_net
    bench_tokio_websockets_echo_server
    bench_tokio_websockets_roundtrip_net
)

echo "=== ws-rs LAN comparative benchmarks ==="
echo "remote: $remote_user_host"
echo "client -> $remote_host:$port"
echo "samples: $samples"

install_remote_deps() {
    ssh "$remote_user_host" 'sudo apt-get update -qq && sudo DEBIAN_FRONTEND=noninteractive apt-get install -y -qq \
        build-essential curl pkg-config libssl-dev rsync'
}

install_remote_rust() {
    ssh "$remote_user_host" 'if ! command -v cargo >/dev/null 2>&1; then
        curl --proto "=https" --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain stable
    fi
    . "$HOME/.cargo/env"
    rustc --version
    cargo --version'
}

sync_sources() {
    rsync -az --delete \
        --exclude '.git' \
        --exclude 'target' \
        "$root/" "$remote_user_host:$remote_dir/"
}

build_local_clients() {
    cargo build --release -p ws-rs-benches --bins
}

build_remote_servers() {
    ssh "$remote_user_host" ". \"\$HOME/.cargo/env\" && cd '$remote_dir' && cargo build --release -p ws-rs-benches \
        --bin bench_echo_server \
        --bin bench_blocking_echo_server \
        --bin bench_tokio_tungstenite_echo_server \
        --bin bench_fastwebsockets_echo_server \
        --bin bench_tokio_websockets_echo_server"
}

stop_remote_server() {
    ssh "$remote_user_host" "if [ -f '$pid_file' ]; then
        kill \"\$(cat '$pid_file')\" 2>/dev/null || true
        rm -f '$pid_file'
    fi"
}

start_remote_server() {
    local server_bin="$1"
    shift
    stop_remote_server
    ssh "$remote_user_host" "nohup '$remote_target_dir/$server_bin' $* \
        </dev/null >'$server_log' 2>&1 & echo \$! > '$pid_file'; sleep 0.8"
    if ! ssh "$remote_user_host" "ss -ltn | grep -q ':$port '"; then
        ssh "$remote_user_host" "cat '$server_log'" >&2 || true
        echo "server $server_bin failed to listen on :$port" >&2
        return 1
    fi
}

metric_p50() {
    echo "$1" | grep -E 'echo_latency|connect_latency' | sed -n 's/.*p50=\([0-9.]*\) ms.*/\1/p' | head -1 || true
}

metric_p99() {
    echo "$1" | grep -E 'echo_latency|connect_latency' | sed -n 's/.*p99=\([0-9.]*\) ms.*/\1/p' | head -1 || true
}

metric_tp() {
    echo "$1" | grep 'echo_throughput_64k' | sed -n 's/.* \([0-9.]*\) MB\/s.*/\1/p' | head -1 || true
}

run_case() {
    local label="$1"
    local server_bin="$2"
    local client_bin="$3"
    local server_args="$4"

    if [[ ! -x "$local_target_dir/$client_bin" ]]; then
        echo "skip $label (missing local $client_bin)"
        return 0
    fi
    if ! ssh "$remote_user_host" "test -x '$remote_target_dir/$server_bin'"; then
        echo "skip $label (missing remote $server_bin)"
        return 0
    fi

    echo
    echo "---------- $label ----------"
    start_remote_server "$server_bin" $server_args

    local out
    if ! out="$("$local_target_dir/$client_bin" "$remote_host" "$port" "$samples" 2>&1)"; then
        echo "$out"
        echo "FAIL $label" >&2
        return 1
    fi
    echo "$out"

    local p50 p99 tp
    p50="$(metric_p50 "$out")"
    p99="$(metric_p99 "$out")"
    tp="$(metric_tp "$out")"
    [[ -z "$p50" ]] && p50="—"
    [[ -z "$p99" ]] && p99="—"
    [[ -z "$tp" ]] && tp="—"
    printf '%s\t%s\t%s\t%s\t%s\n' "$label" "$p50" "$p99" "$tp" "$client_bin" >>"$results_tsv"
}

print_summary() {
    echo
    echo "=== LAN summary (p50/p99 ms, 64 KiB MB/s) ==="
    printf '%-28s %8s %8s %10s\n' "Library" "p50" "p99" "64KiB"
    while IFS=$'\t' read -r label p50 p99 tp _; do
        printf '%-28s %8s %8s %10s\n' "$label" "$p50" "$p99" "$tp"
    done <"$results_tsv"
}

cd "$root"

if [[ "$skip_build" != "1" ]]; then
    install_remote_deps
    install_remote_rust
    sync_sources
    build_remote_servers
    build_local_clients
else
    echo "skip install/sync/build (WSRS_BENCH_SKIP_BUILD=1)"
fi

: >"$results_tsv"
trap stop_remote_server EXIT

bench_failed=0
run_case "ws-rs" bench_echo_server bench_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "ws-rs (blocking)" bench_blocking_echo_server bench_blocking_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "tokio-tungstenite" bench_tokio_tungstenite_echo_server \
    bench_tokio_tungstenite_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "fastwebsockets" bench_fastwebsockets_echo_server \
    bench_fastwebsockets_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "tokio-websockets" bench_tokio_websockets_echo_server \
    bench_tokio_websockets_roundtrip_net "$port 0.0.0.0" || bench_failed=1

print_summary
exit "$bench_failed"
