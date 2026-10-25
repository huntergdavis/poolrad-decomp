#!/bin/sh
# Owner-host gate: all media-dependent unittest files, through the shared heavy lane.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
for media in START.EXE GAME.OVR; do
    if [ ! -f "$ROOT/resources/DOS/POOLRAD/$media" ]; then
        echo "media checks NOT RUN: missing resources/DOS/POOLRAD/$media" >&2
        exit 4
    fi
done
HEAVY=$(dirname "$ROOT")/poolrad-heavy.sh
if [ ! -x "$HEAVY" ]; then
    echo "media checks NOT RUN: heavy-job wrapper unavailable: $HEAVY" >&2
    exit 4
fi
cd "$ROOT"
exec "$HEAVY" python3 tools/run_media_checks.py
