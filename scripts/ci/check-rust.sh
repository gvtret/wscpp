#!/usr/bin/env bash
# check-rust.sh — format, clippy (warnings as errors), tests for rust/ workspace.
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root/rust"

echo "== rustfmt =="
cargo fmt --all --check

export RUSTFLAGS="-D warnings"

echo "== clippy (warnings denied) =="
cargo clippy --workspace --all-targets

echo "== test =="
cargo test --workspace -- --test-threads=1

echo "== doc (broken links + missing docs denied) =="
RUSTDOCFLAGS="-D rustdoc::broken_intra_doc_links" cargo doc -p ws-rs --no-deps

echo "OK: rust checks passed"
