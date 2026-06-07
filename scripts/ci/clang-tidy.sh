#!/usr/bin/env bash
# Run clang-tidy on wscpp library sources using compile_commands.json from $1 (build dir).
set -euo pipefail

build_dir="${1:-build-ci}"
root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root"

if [ ! -f "$build_dir/compile_commands.json" ]; then
    echo "Missing $build_dir/compile_commands.json — configure with CMAKE_EXPORT_COMPILE_COMMANDS=ON" >&2
    exit 1
fi

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "clang-tidy not found" >&2
    exit 1
fi

mapfile -t files < <(
    find src -type f -name '*.cpp' | sort
)

if [ "${#files[@]}" -eq 0 ]; then
    echo "No library .cpp files found for clang-tidy" >&2
    exit 1
fi

failed=0
for f in "${files[@]}"; do
    if ! clang-tidy -p "$build_dir" "$f" \
        -warnings-as-errors='*' \
        --quiet 2>&1; then
        echo "clang-tidy failed: $f" >&2
        failed=1
    fi
done

if [ "$failed" -ne 0 ]; then
    exit 1
fi

echo "clang-tidy: OK (${#files[@]} translation units)"
