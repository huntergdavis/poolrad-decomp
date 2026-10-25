#!/bin/sh
# Every free-model call goes through here: nice 19, at most 2 at once (flock,
# waits rather than fails), refuses any model id that is not free, writes
# stdout to a named file, and prints one report line.
#
# usage: free_run.sh --model ID --prompt-file FILE --out FILE [--timeout SECS]
#   ID is an OpenRouter ':free' id from `pi --list-models`
#   (run through pi, e.g. nvidia/nemotron-3-super-120b-a12b:free) or an
#   opencode zen id ending in '-free' (e.g. opencode/deepseek-v4.1-flash-free).
# report: FREE_RUN model=.. wait_s=.. wall_s=.. exit=.. out_bytes=.. slot=..
# events: appended to $FREE_RUN_EVENTS (default /tmp/free_run.events) as
#   "<epoch> START|END <slot> <model>", so max simultaneous is measurable.
set -u
SLOTS=2
EVENTS=${FREE_RUN_EVENTS:-/tmp/free_run.events}
TIMEOUT=900
MODEL= PF= OUT=
while [ $# -gt 0 ]; do
  case $1 in
    --model) MODEL=$2; shift 2;;
    --prompt-file) PF=$2; shift 2;;
    --out) OUT=$2; shift 2;;
    --timeout) TIMEOUT=$2; shift 2;;
    *) echo "free_run.sh: unknown argument $1" >&2; exit 64;;
  esac
done
[ -n "$MODEL" ] && [ -n "$PF" ] && [ -n "$OUT" ] || { echo "usage: free_run.sh --model ID --prompt-file FILE --out FILE [--timeout SECS]" >&2; exit 64; }
[ -r "$PF" ] || { echo "free_run.sh: cannot read prompt file $PF" >&2; exit 64; }

PI_FREE=$(pi --list-models 2>/dev/null | awk '$1=="openrouter" && $2 ~ /:free$/ {print $2}')
ZEN_FREE=$(opencode models 2>/dev/null | grep -E -- '-free$')
BACKEND=
if printf '%s\n' "$PI_FREE" | grep -Fxq -- "$MODEL"; then BACKEND=pi
elif printf '%s\n' "$ZEN_FREE" | grep -Fxq -- "$MODEL"; then BACKEND=opencode
fi
if [ -z "$BACKEND" ]; then
  echo "free_run.sh: REFUSED '$MODEL' is not a free model id (a paid call spends the \$1 key)." >&2
  echo "valid pi ids:" >&2; printf '  %s\n' $PI_FREE >&2
  echo "valid opencode ids:" >&2; printf '  %s\n' $ZEN_FREE >&2
  exit 65
fi

T0=$(date +%s)
SLOT=
while [ -z "$SLOT" ]; do
  i=1
  while [ $i -le $SLOTS ]; do
    exec 9>"/tmp/free_run.slot.$i"
    if flock -n 9; then SLOT=$i; break; fi
    exec 9>&-
    i=$((i+1))
  done
  [ -n "$SLOT" ] || sleep 3
done
T1=$(date +%s)
echo "$T1 START $SLOT $MODEL" >> "$EVENTS"
if [ $BACKEND = pi ]; then
  nice -n 19 timeout "$TIMEOUT" pi -p --no-session --provider openrouter --model "$MODEL" "$(cat "$PF")" > "$OUT" 2> "$OUT.err" 9>&-
else
  nice -n 19 timeout "$TIMEOUT" opencode run --pure -m "$MODEL" --format json "$(cat "$PF")" > "$OUT" 2> "$OUT.err" 9>&-
fi
RC=$?
T2=$(date +%s)
echo "$T2 END $SLOT $MODEL" >> "$EVENTS"
echo "FREE_RUN model=$MODEL backend=$BACKEND wait_s=$((T1-T0)) wall_s=$((T2-T1)) exit=$RC out_bytes=$(wc -c < "$OUT") slot=$SLOT"
exit $RC
