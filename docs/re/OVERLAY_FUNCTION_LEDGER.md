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
- Turbo Pascal's overlay manager rewrites a loaded unit's stubs into 5-byte far jumps to
  `code segment:word`. Read as a displacement, three of the five words would jump past the
  end of their unit's code, so a displacement reading is inconsistent with execution.
  Three of the slots have fixup-confirmed callers, so they are used.

Verdict: each slot is a **public entry alias** for the real procedure at `word - 0x800`
(grade D in the ledger). Whether 0x800 is a flag consumed by the overlay manager or an
artefact of how these five entries were linked is **unresolved**: the INT 3Fh handler
(installed at offset 0x00D7 of its unit, through DOS function 25h) could not be placed in
the file because the resident image's run-time layout differs from the file layout, and no
bit-11 operation exists anywhere in the image. A DOS trace of one call to slot 0x2f:1,
0x31:1 or 0x3d2:1 would settle the mechanism. The ledger keeps the raw words and shows the
normalized offsets beside them.

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
| grade A: entry slot on the prologue or fixup-confirmed far call to it | 399 | callgraph after the segment-word fixup correction (far-called 189) |
| grade B: decoded direct near-call target only | 269 | 966 call sites, 354 distinct targets (validate_near_calls.py; the raw scan matches it exactly) |
| grade D: public entry alias only | 5 | section 1 |
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

- **Runtime and library code is not in the overlay.** All 1,790 calls to the Turbo Pascal
  SYSTEM segment (0xAF8) leave the overlay; the overlay's 34 units are the game's own
  compiled units. So the overlay portion of the denominator contains no compiler runtime
  routines to exclude.
- **Measured behaviour classes** (from the bounded decode; not a semantic judgement):

| class | definition | count |
| --- | --- | ---: |
| state | touches at least one DS-relative global | 465 |
| glue | no DS global, makes calls | 122 |
| glue, runtime-only | no DS global, every call goes to 0xAF8 | 16 |
| leaf | no calls, no DS globals, no ES operands | 51 |
| leaf with ES operands | no calls, no DS globals, reads or writes through a far pointer | 19 |

- **What is not decided:** which of the 673 are game logic and which are the game's own
  small utilities. The only byte-level candidates for generic helpers are the 16 runtime-only
  glue functions and the 51 pure leaves (67 in all); the other 606 touch game state or call
  game code. A semantic game-versus-helper split for those 606 needs per-function review and
  is **unresolved**; the denominator should carry the range 606 to 673 for "game routines in
  the overlay" until that review exists.

## 4. Denominator statement for the overlay portion

673 routine starts in `GAME.OVR`, every one with entry, call, or alias evidence (399 / 269 /
5), none unsupported, none found outside the prologue set. This is the overlay side only;
the resident side and the whole-executable coverage are tracked under issue #24.

## 5. Reproduce

```sh
G=resources/DOS/POOLRAD; O=/path/outside/repo
python3 tools/overlay/overlay_inventory.py --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/overlay_inventory.json --md $O/overlay_inventory.md
python3 tools/overlay/callgraph.py --inventory $O/overlay_inventory.json --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/callgraph.json --md $O/callgraph_summary.md
python3 tools/overlay/validate_near_calls.py --inventory $O/overlay_inventory.json --callgraph $O/callgraph.json --ovr $G/GAME.OVR --json $O/near_call_evidence.json
python3 tools/overlay/function_ledger.py --inventory $O/overlay_inventory.json --callgraph $O/callgraph.json --near-calls $O/near_call_evidence.json --ovr $G/GAME.OVR --exe $G/START.EXE --csv docs/re/overlay_function_ledger.csv --md docs/re/overlay_function_ledger_summary.md
```

The last two need Capstone 5. Without `--ovr`, the ledger still grades starts from the
inventory and call graph but omits the feature columns and the procedure-pointer evidence.
