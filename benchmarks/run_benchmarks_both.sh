#!/usr/bin/env bash
# Run wscpp bench_roundtrip for both transport builds and compare suite from one tree.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_asio="${WSCPP_BUILD_ASIO:-${root}/build-bench-asio}"
build_linux="${WSCPP_BUILD_LINUX:-${root}/build-bench-linux}"

run_transport() {
    local label="$1"
    local dir="$2"
    local flag="$3"
    echo
    echo "========== wscpp transport: ${label} (${flag}) =========="
    if [[ ! -x "${dir}/bin/bench_roundtrip" ]]; then
        echo "skip (build ${dir} first)"
        return 0
    fi
    "${dir}/bin/bench_roundtrip"
    stat -c "bench_roundtrip size: %s bytes" "${dir}/bin/bench_roundtrip" 2>/dev/null \
        || stat -f "bench_roundtrip size: %z bytes" "${dir}/bin/bench_roundtrip"
}

for dir in "$build_asio" "$build_linux"; do
    if [[ ! -d "$dir" ]]; then
        echo "Configure ${dir}..."
        flag=ON
        [[ "$dir" == *linux* ]] && flag=OFF
        cmake -B "$dir" -DWSCPP_BUILD_BENCHMARKS=ON -DWSCPP_USE_ASIO="$flag" -DCMAKE_BUILD_TYPE=Release
        cmake --build "$dir" --target bench_roundtrip -j"$(nproc)"
    fi
done

run_transport "ASIO" "$build_asio" "WSCPP_USE_ASIO=ON"
run_transport "linux POSIX" "$build_linux" "WSCPP_USE_ASIO=OFF"

if [[ -x "${build_asio}/bin/bench_websocketpp_roundtrip" ]]; then
    echo
    echo "========== third-party compare (ASIO build tree) =========="
    bash "${root}/benchmarks/run_benchmarks.sh" "${build_asio}/bin"
fi
