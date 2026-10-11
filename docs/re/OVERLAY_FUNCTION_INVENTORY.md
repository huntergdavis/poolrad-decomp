# Overlay function-count evidence

This is a **candidate inventory**, not the denominator for game-function
completion. It covers the exact DOS v1.3 files identified by the SHA-256 hashes
in [overlay_inventory.md](overlay_inventory.md). The existing inventory parses
all 34 `GAME.OVR` units without a file gap; the existing call graph resolves
fixup-confirmed far calls but scans raw bytes for near-call opcodes. This report
reuses those two published analyses. An exact `deja` query for prior Pool of
Radiance function-inventory work returned no matching session.

| Measure | Count | What it means |
| --- | ---: | --- |
| Overlay code bytes | 197,451 | Excludes fixup tables and the file header |
| Public entry slots | 404 | Stub slots, not 404 additional functions |
| Pascal-style prologue hits in unit code | 673 | `55 89 E5` byte pattern; a candidate start, not proof |
| Prologue hits in the entire overlay file | 675 | Includes two hits outside unit code |
| Public entries exactly on a prologue hit | 399 | A stronger candidate start; overlaps the 673 |
| Prologues targeted by a fixup-confirmed overlay far call | 187 | All are among the 399 public-entry matches |
| Prologues targeted by a raw near-call candidate | 354 | Instruction boundaries are not yet checked |
| Prologues with entry, far-call, or raw near-call evidence | 668 | Union of the sets above, not their sum |
| Prologues without any listed entry/call evidence | 5 | May be dead, indirectly called, or false matches |

Five public entry offsets do not coincide with a unit-code prologue. Three
(`0x19` entry 1, `0x36` entry 1, `0x3d2` entry 1) exceed their unit's code size;
the latter has one fixup-confirmed far-call candidate. The two in-bounds cases
(`0x2f` entry 1 and `0x31` entry 1) need instruction-boundary review. They are
**not** added as five functions. Similarly, the 269 prologue starts supported
only by a byte-scan near-call candidate need decoding before promotion.

Run the count from owner-supplied, ignored DOS files:

```sh
G=resources/DOS/POOLRAD
python3 tools/overlay/overlay_inventory.py --exe "$G/START.EXE" \
  --ovr "$G/GAME.OVR" --json /tmp/poolrad-functions-inventory.json
python3 tools/overlay/callgraph.py \
  --inventory /tmp/poolrad-functions-inventory.json \
  --exe "$G/START.EXE" --ovr "$G/GAME.OVR" \
  --json /tmp/poolrad-functions-callgraph.json
python3 tools/overlay/function_evidence.py \
  --inventory /tmp/poolrad-functions-inventory.json \
  --callgraph /tmp/poolrad-functions-callgraph.json
```

To establish a denominator, [issue #24](https://github.com/huntergdavis/poolrad-decomp/issues/24)
tracks three remaining checks: validate instruction boundaries and exceptional
entries, inventory and classify resident routines, and distinguish game logic
from Turbo Pascal runtime or library code. Each accepted original routine will
have one stable address ID and evidence grade. The portable C implementation
may combine or split original routines, so the progress numerator will track
reviewed original behavior covered rather than count C symbols.
