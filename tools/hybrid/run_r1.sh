#!/usr/bin/env bash
# Invoke through the site's heavy-job FIFO wrapper. Uses a private X display.
set -euo pipefail
cd "$(dirname "$0")/../.."
if [[ $# -lt 2 || $# -gt 3 || ! $2 =~ ^(original|observer)$ ]]; then
    echo "usage: $0 RUN_NAME original|observer [REGISTRY]" >&2
    exit 2
fi
mode=$2
registry=''
if [[ $mode == observer ]]; then
    [[ $# == 3 ]] || { echo 'observer mode requires a registry' >&2; exit 2; }
    registry=$(realpath -e "$3")
fi
# Clear all inherited hybrid controls, including mutation, route and seed pins.
while IFS= read -r name; do unset "$name"; done < <(compgen -A variable PR_HYBRID_)
export PATH="$PWD/resources/hybrid/bin:$PATH"
[[ -x resources/hybrid/bin/dosbox-x ]] || { echo "private emulator missing" >&2; exit 2; }
run=$(tools/oracle/prepare_run.sh "$1")
if [[ $mode == observer ]]; then
    export PR_HYBRID_REGISTRY=$registry PR_HYBRID_LOG=$run/observer
fi
printf 'run=%s mode=%s dispatch=unset\n' "$run" "$mode"
sha256sum "$(command -v dosbox-x)" tools/hybrid/r1_driver.py tools/hybrid/routes/r1.json > "$run/r1-hashes.txt"
printf '%s\n' "$mode" > "$run/r1-mode.txt"
cp tools/hybrid/routes/r1.json "$run/r1-route.json"
cp tools/hybrid/r1_driver.py "$run/r1-driver.py"
if [[ -n $registry ]]; then
    cp -- "$registry" "$run/r1-registry.tsv"
    export PR_HYBRID_REGISTRY=$run/r1-registry.tsv
    sha256sum "$run/r1-registry.tsv" >> "$run/r1-hashes.txt"
fi
xvfb-run -a -s '-screen 0 1024x768x24' python3 -u "$run/r1-driver.py" "$run" --route "$run/r1-route.json" --total "${R1_TOTAL_S:-2400}" > "$run/r1-driver.log" 2>&1
