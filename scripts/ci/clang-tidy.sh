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
    find src include/wscpp -type f \( -name '*.cpp' -o -name '*.hpp' \) | sort
)

failed=0
for f in "${files[@]}"; do
    # Headers without a translation unit are analyzed via their includer when listed in compile_commands.
    if [[ "$f" == *.hpp && "$f" != */detail/* ]]; then
        continue
    fi
    if [[ "$f" == *.hpp ]]; then
        continue
    fi
    if ! clang-tidy -p "$build_dir" "$f" --quiet; then
        echo "clang-tidy failed: $f" >&2
        failed=1
    fi
done

if [ "$failed" -ne 0 ]; then
    exit 1
fi

echo "clang-tidy: OK (${#files[@]} files scanned)"
