#!/usr/bin/env bash
# Backward-compatible entry point — runs full LAN comparative suite.
exec "$(cd "$(dirname "$0")" && pwd)/run_remote_network_compare.sh" "$@"
