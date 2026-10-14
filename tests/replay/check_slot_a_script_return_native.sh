#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 9 ]]; then
    echo "usage: $0 IGNORED_START.EXE GAME.OVR SAVGAMJ.DAT RAM_34E7 RAM_3171 RAM_026C RAM_34EA RAM_34F5 RAM_3395" >&2
    exit 2
fi
for input in "$@"; do
    private_path=$(realpath -e -- "$input")
    case "$private_path" in
        "$repo_dir"/resources/*) ;;
        *) echo "private inputs must stay in this clone's ignored resources/" >&2
           exit 2 ;;
    esac
done

python3 "$repo_dir/tests/replay/audit_slot_a_script_return_native.py" "$@"
