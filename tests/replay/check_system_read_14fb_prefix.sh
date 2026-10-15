#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-system-read14fb"
mkdir -p "$build_dir"
flags=(-std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Werror -I"$repo_dir/include")
cc "${flags[@]}" "$repo_dir/src/replay/system_read_14fb_prefix.c" "$repo_dir/tests/replay/test_system_read_14fb_prefix.c" -o "$build_dir/test_abi"
"$build_dir/test_abi"
if [[ $# -eq 1 ]];then
 private_path=$(realpath -e -- "$1")
 case "$private_path" in "$repo_dir"/resources/*) ;; *) echo 'inputs must stay under resources/' >&2;exit 2;;esac
 cc "${flags[@]}" -fPIC -shared "$repo_dir/src/replay/system_read_14fb_prefix.c" -o "$build_dir/libabi.so"
 timeout --foreground --kill-after=5s 120s "${POOLRAD_UNICORN_PYTHON:-python3}" -u "$repo_dir/tests/replay/audit_system_read_14fb_prefix.py" "$build_dir/libabi.so" "$private_path"
elif [[ $# -ne 0 ]];then echo 'usage: runner [NATIVE_READ_PREFIX_ROOT]' >&2;exit 2;fi
