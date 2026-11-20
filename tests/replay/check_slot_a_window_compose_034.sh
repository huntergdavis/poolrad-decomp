#!/usr/bin/env bash
# Run the run-034 window composition audit, then every mutant of its new branches.
# Private inputs stay under this tree's ignored resources/; only counts are printed.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 2 ]]; then
    echo "usage: $0 PRIVATE_RUN_t4492-slot6-window-034_DIR PRIVATE_oracle-runs_DIR" >&2
    exit 2
fi
resources=$(realpath -e -- "$repo_dir/resources")
run=$(realpath -e -- "$1")
runs=$(realpath -e -- "$2")
for d in "$run" "$runs"; do
    case "$d" in
        "$resources"/*) ;;
        *) echo "private inputs must stay in this tree's ignored resources/" >&2; exit 2 ;;
    esac
done
game="$resources/DOS/POOLRAD/GAME.OVR"
audit="$repo_dir/tests/replay/audit_slot_a_window_compose_034.py"

echo "== baseline (nine modeled spans, journal fallback, Run B controls)"
nice -n 15 python3 "$audit" "$run" "$game" "$runs"

# Mutants of the new branches.  Each must make the audit fail except the listed equivalent one:
# entry-ax perturbs AX at every modeled span's entry; no model reads AX there (it is dead input).
killed=0
survived=()
for m in entry-sp no-irq-merge bound-narrow skip-model-apply stale-ram drop-journal entry-ax; do
    if POOLRAD_COMPOSE_MUTANT=$m nice -n 15 python3 "$audit" "$run" "$game" "$runs" >/dev/null 2>&1; then
        survived+=("$m")
        echo "START mutant $m -> survived"
    else
        killed=$((killed + 1))
        echo "START mutant $m -> killed"
    fi
done
echo "== mutants: killed $killed of 7, survived: ${survived[*]:-none}"
if [[ "${survived[*]:-}" != "entry-ax" ]]; then
    echo "unexpected mutant survival set (expected exactly: entry-ax)" >&2
    exit 1
fi
echo "window composition audit and mutants verified"
