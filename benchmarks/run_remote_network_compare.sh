#!/usr/bin/env bash
# LAN comparative benchmarks: library echo servers on remote host, clients locally.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
remote_user_host="${WSCPP_BENCH_REMOTE:-deploy@192.168.1.165}"
remote_host="${WSCPP_BENCH_HOST:-192.168.1.165}"
remote_dir="${WSCPP_BENCH_REMOTE_DIR:-/home/deploy/wscpp-bench}"
port="${WSCPP_BENCH_PORT:-19081}"
samples="${WSCPP_BENCH_SAMPLES:-100}"
skip_build="${WSCPP_BENCH_SKIP_BUILD:-0}"
local_build_asio="${WSCPP_BENCH_LOCAL_BUILD:-$root/build-net-compare}"
local_build_linux="${WSCPP_BENCH_LOCAL_BUILD_LINUX:-$root/build-net-compare-linux}"
remote_build_asio="${WSCPP_BENCH_REMOTE_BUILD:-$remote_dir/build}"
remote_build_linux="${WSCPP_BENCH_REMOTE_BUILD_LINUX:-$remote_dir/build-linux}"
pid_file="/tmp/wscpp-bench-server.pid"
server_log="/tmp/wscpp-bench-server.log"
results_tsv="${WSCPP_BENCH_RESULTS:-/tmp/wscpp-net-bench.tsv}"

echo "=== wscpp LAN comparative benchmarks ==="
echo "remote: $remote_user_host"
echo "client -> $remote_host:$port"
echo "samples: $samples"

install_remote_deps() {
    ssh "$remote_user_host" 'sudo apt-get update -qq && sudo DEBIAN_FRONTEND=noninteractive apt-get install -y -qq \
        build-essential cmake g++ libssl-dev zlib1g-dev libwebsockets-dev libboost-system-dev rsync pkg-config'
}

sync_sources() {
    rsync -az --delete \
        --exclude '.git' \
        --exclude 'build' \
        --exclude 'build-*' \
        --exclude 'dist' \
        "$root/" "$remote_user_host:$remote_dir/"
}

configure_remote() {
    local build_dir="$1"
    local use_asio="$2"
    ssh "$remote_user_host" "cmake -S '$remote_dir' -B '$build_dir' -DCMAKE_BUILD_TYPE=Release \
        -DWSCPP_USE_ASIO=$use_asio \
        -DWSCPP_BUILD_TESTS=OFF \
        -DWSCPP_BUILD_EXAMPLES=OFF \
        -DWSCPP_BUILD_BENCHMARKS=ON \
        -DWSCPP_BUILD_DOCS=OFF"
}

configure_local() {
    local build_dir="$1"
    local use_asio="$2"
    cmake -S "$root" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release \
        -DWSCPP_USE_ASIO="$use_asio" \
        -DWSCPP_BUILD_TESTS=OFF \
        -DWSCPP_BUILD_EXAMPLES=OFF \
        -DWSCPP_BUILD_BENCHMARKS=ON \
        -DWSCPP_BUILD_DOCS=OFF
}

build_remote_servers() {
    configure_remote "$remote_build_asio" ON
    ssh "$remote_user_host" "cmake --build '$remote_build_asio' --target compare_net_servers -j\$(nproc)"
    configure_remote "$remote_build_linux" OFF
    ssh "$remote_user_host" "cmake --build '$remote_build_linux' --target bench_echo_server -j\$(nproc)"
}

build_local_clients() {
    configure_local "$local_build_asio" ON
    cmake --build "$local_build_asio" --target compare_net_clients -j"$(nproc)"
    configure_local "$local_build_linux" OFF
    cmake --build "$local_build_linux" --target bench_roundtrip_net -j"$(nproc)"
}

stop_remote_server() {
    ssh "$remote_user_host" "if [ -f '$pid_file' ]; then
        kill \"\$(cat '$pid_file')\" 2>/dev/null || true
        rm -f '$pid_file'
    fi"
}

start_remote_server() {
    local remote_build="$1"
    local server_bin="$2"
    shift 2
    stop_remote_server
    ssh "$remote_user_host" "nohup '$remote_build/bin/$server_bin' $* \
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
    local remote_build="$2"
    local server_bin="$3"
    local local_build="$4"
    local client_bin="$5"
    local server_args="$6"

    if [[ ! -x "$local_build/bin/$client_bin" ]]; then
        echo "skip $label (missing $client_bin)"
        return 0
    fi
    if ! ssh "$remote_user_host" "test -x '$remote_build/bin/$server_bin'"; then
        echo "skip $label (missing remote $server_bin)"
        return 0
    fi

    echo
    echo "---------- $label ----------"
    start_remote_server "$remote_build" "$server_bin" $server_args

    local out
    if ! out="$("$local_build/bin/$client_bin" "$remote_host" "$port" "$samples" 2>&1)"; then
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

if [[ "$skip_build" != "1" ]]; then
    install_remote_deps
    sync_sources
    build_remote_servers
    build_local_clients
else
    echo "skip install/sync/build (WSCPP_BENCH_SKIP_BUILD=1)"
fi

: >"$results_tsv"
trap stop_remote_server EXIT

bench_failed=0
run_case "wscpp (ASIO)" "$remote_build_asio" bench_echo_server "$local_build_asio" \
    bench_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "wscpp (linux)" "$remote_build_linux" bench_echo_server "$local_build_linux" \
    bench_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "websocketpp" "$remote_build_asio" bench_websocketpp_echo_server "$local_build_asio" \
    bench_websocketpp_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "IXWebSocket" "$remote_build_asio" bench_ixwebsocket_echo_server "$local_build_asio" \
    bench_ixwebsocket_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "libwebsockets" "$remote_build_asio" bench_libwebsockets_echo_server "$local_build_asio" \
    bench_libwebsockets_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "Boost.Beast" "$remote_build_asio" bench_beast_echo_server "$local_build_asio" \
    bench_beast_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "Simple-WebSocket-Server" "$remote_build_asio" bench_sws_echo_server "$local_build_asio" \
    bench_sws_roundtrip_net "$port 0.0.0.0" || bench_failed=1
run_case "easywsclient (connect)" "$remote_build_asio" bench_wscpp_connect_server "$local_build_asio" \
    bench_easywsclient_connect_net "$port 0.0.0.0" || bench_failed=1

print_summary
exit "$bench_failed"
