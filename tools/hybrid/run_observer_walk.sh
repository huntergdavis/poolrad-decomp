#!/bin/bash
# Stage (b) observer run (VP i20735): the save-walk route under the observer DOSBox-X build, driven by
# tools/hybrid/walk_driver.py (each key sent when its screen shows). Usage:
#   run_observer_walk.sh RUN_NAME REGISTRY [STAGE_BUDGET_S=300] [TOTAL_BUDGET_S=1500] [STOP=walk|menu]
# Leave PR_HYBRID_DISPATCH unset for an observer-only run.
set -u
cd "$(dirname "$0")/../.."
NAME=$1; REG=$(realpath "$2")
RUN=$(tools/oracle/prepare_run.sh "$NAME") || exit 1
export PATH=$PWD/resources/hybrid/bin:$PATH PR_HYBRID_REGISTRY=$REG PR_HYBRID_LOG=$RUN/observer
echo "run=$RUN dosbox-x sha256=$(sha256sum "$(command -v dosbox-x)" | cut -c1-16)"
echo "dispatch=${PR_HYBRID_DISPATCH:-<unset: observer only>}"
xvfb-run -a -s "-screen 0 1024x768x24" python3 -u tools/hybrid/walk_driver.py "$RUN" "${3:-300}" "${4:-1500}" "${5:-walk}"
echo "driver_rc=$?"
echo "emulators left: $(pgrep -x dosbox-x | wc -l)"
head -1 "$RUN/observer.tsv"; echo "seq lines: $(wc -l < "$RUN/observer.seq")"; ls -l --time-style=+%T "$RUN/observer.tsv"
