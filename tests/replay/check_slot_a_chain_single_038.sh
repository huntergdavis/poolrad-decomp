#!/usr/bin/env bash
# Run the run-038 single-invocation chain audit, then every mutant of its branches.
# Private inputs stay under this tree's ignored resources/; only counts are printed.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 0 ]]; then
    echo "usage: $0" >&2
    exit 2
fi
resources=$(realpath -e -- "$repo_dir/resources")
runs="$resources/oracle-runs"
run038=$(realpath -e -- "$runs/t4492-slot9-chain-038")
game=$(realpath -e -- "$resources/DOS/POOLRAD/GAME.OVR")
build="$resources/build-chain-single-038"
audit="$repo_dir/tests/replay/audit_slot_a_chain_single_038.py"

echo "== baseline"
nice -n 15 python3 "$audit" "$run038" "$runs" "$game" "$build"

# Mutants of the audit's branches.  Each must make it fail except the listed equivalent one: no-hw-frame drops the
# hardware IRQ0 frame stores, whose bytes are overwritten before 3492 (the same dead frame as in the 0747->3492 note).
killed=0
survived=()
for m in dup-exec no-hw-frame ivt-skip regsave-order read-pos read-skip tick-skip comp-in61-03 comp-bind-skip comp-entry-ds \
         xr-expect-equal xr-pin; do
    if POOLRAD_CHAIN_MUTANT=$m nice -n 15 python3 "$audit" "$run038" "$runs" "$game" "$build" >/dev/null 2>&1; then
        survived+=("$m")
        echo "START mutant $m -> survived"
    else
        killed=$((killed + 1))
        echo "START mutant $m -> killed"
    fi
done
echo "== mutants: killed $killed of 12, survived: ${survived[*]:-none}"
if [[ "${survived[*]:-}" != "no-hw-frame" ]]; then
    echo "unexpected mutant survival set (expected exactly: no-hw-frame)" >&2
    exit 1
fi
echo "chain single-invocation audit and mutants verified"
