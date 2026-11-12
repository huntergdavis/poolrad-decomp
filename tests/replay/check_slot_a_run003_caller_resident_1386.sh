#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 2 ]; then echo "usage: $0 RECOVERED_LIBRARY PRIVATE_RUN003_DIR" >&2; exit 2; fi
lib=$1 run=$2
root=$(cd "$(dirname "$0")/../.." && pwd)
printf '%s  %s\n' \
 d392df4b1192c6c74488607646ac4f300a9e301910aae8139268ebb88ae2a44f "$run/RAM-348D-SEGMENTED.BIN" \
 eacba5ce0db51b360c0ba676dd876e507f31a7733a4ed45bdb404e6f99dd3d8b "$run/CPU-348D.TXT" \
 9de586bd5baa6b66dc226178c4ebe5a69b91eb45467f9be7e9ef96fb3e634a88 "$run/RAM-1386.BIN" \
 4f34ecf7352f2dc2a731e9b4e4df5831e4223b9aabec5341ba4aad4a8ee8e773 "$run/CPU-1386.TXT" | sha256sum -c -
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I"$root/include" "$root/tests/replay/test_slot_a_run003_caller_resident_1386.c" "$lib" -o "$bin"
"$bin" "$run/RAM-348D-SEGMENTED.BIN" "$run/RAM-1386.BIN"
