#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 || ! $1 =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; then
  echo "usage: $0 RUN_NAME (letters, digits, dot, underscore, hyphen)" >&2
  exit 2
fi

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)
source_dir=$repo_dir/resources/DOS/POOLRAD
run_root=$repo_dir/resources/oracle-runs
run_dir=$run_root/$1

if [[ ! -d $source_dir || -e $run_dir ]]; then
  echo "DOS input is missing or run already exists: $run_dir" >&2
  exit 1
fi
if [[ -n $(find "$source_dir" -type l -print -quit) ]]; then
  echo "DOS input contains a symlink; refusing to copy" >&2
  exit 1
fi

expected_start=d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
expected_overlay=872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614
actual_start=$(sha256sum "$source_dir/START.EXE" | cut -d ' ' -f 1)
actual_overlay=$(sha256sum "$source_dir/GAME.OVR" | cut -d ' ' -f 1)
if [[ $actual_start != "$expected_start" || $actual_overlay != "$expected_overlay" ]]; then
  echo "DOS executable hashes differ from the audited v1.3 build" >&2
  exit 1
fi

umask 077
mkdir -p "$run_root"
temporary_dir=$(mktemp -d "$run_root/.${1}.tmp.XXXXXX")
trap 'rm -rf -- "$temporary_dir"' EXIT

(cd "$source_dir" && find . -type f -print0 | sort -z | xargs -0 -r sha256sum) > "$temporary_dir/.oracle-input-sha256"
cp -a -- "$source_dir/." "$temporary_dir/"
(cd "$temporary_dir" && sha256sum --status -c .oracle-input-sha256)
chmod -R u+w "$temporary_dir"
mkdir "$temporary_dir/.dosbox-saves"
mv -- "$temporary_dir" "$run_dir"
trap - EXIT
printf '%s\n' "$run_dir"
