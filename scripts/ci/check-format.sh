#!/usr/bin/env bash
# Verify C/C++ sources match .clang-format (repository root).
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format not found" >&2
    exit 1
fi

mapfile -d '' files < <(
    find include src examples benchmarks tests \
        -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) \
        -print0 2>/dev/null | sort -z
)

if [ "${#files[@]}" -eq 0 ]; then
    echo "No source files found for format check" >&2
    exit 1
fi

failed=0
for f in "${files[@]}"; do
    if ! diff -u "$f" <(clang-format "$f") >/dev/null; then
        echo "format check failed: $f" >&2
        failed=1
    fi
done

if [ "$failed" -ne 0 ]; then
    echo "Run: find include src examples benchmarks tests -type f \\( -name '*.cpp' -o -name '*.hpp' \\) -exec clang-format -i {} +" >&2
    exit 1
fi

echo "clang-format: OK (${#files[@]} files)"
