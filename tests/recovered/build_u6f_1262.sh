#!/usr/bin/env bash
# Standalone strict build of the U6F +1262 whole-function model and its test
# (registration in a separate commit of the same branch).
#   usage: tests/recovered/build_u6f_1262.sh [build-dir] [extra-cc-flags...]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/poolrad-u6f-1262-build}; shift || true
CC=${CC:-cc}
mkdir -p "$OUT"
"$CC" -std=c11 -Wall -Wextra -Wpedantic -Werror "$@" -I"$ROOT/include" \
  "$ROOT/src/recovered/overlay_u6f_1262_add_npc.c" \
  "$ROOT/tests/recovered/test_overlay_u6f_1262_add_npc.c" \
  -o "$OUT/poolrad_u6f_1262_tests"
"$OUT/poolrad_u6f_1262_tests"
