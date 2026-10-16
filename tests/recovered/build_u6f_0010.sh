#!/usr/bin/env bash
# Standalone strict build of the U6F +0010 whole-function model and its test
# (no CMake edit; registration is proposed in docs/re/U6F_0010_READ_LIST.md).
#   usage: tests/recovered/build_u6f_0010.sh [build-dir] [extra-cc-flags...]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/poolrad-u6f-0010-build}; shift || true
CC=${CC:-cc}
mkdir -p "$OUT"
"$CC" -std=c11 -Wall -Wextra -Wpedantic -Werror "$@" -I"$ROOT/include" \
  "$ROOT/src/recovered/overlay_u6f_0010_read_list.c" \
  "$ROOT/tests/recovered/test_overlay_u6f_0010_read_list.c" \
  -o "$OUT/poolrad_u6f_0010_tests"
"$OUT/poolrad_u6f_0010_tests"
