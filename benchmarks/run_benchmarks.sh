#!/usr/bin/env bash
# Run all benchmark binaries in the current directory (typically build/bin).
set -euo pipefail

bin_dir="${1:-.}"
cd "$bin_dir"

run_one() {
    local name="$1"
    if [[ -x "./${name}" ]]; then
        echo
        "./${name}"
    else
        echo "skip ${name} (not built)"
    fi
}

micro=(
    bench_frame_parse
    bench_masking
    bench_roundtrip
)

compare=(
    bench_websocketpp_roundtrip
    bench_ixwebsocket_roundtrip
    bench_libwebsockets_roundtrip
    bench_easywsclient_connect
    bench_beast_roundtrip
    bench_sws_roundtrip
)

echo "wscpp benchmarks — $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "cwd: $(pwd)"

for t in "${micro[@]}"; do
    run_one "$t"
done

for t in "${compare[@]}"; do
    run_one "$t"
done

echo
echo "Binary sizes (bytes):"
for t in "${micro[@]}" "${compare[@]}"; do
    if [[ -f "./${t}" ]]; then
        printf "  %-32s %s\n" "$t" "$(stat -c%s "./${t}" 2>/dev/null || stat -f%z "./${t}")"
    fi
done
