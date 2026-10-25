#!/usr/bin/env bash
# Disposable runs only; invoke through the shared heavy-job queue.
set -euo pipefail
cd "$(dirname "$0")/../.."
if [[ $# != 3 || ! $2 =~ ^(original|replacement)$ || ! $3 =~ ^(loaded|turn)$ ]]; then
    echo 'usage: run_parity.sh RUN_NAME original|replacement loaded|turn' >&2
    exit 2
fi
while IFS= read -r name; do unset "$name"; done < <(compgen -A variable PR_HYBRID_)
while IFS= read -r name; do unset "$name"; done < <(compgen -A variable PR_PARITY_)
export PATH="$PWD/resources/hybrid/bin:$PATH"
[[ -x resources/hybrid/bin/dosbox-x ]] || { echo 'private checkpoint build missing' >&2; exit 2; }
run=$(tools/oracle/prepare_run.sh "$1")
cp resources/hybrid/registry_all.tsv "$run/registry.tsv"
export PR_HYBRID_REGISTRY="$run/registry.tsv" PR_HYBRID_LOG="$run/observer"
export PR_PARITY_TARGET=RES_0A40_02FD PR_PARITY_PREFIX="$run/checkpoint"
if [[ $2 == replacement ]]; then
    export PR_HYBRID_BUS_MODE=route
    export PR_HYBRID_DISPATCH
    PR_HYBRID_DISPATCH=$(python3 -c 'import re; from pathlib import Path; print(",".join(re.findall(r"\{\"((?:RES|OVL)_[A-F0-9_]+)\",", Path("resources/hybrid/dosbox-x-src/src/cpu/pr_hybrid_dispatch.h").read_text())))')
    [[ -n $PR_HYBRID_DISPATCH ]]
fi
printf '%s\n' "$2" > "$run/parity-mode.txt"
printf '%s\n' "${PR_HYBRID_DISPATCH:-}" > "$run/parity-dispatch.txt"
printf '%q ' "$0" "$@" > "$run/parity-command.txt"
printf '\n' >> "$run/parity-command.txt"
git rev-parse HEAD > "$run/parity-revision.txt"
sha256sum resources/hybrid/bin/dosbox-x resources/hybrid/pr_models.o \
    resources/hybrid/pr_models.sources resources/hybrid/model-source-sha256 \
    tools/hybrid/parity_checkpoint.h tools/hybrid/parity_driver.py \
    tools/hybrid/run_parity.sh tools/hybrid/dosbox-x-5a9c7c53-dispatch.patch > "$run/parity-build-sha256"
xvfb-run -a -s '-screen 0 1024x768x24' python3 -u tools/hybrid/parity_driver.py "$run" "$3" > "$run/parity-driver.log" 2>&1
echo "$run"
