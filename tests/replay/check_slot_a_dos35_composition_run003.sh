#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 1 ]; then echo "usage: $0 PRIVATE_RUN003_DIR" >&2; exit 2; fi
run=$1
root=$(cd "$(dirname "$0")/../.." && pwd)
printf '%s  %s\n' \
 9de586bd5baa6b66dc226178c4ebe5a69b91eb45467f9be7e9ef96fb3e634a88 "$run/RAM-1386.BIN" \
 917b3ad32ce67c6601db787d542d692d96152e51560e47a907db059adab2bf69 "$run/RAM-1388.BIN" \
 4f34ecf7352f2dc2a731e9b4e4df5831e4223b9aabec5341ba4aad4a8ee8e773 "$run/CPU-1386.TXT" \
 0bb1d1b9f7600b2685ae390424995560775037153421adbd1207a537858ff437 "$run/CPU-1388.TXT" | sha256sum -c -
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I"$root/include" "$root/src/recovered/resident_08ae_vector_137c_cpu.c" "$root/tests/replay/test_slot_a_dos35_composition_run003.c" -o "$bin"
"$bin" "$run/RAM-1386.BIN" "$run/RAM-1388.BIN"
