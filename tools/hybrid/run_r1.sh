#!/usr/bin/env bash
# Invoke through the site's heavy-job FIFO wrapper. Uses a private X display.
set -euo pipefail
cd "$(dirname "$0")/../.."
if [[ $# -lt 2 || $# -gt 3 || ! $2 =~ ^(original|observer)$ ]]; then
    echo "usage: WALK_ROUTE=r1 [R1_SEED_PIN=12345678] $0 RUN_NAME original|observer [REGISTRY]" >&2
    exit 2
fi
mode=$2
route=${WALK_ROUTE:-r1}
seed=${R1_SEED_PIN:-}
[[ $route =~ ^[A-Za-z0-9_-]+$ && -f tools/hybrid/routes/$route.json ]] || { echo 'unknown route' >&2; exit 2; }
[[ -z $seed || $seed =~ ^[[:xdigit:]]{8}$ ]] || { echo 'seed must be eight hex digits' >&2; exit 2; }
registry=''
if [[ $# == 3 ]]; then
    registry=$(realpath -e "$3")
elif [[ $mode == observer ]]; then
    echo 'observer mode requires a registry' >&2
    exit 2
elif [[ -n $seed ]]; then
    registry=$(realpath -e resources/hybrid/registry_pin.tsv)
fi
if [[ -n $seed ]]; then
    awk -F '\t' '$1 ~ /^DRAW_/ && $6 == "unique" { found=1 } END { exit !found }' "$registry" || {
        echo 'seed pin requires a unique DRAW_ registry row' >&2; exit 2;
    }
fi
# Preserve only explicit harness inputs, never inherited hybrid controls.
while IFS= read -r name; do unset "$name"; done < <(compgen -A variable PR_HYBRID_)
export PATH="$PWD/resources/hybrid/bin:$PATH"
[[ -x resources/hybrid/bin/dosbox-x ]] || { echo 'private emulator missing' >&2; exit 2; }
run=$(tools/oracle/prepare_run.sh "$1")
printf 'run=%s mode=%s route=%s seed=%s dispatch=unset\n' "$run" "$mode" "$route" "${seed:-unset}"
printf '%s\n' "$mode" > "$run/r1-mode.txt"
printf '%s\n' "$seed" > "$run/r1-seed.txt"
cp "tools/hybrid/routes/$route.json" "$run/r1-route.json"
cp tools/hybrid/r1_driver.py "$run/r1-driver.py"
sha256sum "$(command -v dosbox-x)" "$run/r1-driver.py" "$run/r1-route.json" > "$run/r1-hashes.txt"
if [[ -n $registry ]]; then
    cp -- "$registry" "$run/r1-registry.tsv"
    export PR_HYBRID_REGISTRY=$run/r1-registry.tsv PR_HYBRID_LOG=$run/observer
    sha256sum "$run/r1-registry.tsv" >> "$run/r1-hashes.txt"
fi
if [[ -n $seed ]]; then
    export PR_HYBRID_SEED_PIN=$seed
fi
xvfb-run -a -s '-screen 0 1024x768x24' python3 -u "$run/r1-driver.py" "$run" --route "$run/r1-route.json" --total "${R1_TOTAL_S:-2400}" > "$run/r1-driver.log" 2>&1
if [[ -n $seed ]]; then
    rg -q "^# PINNED RandSeed := ${seed^^} " "$run/observer.draws" || {
        echo 'route ended without the requested seed pin' >&2; exit 11;
    }
fi
python3 tools/hybrid/r1_report.py "$run" > "$run/r1-report.log"
