#!/usr/bin/env bash
set -eu
# Use ci3 script base.
source $(git rev-parse --show-toplevel)/ci3/base/source

echo -e "\033[1mRetrieving avm-transpiler from remote cache...\033[0m"
export AZTEC_CACHE_REBUILD_PATTERNS="../noir/.rebuild_patterns_native .rebuild_patterns"
$ci3/cache/download avm-transpiler-$($ci3/cache/content_hash).tar.gz