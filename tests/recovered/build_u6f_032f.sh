#!/usr/bin/env bash
# Standalone strict build of the U6F +032F leaf translation and its test
# (registration in a separate commit of the same branch).
#   usage: tests/recovered/build_u6f_032f.sh [build-dir] [extra-cc-flags...]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/poolrad-u6f-032f-build}; shift || true
CC=${CC:-cc}
mkdir -p "$OUT"
"$CC" -std=c11 -Wall -Wextra -Wpedantic -Werror "$@" -I"$ROOT/include" \
  "$ROOT/src/recovered/overlay_u6f_032f_picture_merge.c" \
  "$ROOT/tests/recovered/test_overlay_u6f_032f_picture_merge.c" \
  -o "$OUT/poolrad_u6f_032f_tests"
"$OUT/poolrad_u6f_032f_tests"
