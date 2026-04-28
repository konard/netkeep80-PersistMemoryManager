#!/usr/bin/env bash
# Local demo build entry point. Limits parallelism to 3 jobs by default
# (issue #371). Override via the PMM_JOBS env var, e.g. `PMM_JOBS=8 ./scripts/demo.sh`.
set -euo pipefail

: "${PMM_JOBS:=3}"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$repo_root/build"

mkdir -p "$build_dir"
cmake -S "$repo_root" -B "$build_dir" -DPMM_BUILD_DEMO=ON
cmake --build "$build_dir" --target pmm_demo -j "$PMM_JOBS"
"$build_dir/demo/pmm_demo"
