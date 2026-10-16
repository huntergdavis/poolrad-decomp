#!/usr/bin/env bash
# Standalone strict build of the U6F +0482 whole-function model and its test
# (registration proposed in docs/re/U6F_0482_PICTURE_BUILD.md and applied in a
# separate commit of the same branch).
#   usage: tests/recovered/build_u6f_0482.sh [build-dir] [extra-cc-flags...]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/poolrad-u6f-0482-build}; shift || true
CC=${CC:-cc}
mkdir -p "$OUT"
"$CC" -std=c11 -Wall -Wextra -Wpedantic -Werror "$@" -I"$ROOT/include" \
  "$ROOT/src/recovered/overlay_u6f_0482_picture_build.c" \
  "$ROOT/tests/recovered/test_overlay_u6f_0482_picture_build.c" \
  -o "$OUT/poolrad_u6f_0482_tests"
"$OUT/poolrad_u6f_0482_tests"
