#!/usr/bin/env bash
# Verify C/C++ sources match .clang-format (repository root).
# Only first-party paths are checked; third-party trees (FetchContent, rust/, build/) are excluded.
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format not found" >&2
    exit 1
fi

mapfile -d '' files < <(
    find include src examples benchmarks tests \
        \( -path '*/rust/*' -o -path '*/build/*' -o -path '*/_deps/*' -o -path '*/third_party/*' \) -prune \
        -o -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0 2>/dev/null | sort -z
)

if [ "${#files[@]}" -eq 0 ]; then
    echo "No source files found for format check" >&2
    exit 1
fi

failed=0
for f in "${files[@]}"; do
    if ! clang-format --dry-run --Werror "$f" >/dev/null 2>&1; then
        echo "format check failed: $f" >&2
        clang-format --dry-run --Werror "$f" 2>&1 | head -20 >&2 || true
        failed=1
    fi
done

if [ "$failed" -ne 0 ]; then
    echo "Run: find include src examples benchmarks tests -type f \\( -name '*.cpp' -o -name '*.hpp' \\) -exec clang-format -i {} +" >&2
    exit 1
fi

echo "clang-format: OK (${#files[@]} files)"
