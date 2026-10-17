#!/usr/bin/env bash
# Standalone strict build of the U3B9 +0708 (entry 7) whole-function model and
# its test (no CMake edit; registration is proposed in
# docs/re/U3B9_ENTRY7_0708_SPRITE.md).
#   usage: tests/recovered/build_u3b9_0708.sh [build-dir] [extra-cc-flags...]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/poolrad-u3b9-0708-build}; shift || true
CC=${CC:-cc}
mkdir -p "$OUT"
"$CC" -std=c11 -Wall -Wextra -Wpedantic -Werror "$@" -I"$ROOT/include" \
  "$ROOT/src/recovered/overlay_u3b9_entry7_0708.c" \
  "$ROOT/tests/recovered/test_overlay_u3b9_entry7_0708.c" \
  -o "$OUT/poolrad_u3b9_0708_tests"
"$OUT/poolrad_u3b9_0708_tests"
