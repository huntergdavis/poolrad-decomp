#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 1 ]; then echo "usage: $0 PRIVATE_RUN003_DIR" >&2; exit 2; fi
run=$1
root=$(cd "$(dirname "$0")/../.." && pwd)
printf '%s  %s\n' \
 9de586bd5baa6b66dc226178c4ebe5a69b91eb45467f9be7e9ef96fb3e634a88 "$run/RAM-1386.BIN" \
 917b3ad32ce67c6601db787d542d692d96152e51560e47a907db059adab2bf69 "$run/RAM-1388.BIN" \
 4f34ecf7352f2dc2a731e9b4e4df5831e4223b9aabec5341ba4aad4a8ee8e773 "$run/CPU-1386.TXT" \
 0bb1d1b9f7600b2685ae390424995560775037153421adbd1207a537858ff437 "$run/CPU-1388.TXT" \
 31cd7bfd60bc222f1530fed17ceca134f3f5645a6ed9ac84783d276b98ce3525 "$run/RAM-139E.BIN" \
 4315d431b602ddab764ce15dc4aac13a25419a53139a14289c9e44142bed1bf0 "$run/CPU-139E.TXT" \
 606e2012a24646fa66700f6cf89e0d4505f7c2963cdbde67ec15cc7a9212aa40 "$run/RAM-13A0.BIN" \
 418c82068ac7f3f5243272d895926ad2e408cbb7bea2696f7ceb3396bde981b3 "$run/CPU-13A0.TXT" \
 606e2012a24646fa66700f6cf89e0d4505f7c2963cdbde67ec15cc7a9212aa40 "$run/RAM-13A3.BIN" \
 f01232f48666d06d1edafb82b46e2cf0b05d1a67f871f0e81391f9e296470fb7 "$run/CPU-13A3.TXT" | sha256sum -c -
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I"$root/include" "$root/src/recovered/resident_08ae_vector_137c_cpu.c" "$root/src/recovered/resident_08ae_dispatch_00dc_cpu.c" "$root/src/recovered/resident_08ae_table_1742_cpu.c" "$root/tests/replay/test_slot_a_dos35_composition_run003.c" -o "$bin"
"$bin" "$run/RAM-1386.BIN" "$run/RAM-1388.BIN" "$run/RAM-139E.BIN" "$run/RAM-13A0.BIN" "$run/RAM-13A3.BIN"
