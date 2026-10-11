#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 PREPARED_RUN_DIR [DOSBOX_X_OPTIONS...]" >&2
  exit 2
fi

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)
run_dir=$(realpath -e -- "$1")
shift
if [[ $run_dir != "$repo_dir"/resources/oracle-runs/* || ! -f $run_dir/.oracle-input-sha256 ]]; then
  echo "Expected a run prepared under this repository's resources/oracle-runs" >&2
  exit 1
fi
command -v bwrap >/dev/null
command -v dosbox-x >/dev/null

mkdir -p "$run_dir/.config" "$run_dir/.cache" "$run_dir/.local/share" "$run_dir/.dosbox-saves"
export XDG_CONFIG_HOME=$run_dir/.config
export XDG_CACHE_HOME=$run_dir/.cache
export XDG_DATA_HOME=$run_dir/.local/share
export SDL_AUDIODRIVER=dummy

exec bwrap --unshare-net --ro-bind / / --dev-bind /dev /dev \
  --bind "$run_dir" "$run_dir" --chdir "$run_dir" \
  dosbox-x -defaultconf -defaultmapper -nogui -fastlaunch \
  -savedir "$run_dir/.dosbox-saves" "$@" \
  -c "mount c $run_dir" -c 'c:' -c 'START'
