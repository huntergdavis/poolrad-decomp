#!/usr/bin/env bash
# Run the --native (synthetic DOSBox-X) corroboration of the four 08AE C models, verify its reports, then run every
# mutant: seven of the verifier's branches and four mutations of the C model sources that the native runs must catch.
# Everything stays under this tree's ignored resources/; only counts are printed.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 0 ]]; then
    echo "usage: $0" >&2
    exit 2
fi
resources=$(realpath -e -- "$repo_dir/resources")
work="$resources/build-native-models-08ae"
audit="$repo_dir/tests/replay/audit_slot_a_native_models_08ae.py"

echo "== collect (native on the system and the pinned emulator; unicorn on the system pass)"
nice -n 15 python3 "$audit" collect "$work"
echo "== verify (baseline)"
nice -n 15 python3 "$audit" verify "$work"

# Mutants of the verifier's branches: each must fail.  mask-narrow depends on at least one PF difference between the
# two emulator builds in the collected run (IN 61h reads live hardware; 9 and 16 of 32 records were seen).
killed=0
survived=()
for m in expect-cases expect-body expect-owner expect-masked emu-sha unicorn-count mask-narrow; do
    if POOLRAD_NATIVE_MUTANT=$m nice -n 15 python3 "$audit" verify "$work" >/dev/null 2>&1; then
        survived+=("$m")
        echo "START verifier mutant $m -> survived"
    else
        killed=$((killed + 1))
        echo "START verifier mutant $m -> killed"
    fi
done

# Mutations of the C model sources, applied in a scratch copy of the tree under the work dir (no source change in this
# tree): the model's own --native audit must fail on each.
mutroot="$work/mutroot"
rm -rf "$mutroot"
mkdir -p "$mutroot/src"
cp -r "$repo_dir/tools" "$mutroot/tools"
cp -r "$repo_dir/include" "$mutroot/include"
cp -r "$repo_dir/src/recovered" "$mutroot/src/recovered"
ln -s "$resources" "$mutroot/resources"
model_killed=0
model_survived=()
mutate() {   # name audit source sed-expression
    local name=$1 audit_name=$2 source=$3 expr=$4 file="$mutroot/src/recovered/$3.c" backup
    backup=$(mktemp -p "$work")
    cp "$file" "$backup"
    sed -i "$expr" "$file"
    if cmp -s "$file" "$backup"; then
        echo "mutation $name did not change the source" >&2
        exit 1
    fi
    if (cd "$mutroot" && TMPDIR="$work/tmp" nice -n 15 python3 "tools/overlay/$audit_name.py" --native >/dev/null 2>&1); then
        model_survived+=("$name")
        echo "START model mutant $name -> survived"
    else
        model_killed=$((model_killed + 1))
        echo "START model mutant $name -> killed"
    fi
    cp "$backup" "$file"
    rm -f "$backup"
}
mutate 1742-di-stride res08ae_table_1742_cpu_audit resident_08ae_table_1742_cpu 's/c->di = arith(c, c->di, 0x30u, 16, 0);/c->di = arith(c, c->di, 0x31u, 16, 0);/'
mutate 17ae-di-stride res08ae_table_17ae_cpu_audit resident_08ae_table_17ae_cpu 's/c->di = arith(c, c->di, 0x30u, 16, 0);/c->di = arith(c, c->di, 0x31u, 16, 0);/'
mutate 13b1-ax-high res08ae_vector_13b1_cpu_audit resident_08ae_vector_13b1_cpu 's/c->ax = 0xFFFFu; c->ip = 0x13CCu;/c->ax = 0xFEFFu; c->ip = 0x13CCu;/'
mutate 00dc-al-read res08ae_dispatch_00dc_cpu_audit resident_08ae_dispatch_00dc_cpu 's/(c->ax \& 0xFF00u) | ram\[physical(c->ds, 0x5372u)\]/(c->ax \& 0xFF00u) | ram[physical(c->ds, 0x5373u)]/'
rm -rf "$mutroot"
echo "== verifier mutants: killed $killed of 7, survived: ${survived[*]:-none}; model mutants: killed $model_killed of 4, survived: ${model_survived[*]:-none}"
if [[ ${#survived[@]} -ne 0 || ${#model_survived[@]} -ne 0 ]]; then
    echo "unexpected mutant survival" >&2
    exit 1
fi
echo "native model corroboration and mutants verified"
