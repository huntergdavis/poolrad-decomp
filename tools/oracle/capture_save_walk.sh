#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 PREPARED_RUN_DIR" >&2
  exit 2
fi

run_dir=$(realpath -e -- "$1")
command -v import >/dev/null
command -v tesseract >/dev/null

# DOSBox-X queues these keys internally. Observe the position display rather
# than assuming a fixed time for disk loading or scene rendering.
tools/oracle/launch.sh "$run_dir" -time-limit 90 \
  -c 'AUTOTYPE -w 28 -p 4 enter l , a , up' \
  > "$run_dir/slot-a-walk.log" 2>&1 &
emulator_pid=$!
trap 'kill "$emulator_pid" 2>/dev/null || true' EXIT

seen_start=0
deadline=$((SECONDS + 82))
while ((SECONDS < deadline)); do
  if ! kill -0 "$emulator_pid" 2>/dev/null; then
    break
  fi
  sleep 2
  import -window root "$run_dir/slot-a-current.png"
  screen_text=$(tesseract "$run_dir/slot-a-current.png" stdout --psm 11 \
    -c tessedit_char_whitelist=0123456789,NSWE: 2>/dev/null || true)
  if [[ $screen_text == *'15,4'* && $seen_start -eq 0 ]]; then
    cp -- "$run_dir/slot-a-current.png" "$run_dir/slot-a-before.png"
    seen_start=1
  fi
  if [[ $screen_text == *'14,4'* && $seen_start -eq 1 ]]; then
    cp -- "$run_dir/slot-a-current.png" "$run_dir/slot-a-after.png"
    printf 'verified saved party movement 15,4 -> 14,4; captures: %s\n' "$run_dir"
    exit 0
  fi
done

echo "Save A did not show the expected 15,4 -> 14,4 movement within 82 seconds" >&2
exit 1
