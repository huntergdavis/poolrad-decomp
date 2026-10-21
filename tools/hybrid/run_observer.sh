#!/bin/bash
# Stage 0 observer run: one cold start of the audited v1.3 game under the observer DOSBox-X build, with Enter typed at 28 s
# (as tools/oracle/capture_menu.sh does), a screenshot at SHOT seconds, and DOSBox-X's own time limit. Usage:
#   run_observer.sh RUN_NAME REGISTRY [SHOT=55] [LIMIT=60]
set -u
cd "$(dirname "$0")/../.."
NAME=$1; REG=$(realpath "$2"); SHOT=${3:-55}; LIMIT=${4:-60}
RUN=$(tools/oracle/prepare_run.sh "$NAME") || exit 1
export PATH=$PWD/resources/hybrid/bin:$PATH PR_HYBRID_REGISTRY=$REG PR_HYBRID_LOG=$RUN/observer
echo "run=$RUN registry=$REG dosbox-x=$(command -v dosbox-x)"
xvfb-run -a -s "-screen 0 1024x768x24" bash -c "
  tools/oracle/launch.sh '$RUN' -set 'cpu core=normal' -time-limit $LIMIT -c 'AUTOTYPE -w 28 enter' > '$RUN/launch.log' 2>&1 &
  pid=\$!
  sleep $SHOT; import -window root '$RUN/screen-${SHOT}s.png'
  wait \$pid; echo launch_rc=\$?
"
echo "emulators left: $(pgrep -x dosbox-x | wc -l)"
head -1 "$RUN/observer.tsv"
