#!/usr/bin/env bash
# Standalone strict build of the U6F +01B8 whole-function model and its test
# (no CMake edit; registration is proposed in docs/re/U6F_01B8_WRITE_LIST.md).
#   usage: tests/recovered/build_u6f_01b8.sh [build-dir] [extra-cc-flags...]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/poolrad-u6f-01b8-build}; shift || true
CC=${CC:-cc}
mkdir -p "$OUT"
"$CC" -std=c11 -Wall -Wextra -Wpedantic -Werror "$@" -I"$ROOT/include" \
  "$ROOT/src/recovered/overlay_u6f_01b8_write_list.c" \
  "$ROOT/tests/recovered/test_overlay_u6f_01b8_write_list.c" \
  -o "$OUT/poolrad_u6f_01b8_tests"
"$OUT/poolrad_u6f_01b8_tests"
