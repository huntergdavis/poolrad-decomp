#!/usr/bin/env bash
# Linux/POSIX guard mapping; the high uncaptured range is PROT_NONE.
set -euo pipefail
if [ "$#" -ne 2 ]; then echo "usage: $0 RECOVERED_LIBRARY PRIVATE_RUN002_DIR" >&2; exit 2; fi
lib=$1 run=$2
root=$(cd "$(dirname "$0")/../.." && pwd)
python3 "$root/tests/replay/check_slot_a_resident_prefix.py" "$run"
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I"$root/include" "$root/tests/replay/test_slot_a_c_resident_prefix.c" "$lib" -o "$bin"
"$bin" "$run/RAM-0742-SEGMENTED.BIN" "$run/RAM-1386-SEGMENTED.BIN"
