#!/usr/bin/env bash
# Run the run-040 OVL_007F_0294 native audit (replay, IRET pops, C composition), then every mutant of its branches.
# Private inputs stay under this tree's ignored resources/; only counts are printed. A full pass takes about 40 minutes.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 0 ]]; then
    echo "usage: $0" >&2
    exit 2
fi
resources=$(realpath -e -- "$repo_dir/resources")
run040=$(realpath -e -- "$resources/oracle-runs/t578-ovl7f0294-040")
game=$(realpath -e -- "$resources/DOS/POOLRAD/GAME.OVR")
build="$resources/build-ovl7f0294-native-t578"
audit="$repo_dir/tests/replay/audit_slot_a_ovl7f0294_native_t578.py"

echo "== baseline"
nice -n 15 python3 -B "$audit" "$run040" "$game" "$build"

# Killed = the audit exits non-zero with a checked message (a traceback never counts, and a model mutant counts only once it built).
# The six survivors are classified in docs/re/SLOT_A_OVL7F0294_NATIVE_T578.md: three equivalent (pair-dup, comp-entry-cf, m-clamp-always)
# and three gaps the call does not exercise (m-esc-1c, m-bs-inc, m-limit-plus1).
mutants=(kbd-skip tick-skip no-hw-frame int-frame-skip host-inject-skip host-tail-skip pair-dup
         comp-entry-cf comp-keys-merged comp-out-drop
         m-enter-0e m-esc-1c m-clamp-always m-result-plus1 m-bs-inc m-limit-plus1 m-space-027 m-colour-0e m-frame-size)
expected="comp-entry-cf m-bs-inc m-clamp-always m-esc-1c m-limit-plus1 pair-dup"
killed=0
survived=()
for m in "${mutants[@]}"; do
    out=$(mktemp -p "$build")
    rc=0
    POOLRAD_T578_MUTANT=$m nice -n 15 python3 -B "$audit" "$run040" "$game" "$build" >"$out" 2>&1 || rc=$?
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
        echo "START mutant $m -> killed"
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
echo "OVL_007F_0294 native audit and mutants verified"
