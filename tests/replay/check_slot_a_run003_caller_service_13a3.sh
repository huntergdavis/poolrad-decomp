#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 2 ]; then echo "usage: $0 RECOVERED_LIBRARY PRIVATE_RUN003_DIR" >&2; exit 2; fi
lib=$1 run=$2
root=$(cd "$(dirname "$0")/../.." && pwd)
printf '%s  %s\n' \
 d392df4b1192c6c74488607646ac4f300a9e301910aae8139268ebb88ae2a44f "$run/RAM-348D-SEGMENTED.BIN" \
 eacba5ce0db51b360c0ba676dd876e507f31a7733a4ed45bdb404e6f99dd3d8b "$run/CPU-348D.TXT" \
 606e2012a24646fa66700f6cf89e0d4505f7c2963cdbde67ec15cc7a9212aa40 "$run/RAM-13A3.BIN" \
 f01232f48666d06d1edafb82b46e2cf0b05d1a67f871f0e81391f9e296470fb7 "$run/CPU-13A3.TXT" | sha256sum -c -
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I"$root/include" "$root/tests/replay/test_slot_a_run003_caller_service_13a3.c" "$root/src/recovered/resident_08ae_vector_137c_cpu.c" "$lib" -o "$bin"
"$bin" "$run/RAM-348D-SEGMENTED.BIN" "$run/RAM-13A3.BIN"
