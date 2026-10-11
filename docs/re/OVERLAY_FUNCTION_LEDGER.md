# Overlay routine ledger: starts, anomalies and classification (issue #26)

**Scope.** The overlay portion of the game-function denominator, for the exact DOS v1.3
files in [overlay_inventory.md](overlay_inventory.md) (`START.EXE` sha256
`d825df2b…6a07bd`, `GAME.OVR` sha256 `872964a3…928614`). Baseline from
[OVERLAY_FUNCTION_INVENTORY.md](OVERLAY_FUNCTION_INVENTORY.md): 34 units, 197,451 code bytes,
673 Pascal-prologue candidates, 404 public entry slots, 668 candidates with entry or
decoded-call evidence, 5 without, 5 entry slots that match no prologue. Resident routines are
Codex's lane and are not touched here. Only offsets, counts and hashes appear below; the
per-address rows are in [overlay_function_ledger.csv](overlay_function_ledger.csv) and the
generated counts in [overlay_function_ledger_summary.md](overlay_function_ledger_summary.md),
both produced by `tools/overlay/function_ledger.py`.

## 1. The five anomalous public entries are aliases of the five unsupported prologues

| unit | slot | raw entry word | unit code size | raw word inside code? | fixup-confirmed callers of the slot | raw word minus 0x800 | prologue there? |
| --- | ---: | --- | ---: | --- | ---: | --- | --- |
| 0x19 | 1 | 0x08FD | 862 | no | 0 | 0x00FD | yes |
| 0x2f | 1 | 0x087A | 4,735 | yes | yes | 0x007A | yes |
| 0x31 | 1 | 0x080F | 6,439 | yes | yes | 0x000F | yes |
| 0x36 | 1 | 0x082D | 1,562 | no | 0 | 0x002D | yes |
| 0x3d2 | 1 | 0x0804 | 252 | no | yes | 0x0004 | yes |

Facts from the bytes:

- All five raw words equal the offset of the one prologue in the same unit that has no other
  evidence, plus exactly 0x800. The five unsupported prologues are precisely these five.
- The two in-code raw words (0x2f:0x087A, 0x31:0x080F) are not instruction boundaries: a
  linear decode from the prologue that precedes each lands between instructions. They are
  not starts.
- The five stubs look like every other stub: `INT 3Fh`, the word, a zero fifth byte.
- The resident image contains no instruction that masks, tests, adds or compares bit 11 of a
  word (`and/test/cmp/add/sub` with 0x0800, 0xF7FF or 0x07FF, or `test ah,8`): zero sites.
- Read as a displacement, three of the five raw words would point past the end of their
  unit's code; three of the slots have fixup-confirmed callers, so they are used. The
  run-time stub words (below) carry the normalized offsets.

Verdict: each slot is a **public entry alias** for the real procedure at `word - 0x800`.
The ledger resolves alias slots to their prologue before counting incoming calls, so the
three procedures whose alias slot has fixup-confirmed callers (0x2f:0x007A, 0x31:0x000F,
0x3d2:0x0004; four call sites) are grade A, and the two without callers (0x19:0x00FD,
0x36:0x002D) are grade D. The ledger keeps the raw descriptor words beside the normalized
offsets.

**Run-time stub words, established statically from the expanded resident image** (Codex's issue #28 loader
expansion, posted on issue #26 and cross-checked here with the same module against the
inventory JSON): the packed descriptor table in the `START.EXE` file and the run-time stub
table that the loader expands into memory are two different tables. Across all 404 public
entries the expanded stub at `memory_segment:0x20 + 5*slot` carries `INT 3Fh` (404 of 404),
and the word after it equals the raw descriptor word for 399 entries and the raw word
minus 0x800 for exactly the five above (expanded image offsets 0x1E5, 0x375, 0x3A5, 0x405,
0x3F05). The `INT 3Fh` handler installed through DOS function 25h at `0xAA2:0x02E6`
(expanded image 0xAD06) pushes the word that follows the stub's `INT 3Fh` and dispatches
on it, so the expanded stubs supply the normalized prologue offsets to the handler for these
five slots, with no subtraction in the handler. This is static evidence; no execution of
these slots has been observed yet. What remains **unresolved** is only the meaning of the +0x800 in the packed
descriptor words (linker flag, alias encoding or build artefact); other normal entries also
exceed 0x800, so bit 11 is not a general mask. My earlier note that the handler could not
be placed applied to the packed file layout only and is withdrawn.

The five procedures themselves, from private disassembly (identities only): 0x19:0x00FD
(604-byte bound, loops over a runtime random call and resident far calls), 0x2f:0x007A
(sets a procedure variable at `[0x6B84]` to unit 0x5e slot 14 and calls unit 0x38 slot 1),
0x31:0x000F (takes a far record pointer, calls units 0x3b0 and 0x9e), 0x36:0x002D (the
window and script-buffer allocator of ledger section 1.7), 0x3d2:0x0004 (string handling
with runtime calls). All five are ordinary Pascal procedures.

## 2. Starts, non-prologue starts and indirect targets

| measure | count | source |
| --- | ---: | --- |
| prologue candidates in unit code | 673 | inventory |
| grade A: entry slot on the prologue or fixup-confirmed far call to it (alias slots resolved first) | 402 | callgraph after the segment-word fixup correction (far-called 192, 4 of them through alias slots) |
| grade B: direct near-call target only | 269 | 966 call sites, 354 distinct targets; the raw exact-prologue scan is promoted only because the validation summary proves it identical to the Capstone-decoded set (invariants checked by the generator) |
| grade D: public entry alias only | 2 | section 1 |
| grade U: no evidence | 0 | |
| procedure-pointer constants (`mov dx,seg` fixups with a paired `mov ax,offset`) | 208 of 209 resolve to entry slots of 169 distinct prologues, all already grade A | this ledger |
| the one unresolved far-pointer constant | unit 0x5e offset 0x0151: inside the function at 0x0123, not an instruction boundary, so a typed constant in the code segment, not code | this ledger |
| indirect call sites through memory | 3 (two through the procedure variable at `[0x6B84]`, one through the table at `[0x6828]` that the 208 constants populate) | this ledger |
| raw E8 candidates not at prologues | 79, not promoted (the merged audit found none of them on a decoded path) | validate_near_calls.py |
| prologue byte pattern outside unit code | 2, both in the overlay file header's string table (file offsets 0x4A and 0xB4): data | this ledger |
| entry stubs whose fifth byte is non-zero | 10, each the last slot of its unit, where the fifth byte already belongs to what follows; the entry words are unaffected | this ledger |

No callable start without a prologue was found: every resolved far call, decoded near call,
procedure-pointer constant and public entry lands on one of the 673 prologues or on an alias
of one. What remains unverified is the inverse: that none of the 673 is a false match. The
linear decode from each prologue stops before the next prologue in 171 cases (inline
constants or jump tables after a return), which is normal for compiled Pascal and does not
affect the starts.

## 3. Classification: what the bytes support

- **The Turbo Pascal SYSTEM runtime is resident, not overlaid:** all 1,790 calls to segment
  0xAF8 leave the overlay. That excludes the SYSTEM unit from the overlay count. It does not
  show whether other linked library units (third-party or standard units compiled with
  overlays enabled) occupy some of the 34 overlay units; nothing in the bytes examined here
  decides that.
- **Measured behaviour classes** (from the bounded decode; not a semantic judgement):

| class | definition | count |
| --- | --- | ---: |
| state | touches at least one DS-relative global | 465 |
| glue | no DS global, makes calls | 122 |
| glue, runtime-only | no DS global, every call goes to 0xAF8 | 16 |
| leaf | no calls, no DS globals, no ES operands | 51 |
| leaf with ES operands | no calls, no DS globals, reads or writes through a far pointer | 19 |

- **What is not decided:** the semantic split of the 673 into game logic, library code and
  helper utilities is **unresolved**. The feature classes above are measurements of what
  each routine touches and calls; touching a DS global does not make a routine game logic,
  and being a leaf does not make it a library helper. No game-routine count or range is
  derived from them.

## 4. Denominator statement for the overlay portion

673 routine starts in `GAME.OVR`, every one with entry, call, or alias evidence (402 grade
A, 269 grade B, 2 grade D), none unsupported, none found outside the prologue set. How many
of them are game routines rather than library or helper code is not established. This is
the overlay side only; the resident side and the whole-executable coverage are tracked under
issue #24.

## 5. Reproduce

```sh
G=resources/DOS/POOLRAD; O=/path/outside/repo
python3 tools/overlay/overlay_inventory.py --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/overlay_inventory.json --md $O/overlay_inventory.md
python3 tools/overlay/callgraph.py --inventory $O/overlay_inventory.json --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/callgraph.json --md $O/callgraph_summary.md
python3 tools/overlay/validate_near_calls.py --inventory $O/overlay_inventory.json --callgraph $O/callgraph.json --ovr $G/GAME.OVR --json $O/near_call_evidence.json
python3 tools/overlay/function_ledger.py --inventory $O/overlay_inventory.json --callgraph $O/callgraph.json --near-calls $O/near_call_evidence.json --ovr $G/GAME.OVR --exe $G/START.EXE --csv docs/re/overlay_function_ledger.csv --md docs/re/overlay_function_ledger_summary.md
```

The last two need Capstone 5. The generator verifies the SHA-256 of every file it is given
against the inventory and refuses to write output on a mismatch; with `--near-calls` it also
requires the validation summary's invariants (no invalid or truncated paths, raw and decoded
exact-prologue call sets identical) before labelling near-call evidence as decoded. Without
`--ovr`, it omits the feature columns and the procedure-pointer evidence; without
`--near-calls`, grade B is labelled as raw, unvalidated.
