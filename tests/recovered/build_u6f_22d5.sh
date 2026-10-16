#!/usr/bin/env bash
# Standalone strict build of the U6F +22D5 whole-function model and its test
# (no CMake edit; registration is proposed in docs/re/U6F_22D5_TRANSITION_PROMPT.md).
#   usage: tests/recovered/build_u6f_22d5.sh [build-dir] [extra-cc-flags...]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/poolrad-u6f-22d5-build}; shift || true
CC=${CC:-cc}
mkdir -p "$OUT"
"$CC" -std=c11 -Wall -Wextra -Wpedantic -Werror "$@" -I"$ROOT/include" \
  "$ROOT/src/recovered/overlay_u6f_22d5_transition_prompt.c" \
  "$ROOT/tests/recovered/test_overlay_u6f_22d5_transition_prompt.c" \
  -o "$OUT/poolrad_u6f_22d5_tests"
"$OUT/poolrad_u6f_22d5_tests"
