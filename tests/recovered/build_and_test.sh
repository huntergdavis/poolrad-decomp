#!/usr/bin/env bash
# Build and run the recovered-function tests without CMake (CMakeLists.txt is
# owned by lane B; the equivalent targets are proposed in docs/re/FUNCTION_LEDGER.md).
#   usage: tests/recovered/build_and_test.sh [build-dir]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/poolrad-recovered-build}
CC=${CC:-cc}
mkdir -p "$OUT"
"$CC" -std=c11 -Wall -Wextra -Wpedantic -Werror -I"$ROOT/include" \
  "$ROOT/src/recovered/ecl_helpers.c" "$ROOT/tests/recovered/test_ecl_helpers.c" \
  -o "$OUT/poolrad_recovered_tests"
"$OUT/poolrad_recovered_tests"
