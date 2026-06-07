#!/usr/bin/env bash
# Run all ws-rs benchmark binaries (mirrors benchmarks/run_benchmarks.sh).
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

cargo build --release -p ws-rs-benches --bins

bin_dir="${root}/target/release"

run_one() {
    local name="$1"
    if [[ -x "${bin_dir}/${name}" ]]; then
        echo
        "${bin_dir}/${name}"
    else
        echo "skip ${name} (not built)"
    fi
}

micro=(
    bench_frame_parse
    bench_masking
    bench_roundtrip
    bench_blocking_roundtrip
)

compare=(
    bench_tokio_tungstenite_roundtrip
    bench_fastwebsockets_roundtrip
    bench_tokio_websockets_roundtrip
)

echo "ws-rs benchmarks — $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "cwd: ${bin_dir}"

for t in "${micro[@]}"; do
    run_one "$t"
done

for t in "${compare[@]}"; do
    run_one "$t"
done

echo
echo "Binary sizes (bytes):"
for t in "${micro[@]}" "${compare[@]}"; do
    if [[ -f "${bin_dir}/${t}" ]]; then
        printf "  %-32s %s\n" "$t" "$(stat -c%s "${bin_dir}/${t}" 2>/dev/null || stat -f%z "${bin_dir}/${t}")"
    fi
done
