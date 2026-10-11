#!/usr/bin/env bash
# Reproduce the 16-bit decompiler feasibility spike with headless Ghidra.
# Requires: GHIDRA_HOME (Ghidra 12.x), JAVA_HOME (JDK 21+), python3, and the owner-supplied
# START.EXE / GAME.OVR. All outputs stay in SPIKE_DIR (keep it outside the repository: the
# decompiled C is derived from the game's code and must not be committed).
#   usage: run_spike.sh <START.EXE> <GAME.OVR> <SPIKE_DIR> [bare]
set -euo pipefail
EXE=$1; OVR=$2; export SPIKE_DIR=$3; MODE=${4:-}
mkdir -p "$SPIKE_DIR/proj"
HERE=$(cd "$(dirname "$0")" && pwd)
python3 "$HERE/spike_inputs.py" --exe "$EXE" --ovr "$OVR" --unit-ovr-offset 3398 --out "$SPIKE_DIR"
"$GHIDRA_HOME/support/analyzeHeadless" "$SPIKE_DIR/proj" spike_$RANDOM \
  -import "$SPIKE_DIR/resident.bin" -loader BinaryLoader -loader-baseAddr 0000:0000 \
  -processor "x86:LE:16:Real Mode" -noanalysis -scriptPath "$HERE" -postScript Spike.java $MODE -deleteProject \
  | sed -n 's/.*Spike.java> //p'
