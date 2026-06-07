#!/usr/bin/env bash
# Stage `cmake --install` output and create wscpp-<version>-<os>.tar.gz in dist/.
set -euo pipefail

build_dir="${1:-build-release}"
root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root"

version="$(tr -d '[:space:]' < VERSION)"
staging="$root/dist/staging"
archive_dir="$root/dist"
mkdir -p "$archive_dir"
rm -rf "$staging"
mkdir -p "$staging"

cmake --install "$build_dir" --prefix "$staging"

name="wscpp-${version}-linux-x86_64"
tarball="$archive_dir/${name}.tar.gz"
tar -C "$(dirname "$staging")" -czf "$tarball" "$(basename "$staging")"

echo "$tarball"
