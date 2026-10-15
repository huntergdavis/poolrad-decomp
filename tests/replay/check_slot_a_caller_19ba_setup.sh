#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-caller19ba-setup"
mkdir -p "$build_dir"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Werror -I"$repo_dir/include" \
    "$repo_dir/src/replay/slot_a_caller_19ba_setup.c" \
    "$repo_dir/tests/replay/test_slot_a_caller_19ba_setup.c" -o "$build_dir/test_setup"
"$build_dir/test_setup"
if [[ $# -eq 1 ]]; then
    private_path=$(realpath -e -- "$1")
    case "$private_path" in "$repo_dir"/resources/*) ;; *) echo "private inputs must stay in this clone resources/" >&2; exit 2 ;; esac
    cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Werror -fPIC -shared -I"$repo_dir/include" \
        "$repo_dir/src/replay/slot_a_caller_19ba_setup.c" -o "$build_dir/libsetup.so"
    timeout --foreground --kill-after=5s 60s "${POOLRAD_UNICORN_PYTHON:-python3}" -u \
        "$repo_dir/tests/replay/audit_slot_a_caller_19ba_setup.py" "$build_dir/libsetup.so" "$private_path"
elif [[ $# -ne 0 ]]; then echo "usage: $0 [NATIVE_CALLER_ROOT]" >&2; exit 2
fi
