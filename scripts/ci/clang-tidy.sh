#!/usr/bin/env bash
# Run clang-tidy on wscpp library sources using compile_commands.json from $1 (build dir).
# Third-party headers (FetchContent deps, bundled fmt, OpenSSL, etc.) are not linted.
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

readonly -a system_header_prefixes=(
    spdlog/
    fmt/
    asio/
    openssl/
)

mapfile -t files < <(
    find src -type f -name '*.cpp' | sort
)

if [ "${#files[@]}" -eq 0 ]; then
    echo "No library .cpp files found for clang-tidy" >&2
    exit 1
fi

supports_system_header_prefix=0
if clang-tidy --help 2>&1 | grep -q 'system-header-prefix'; then
    supports_system_header_prefix=1
fi

failed=0
for f in "${files[@]}"; do
    tidy_args=(-p "$build_dir" -warnings-as-errors='*' --quiet)
    if [ "$supports_system_header_prefix" -eq 1 ]; then
        for prefix in "${system_header_prefixes[@]}"; do
            tidy_args+=(--system-header-prefix="$prefix")
        done
    fi
    if ! clang-tidy "${tidy_args[@]}" "$f" 2>&1; then
        echo "clang-tidy failed: $f" >&2
        failed=1
    fi
done

if [ "$failed" -ne 0 ]; then
    exit 1
fi

echo "clang-tidy: OK (${#files[@]} translation units)"
