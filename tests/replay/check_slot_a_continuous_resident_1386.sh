#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 2 ]; then echo "usage: $0 RECOVERED_LIBRARY PRIVATE_RUN002_DIR" >&2; exit 2; fi
lib=$1 run=$2
root=$(cd "$(dirname "$0")/../.." && pwd)
python3 "$root/tests/replay/check_slot_a_resident_prefix.py" "$run"
printf '%s  %s\n' \
 ded9ac1d1db89fead04ed13a6515be9152662478e10d2edc721e6bb8cc5a868e "$run/RAM-0D6D-SEGMENTED.BIN" \
 54564c50156141b2e550a2d59ad6c34df44be42425cc1de68ee18d9178d25cd3 "$run/CPU-0D6D.TXT" | sha256sum -c -
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I"$root/include" "$root/tests/replay/test_slot_a_continuous_resident_1386.c" "$lib" -o "$bin"
"$bin" "$run/RAM-0D6D-SEGMENTED.BIN" "$run/RAM-0742-SEGMENTED.BIN" "$run/RAM-1386-SEGMENTED.BIN"
