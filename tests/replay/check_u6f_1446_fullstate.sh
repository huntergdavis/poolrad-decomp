#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
flags=(-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror)
mode=strict
native_run=
if [[ $# -eq 1 && $1 == --sanitize ]]; then
    flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
    mode=sanitize
elif [[ $# -eq 2 && $1 == --native-prefix ]]; then
    native_run=$(realpath -e -- "$2")
elif [[ $# -ne 0 ]]; then
    echo "usage: $0 [--sanitize | --native-prefix IGNORED_RUN]" >&2
    exit 2
fi
build_dir="$repo_dir/resources/build-u6f-1446-fullstate-$mode"
mkdir -p "$build_dir"
cc "${flags[@]}" -I"$repo_dir/include" \
    "$repo_dir/src/replay/u6f_1446_fullstate.c" \
    "$repo_dir/src/recovered/overlay_u6f_party_insert_1446_ss.c" \
    "$repo_dir/src/recovered/overlay_u9b_entry_zero_0000.c" \
    "$repo_dir/src/recovered/overlay_u9b_record_table_0255.c" \
    "$repo_dir/src/recovered/overlay_u9b_record_fields_0329.c" \
    "$repo_dir/tests/replay/test_u6f_1446_fullstate.c" \
    -o "$build_dir/test_u6f_1446_fullstate"
"$build_dir/test_u6f_1446_fullstate"
if [[ -n $native_run ]]; then
    case "$native_run" in
        */resources/*) ;;
        *) echo "preserved captures must stay under resources/" >&2; exit 2 ;;
    esac
    cc "${flags[@]}" -fPIC -shared -I"$repo_dir/include" \
        "$repo_dir/src/replay/u6f_1446_fullstate.c" \
        "$repo_dir/src/recovered/overlay_u9b_entry_zero_0000.c" \
        "$repo_dir/src/recovered/overlay_u9b_record_table_0255.c" \
        "$repo_dir/src/recovered/overlay_u9b_record_fields_0329.c" \
        -o "$build_dir/libu6f_1446.so"
    PYTHONDONTWRITEBYTECODE=1 python3 \
        "$repo_dir/tests/replay/check_u6f_1446_native_prefix.py" \
        "$build_dir/libu6f_1446.so" "$native_run"
fi
