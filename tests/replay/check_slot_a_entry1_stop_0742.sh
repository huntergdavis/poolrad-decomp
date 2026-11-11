#!/usr/bin/env bash
# Compare only modeled fields at a refused call, not full CPU/RAM/device state.
set -euo pipefail
if [ "$#" -ne 3 ]; then
    echo "usage: $0 RECOVERED_LIBRARY ENTRY_0D6D_RAM STOP_0742_RAM" >&2
    exit 2
fi
lib=$1 entry=$2 stop=$3
root=$(cd "$(dirname "$0")/../.." && pwd)
printf '%s  %s\n' ded9ac1d1db89fead04ed13a6515be9152662478e10d2edc721e6bb8cc5a868e "$entry" 2ef55fd5fd893f3e5397130a35bfc77777c4e126eda939bebda5da3cace7de4e "$stop" | sha256sum -c -
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I"$root/include" "$root/tests/replay/test_slot_a_entry1_stop_0742.c" "$lib" -o "$bin"
"$bin" "$entry" "$stop"
