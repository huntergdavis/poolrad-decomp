#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [ "$#" -gt 1 ]; then
    echo "usage: $0 [IGNORED_SCRIPT_ENTRY_RAM]" >&2
    exit 2
fi
bash "$repo_dir/tests/replay/check_slot_a_first_exit.sh"
if [ "$#" -eq 1 ]; then
    printf '%s  %s\n' f71571a2f22f6264dc39f52424a9d4d300a3429f2d1c7932cec8735a98ae0558 "$1" | sha256sum -c -
    "$repo_dir/resources/build-slot-a-first-exit/test_slot_a_first_exit" "$1"
fi
