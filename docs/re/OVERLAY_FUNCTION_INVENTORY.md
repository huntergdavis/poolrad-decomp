# Overlay function-count evidence

This is the **raw candidate baseline**, not the denominator for game-function
completion. Its five apparent entry exceptions and five candidates without
listed callers were subsequently resolved in the
[canonical overlay function ledger](OVERLAY_FUNCTION_LEDGER.md) by checking
the expanded runtime stubs and normalizing five `+0x800` descriptor aliases.
The raw counts below remain reproducible and show why that later audit was
needed. This inventory covers the exact DOS v1.3 files identified by the SHA-256 hashes
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
| Prologues targeted by a decoded direct near call | 354 | 966 call instructions reached by the boundary-aware walk |
| Prologues with entry, far-call, or decoded near-call evidence | 668 | Union of the sets above, not their sum |
| Prologues without any listed entry/call evidence | 5 | May be dead, indirectly called, or false matches |

In this raw baseline, five public entry offsets do not coincide with a
unit-code prologue. Three (`0x19` entry 1, `0x36` entry 1, `0x3d2` entry 1)
exceed their unit's code size; the latter has a fixup-confirmed far-call
candidate. The two in-bounds cases (`0x2f` entry 1 and `0x31` entry 1)
required instruction-boundary review. The later ledger established that each
raw word is exactly `0x800` above one of the five otherwise unsupported
prologues. Its expanded runtime stubs contain the normalized offsets, and the
two in-bounds raw words are not instruction starts. They are **not** added as
five additional functions: all five are already among the 673 candidates.
A Capstone 5 path walk from each candidate
prologue decoded 71,693 instruction positions, including all 966 direct near
calls that the earlier byte scan associated with exact prologues. Those calls
target 354 distinct starts; 269 of them lack a public entry. No path failed to
decode or hit the per-seed step limit. This validates call instruction boundaries
*under the assumption that each prologue seed is a function start*. It does not
settle indirect calls, code before the first prologue, inline data, or whether
every seed is a real game routine. The raw scan's other 79 near-call candidates
are not promoted by this audit.

The final overlay unit (`0x40c`) has no successor descriptor to record its
run-time stub segment. Late descriptor spacing predicts segment `0x428`, and
seven fixup-confirmed calls to two of its entry slots corroborate the mapping.
The corrected call graph classifies 1,161 far-call sites as overlay and 1,061
as non-runtime resident. The resident target classification remains tracked in
issue #24.

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
# Requires Capstone 5 with 16-bit x86 support:
python3 tools/overlay/validate_near_calls.py \
  --inventory /tmp/poolrad-functions-inventory.json \
  --callgraph /tmp/poolrad-functions-callgraph.json \
  --ovr "$G/GAME.OVR"
```

The later [overlay function ledger](OVERLAY_FUNCTION_LEDGER.md) resolves all
five aliases, validates the 966 direct near-call sites to prologues, and gives
all 673 candidate starts entry, call, or alias evidence (402 grade A, 269 B,
two D, zero unsupported). This strengthens start evidence; it does not prove
that all 673 are game-authored routines or that no starts exist outside the
candidate set. [Issue #24](https://github.com/huntergdavis/poolrad-decomp/issues/24)
continues the executable-region, indirect-path, resident-start, and
game-versus-linked-library audit. Each accepted original routine needs a
stable address ID and evidence grade. The portable C implementation may
combine or split original routines, so the progress numerator tracks reviewed
original behavior covered rather than C symbol counts.
