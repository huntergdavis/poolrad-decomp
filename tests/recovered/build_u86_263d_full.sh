#!/usr/bin/env bash
# Standalone strict build of the U86 +263d full-state model and its smoke
# test (registration in a separate commit of the same branch).
#   usage: tests/recovered/build_u86_263d_full.sh [build-dir] [extra-cc-flags...]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/poolrad-u86-263d-full-build}; shift || true
CC=${CC:-cc}
mkdir -p "$OUT"
"$CC" -std=c11 -Wall -Wextra -Wpedantic -Werror "$@" -I"$ROOT/include" \
  "$ROOT/src/recovered/overlay_u86_status_263d_full.c" \
  "$ROOT/tests/recovered/test_overlay_u86_status_263d_full.c" \
  -o "$OUT/poolrad_u86_263d_full_tests"
"$OUT/poolrad_u86_263d_full_tests"
