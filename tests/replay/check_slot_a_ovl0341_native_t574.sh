#!/usr/bin/env bash
# Run the run-039 OVL_0025_0341 native audit, then every mutant of its branches.
# Private inputs stay under this tree's ignored resources/; only counts are printed.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 0 ]]; then
    echo "usage: $0" >&2
    exit 2
fi
resources=$(realpath -e -- "$repo_dir/resources")
run039=$(realpath -e -- "$resources/oracle-runs/t4492-slot10-ovl0341-039")
game=$(realpath -e -- "$resources/DOS/POOLRAD/GAME.OVR")
build="$resources/build-ovl0341-native-t574"
audit="$repo_dir/tests/replay/audit_slot_a_ovl0341_native_t574.py"

echo "== baseline"
nice -n 15 python3 "$audit" "$run039" "$game" "$build"

# Mutants of the audit's branches (replay-*, comp-*) and of the C model's source (m-*).  A mutant is KILLED only when the audit exits
# non-zero WITHOUT a Python traceback and, for a model mutant, after the build marker (a mutant that does not build is never a kill).
# The expected survivors are triaged in docs/re/SLOT_A_OVL0341_NATIVE_T574.md (two equivalent, three state-indistinguishable).
mutants=(regsave-order read-skip tick-skip kbd-skip no-hw-frame int-frame-skip psp-skip rep-short read-pos seek-origin
         comp-handle-5 comp-entry-ds comp-flags-cf comp-flags-if comp-host-narrow comp-out-drop
         m-fill-len m-557c m-skip-freemem m-add2 m-dec-once m-digit-ds m-retf-pop m-jb-invert)
expected="comp-flags-cf m-557c m-dec-once m-digit-ds no-hw-frame"
killed=0
survived=()
for m in "${mutants[@]}"; do
    out=$(mktemp -p "$build")
    rc=0
    POOLRAD_T574_MUTANT=$m nice -n 15 python3 "$audit" "$run039" "$game" "$build" >"$out" 2>&1 || rc=$?
    if grep -q Traceback "$out"; then
        echo "INVALID mutant $m: the audit crashed instead of failing a check" >&2
        tail -3 "$out" >&2
        rm -f "$out"
        exit 1
    fi
    if [[ $m == m-* ]] && ! grep -q "MODEL MUTANT BUILT: $m" "$out"; then
        echo "INVALID mutant $m: it did not build" >&2
        rm -f "$out"
        exit 1
    fi
    if [[ $rc -eq 0 ]]; then
        survived+=("$m")
        echo "START mutant $m -> survived"
    else
        killed=$((killed + 1))
        echo "START mutant $m -> killed ($(tail -1 "$out" | cut -c1-100))"
    fi
    rm -f "$out"
done
total=${#mutants[@]}
sorted=$(printf '%s\n' "${survived[@]:-}" | sort | tr '\n' ' ' | sed 's/ $//')
echo "== mutants: killed $killed of $total, survived: ${sorted:-none}"
if [[ "$sorted" != "$expected" ]]; then
    echo "unexpected mutant survival set (expected exactly: $expected)" >&2
    exit 1
fi
echo "OVL_0025_0341 native audit and mutants verified"
