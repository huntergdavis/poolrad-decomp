# RES_00BA_16EC admitted-entry contract and read-time message bound (t24, NC1 and NC5 evidence)

**Proposed static evidence for two open closure reasons; nothing is discharged here.**
This document and `tools/overlay/res16ec_entry_contract.py` enumerate every
direct entry of `00BA:16EC`, classify each entry's argument setup, derive a
read-time bound on the message length for each entry, and check the selector-read
invariant. It is original-byte static analysis: no instruction is executed, no
natural arrival is claimed, and it confers no identity, readiness, membership,
closure, credit or unlock. The [closure record](RES_00BA_16EC_CLOSURE_STATUS.md)
keeps NC1 and NC5 open until an independent reviewer accepts an exact head and
records the discharge there. The [t3668 closure map](../../resources/t3668-nc-closure-map/README.md)
(private, SHA-256 `678534bf…`) proposed this order; the t100 and t3667 identity
results are untouched.

Pinned inputs: START.EXE `d825df2b…` and GAME.OVR `872964a3…` (the project
digests), the loader-expanded image `a42a57e6…`, and the 247-byte 16EC body
`c8507a56…`. Original text never leaves `resources/`; the tables below carry
offsets, lengths, counts and hashes only.

## NC1: the admitted entries

The checker rederives the entry set from the bytes and refuses on any change.

| Entry kind | How found | Count |
| --- | --- | ---: |
| Relocated overlay far call `9A EC 16 BA 00` with a fixup on the segment word | `far_call_sites.sites` over every unit | **63** |
| Resident near call `0E E8 rel16` to `16EC` inside segment `00BA` | raw scan of `00BA:[0000,3370)` (up to the next resident segment `03F1`), each confirmed as an aligned `PUSH CS; CALL` in its ledger owner's linear decode | **4** |

The 63 far sites are owned by 53 `game_evidenced` overlay bodies in units
`1E`, `2F`, `31`, `38` (23 sites), `5E`, `6A`, `77`, `86` and `9E`; the four
near sites are `00BA:0A1A` (`RES_00BA_09E3`), `208E` and `218F`
(`RES_00BA_1FEB`) and `239A` (`RES_00BA_234A`). The checker pins all 67
rows as `(site, owner, kind, row, wait, message class, bound, title source)`.

**Frame ABI at entry** (from the pinned body, see [RES_00BA_16EC_CPU.md](RES_00BA_16EC_CPU.md)):
`[BP+6]` wait byte, `[BP+8]` row byte, `[BP+0A/0C]` message far pointer,
`[BP+0E/10]` the title pointer forwarded to `1816`; `RETF 0Ch`.

**Argument classes measured across the 67 entries:**

| Argument | Values seen | Count |
| --- | --- | ---: |
| wait byte | immediate 1 / immediate 0 | 48 / 19 |
| row byte | immediate 10 | 58 |
| | immediate 12 | 3 |
| | immediate 0 | 2 |
| | frame byte `[BP-1]` (two sites in `OVL_005E_02F9`), `[BP-2]` (`00BA:0A1A`) | 3 |
| | `DS:5E25 + 1`, the text cursor row (`009E:15F6`) | 1 |
| message | a stack temp loaded from a CS literal by SYSTEM `0B0B` immediately before the call | 56 |
| | a stack local written earlier in the owner body | 10 |
| | a stack temp loaded from a CS literal then concatenated once (`0038:2191`) | 1 |
| title pointer | the owner's own `[BP+k]` argument / a DS far pointer (`5D92`, `6826`, `6C95`) / an owner local | 40 / 21 / 6 |

Every row and wait byte is pushed with `MOV AL,imm; PUSH AX` or the two
frame/DS forms above; `AH` is whatever the previous instruction left, so only
the low byte is contracted, as the body reads only `[BP+6]` and `[BP+8]` bytes.

**Selector paths.** 16EC chooses its branch from two data cells read after
StrStore: `DS:49F3` at `1707` (`== 5` bypasses the box) and `DS:6DDA` at
`1757` (`== 0` draws the box). The admitted entries do not pass these; they
are game state. The checked invariant is that the two reads observe the
entry-time values:

1. The pinned prologue `16EC..1702` writes only the frame (`SS:[BP-10Ah,BP)`)
   and then StrStore writes `D = SS:[BP-100h]` with `m = FFh`, at most 256
   bytes. `170C JNE 1757` is a direct jump: no write lies between the reads.
2. The real stack segment is not the MZ header's: START.EXE is EXEPACK-style
   packed, and the pack header at the entry (`RB` signature) carries
   `real_ss = 14F2h`, `real_sp = 4000h`. SYSTEM init at `0AF8:0000` sets
   `DS = 0C7Ch`. Both are pinned.
3. `0C7Ch*16 + 6DDAh = 1359Ah < 14F2h*16 = 14F20h`: every `SS:offset` lies
   above both cells by at least **6,534 bytes**, so no frame or StrStore write
   can alias them, for every admitted entry and every entry SP.

Therefore each admitted entry's selector path is one of three, all inside the
domain the [#1558 audit](RES_00BA_16EC_CPU.md) already compares against the
original over 388 full-state cases: `mode == 5` (bypass), `mode != 5 && colour == 0`
(box), `mode != 5 && colour != 0` (colour 12h, no box). The audit's case
matrix covers all four selector combinations and both wait values.

**A coverage gap worth closing (not a correctness claim):** the audit's rows
are 0..9 and 13..19; the admitted entries' dominant rows **10 and 12** are
not among its cases. The raw-CPU model treats the row as data, so this is a
missing sample, not a known divergence. Adding `row=10`/`row=12` cases to the
audit is a one-line change proposed for the next audit rerun.

## NC5: read-time message length bound per entry

The 255-byte printer case is reached only when the local copy at `[BP-100h]`
has length `FFh` at the printer's read. StrStore caps the copy at the source
length, so the bound is the source string's length at the call.

For each entry the checker runs a forward dataflow over the owner body's
direct-jump CFG with a per-local length bound (join = maximum, unknown = 255):

- SYSTEM `0B0B` StrLoad(D, S) sets `D := bound(S)`; `0B98` StrCat adds `S`
  to the chain destination and clamps at 255; `0B25` StrStore(S, D, m) sets
  `D := min(m, bound(S))` (only the low byte of `m` limits the copy);
- a CS literal's bound is its length byte; a `[BP+k]` string parameter is
  bounded by a depth-one caller census (every `PUSH CS; CALL` and relocated
  far caller of the owner) or 255 when none is found; the result of any
  other call is 255;
- a local whose address is passed to any other callee becomes 255; a direct
  byte store `MOV byte [BP-n],imm` sets the length to `imm`; any other
  direct write covering the length byte sets 255;
- every write through a register pointer, whatever the pointer's syntactic
  origin (a parameter's contents may be a stack address: t3683 R1), every
  write through `SS` or an explicit `ES`/`CS` segment, and every string store
  through a tainted `ES:DI`, sets every local to 255. A DS-absolute store is
  safe only when `DS` is untainted and its physical range lies below the
  stack base (`0C7Ch*16 + offset + size <= 14F2h*16`); writes through
  `[BP+k]` (the caller's frame) cannot reach this body's locals. The taint
  set (registers, segment registers and locals holding `LEA [BP..]`, `SS`,
  `BP`, `SP`, `POP` or call-clobbered values) decides which loads are stack
  pointers (t3677 R1);
- any direct call other than the three SYSTEM string leaves and 16EC itself
  sets every local at or above the lowest address this body has taken so far
  to 255, and everything when a stack segment or tainted register is pushed
  to it outside a recognised unit or a stack address was stored to non-local
  memory earlier; an indirect call sets every local to 255 (t3683 R3);
- the backward walk over pushed arguments and every multi-instruction unit it
  recognises (a far-pointer triple, an immediate push, a StrLoad/StrCat chain)
  must be contiguous: an instruction entered by a jump ends the walk, so a
  setup a path can skip is never attributed to the call (t3677 R2); an entry
  whose message cannot be resolved that way is refused rather than bounded;
- a body is decoded linearly from its ledger start to its own epilogue
  return; indirect jumps or jumps leaving the decode refuse the body, and an
  indirect call sets every local to 255.

| Message class | Entries | Bound derivation | Max |
| --- | ---: | --- | ---: |
| literal temp | 56 | the literal's length byte | 26 |
| local, StrStore-capped | 6 | `min(cap, source)`: caps `28h` or `50h` with literal or parameter sources | 40 |
| chain | 1 | literal 11 + a `28h`-capped local (`0038:2191`) | 51 |
| local, depth-one callers | 1 | `001E:0042`: cap `FFh` from the owner's `[BP+6]`; its six `PUSH CS; CALL 0013` callers all pass literal temps of 12..16 bytes | 16 |
| local, **no sound bound** | 3 | an unmodelled callee runs after the message local's address was taken (below) | 255 |

**Result: 64 of the 67 admitted entries have a sound read-time bound ≤ 51
< 255; three do not.** The checker pins both the bounded table and the
unbounded set and refuses if either changes.

### The three entries without a sound bound

Each of these sites stores its message into a `28h`- or `FFh`-capped local,
takes the local's address for that store, and then calls routines this
analysis does not model before the 16EC call. Under the sound rule a callee
reached after a local's address was taken may hold and write that pointer,
so the bound is 255. The forcing calls are named so a reviewer can see what
evidence would restore a bound:

| Entry | Owner | Message local | Store | First unmodelled calls after the store |
| --- | --- | --- | --- | --- |
| `009E:0F5D` | `0x9e:0x0f06` | `[BP-29h]` (cap `28h` from `[BP+6]`) | `0F1A` | `0F33` and `0F47` (unit `3E0` entries `006B`, `007F`); object writes `0F65..0FA9` |
| `009E:15F6` | `0x9e:0x1344` | `[BP-100h]` (cap `FFh`, literals `1301`/`130B`/`133A`) | `157D`..`15DB` | `15C5` (SYSTEM `0DC0`) between the last store and the call |
| `00BA:208E` | `RES_00BA_1FEB` | `[BP-29h]` (cap `28h` from `[BP+6]`) | `1FFF` | `2011` (`08AE:01A5`), `202B` (near `19DE`), `2037`.. (unit `3E0`), `206D`/`2078` (`08AE:00DC`) |

A reviewed read-only or non-aliasing contract for those callees (for example
SYSTEM `0DC0` as a compare leaf), or a checked non-alias entry contract,
would restore a bound; neither is claimed here. The checker's notes record
the forcing instruction for every invalidated entry.

Preservation from the read to the printer's use (the "argument-preservation"
half of NC5) is **not** established here: it needs the 16EC frame-preservation
guard proposed for NC2, which is a separate change.

## Reproduce

```sh
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh \
  python3 tools/overlay/res16ec_entry_contract.py --output resources/t24-entry-contract/entry-contract.json
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh \
  python3 -m unittest -v tools/overlay/test_res16ec_entry_contract.py
```

`--print-table` emits the measured 67-row table in the form pinned in the
checker. The unit test is discovered by `tools/run_media_checks.sh`. Its
synthetic controls assemble small bodies by hand and require: a literal temp
is bounded by its literal; a cap binds a parameter even without callers; a
concatenation adds and clamps at 255; passing the local to an unknown callee
unbounds it; a direct byte store sets the length and a covering word write
unbounds it; the join takes the maximum over paths; a body is cut at its own
return and an indirect jump is refused. The review of the first head (t3677) added
two negative controls that the first version failed and this version passes:
an indirect `MOV byte SS:[DI],FFh` after `LEA DI,[BP-29h]` must unbound the
local, and a `JMP` landing on the StrLoad call must not attribute the skipped
literal; three taint controls (a stored and reloaded stack pointer, `PUSH SS;
POP ES`, and a post-call `ES:[DI]` store) must unbound, while a DS-absolute
store below the stack base keeps the bound and one that reaches the stack
segment does not. The second review (t3683) added two more: a parameter
pointer write (`LES DI,[BP+6]; MOV byte ES:[DI],FFh`) and an indirect
`CALL word ptr [0400h]` must both unbound; a direct call after a taken
pointer unbounds the reachable locals while a never-taken local survives it.
The owner-media controls require the 63/4 census, the pinned table, the pinned
three-entry unbounded set (64 bounded, maximum 51), and that a mutated pin, a
changed unbounded set, a shortened census, a changed body byte and a lowered
stack segment are each refused.

## Limits

- Static, enumerated direct entries only. Indirect or computed calls,
  overlapping code, runtime patches, and natural arrival or frequency are
  not covered. A call site the census does not see is not admitted.
- The bound analysis trusts the ledger's body starts and Capstone's linear
  decode; the per-local model ignores overlaps between a capped copy's
  region and neighbouring locals (compiler-allocated strings do not overlap).
- Depth-one caller analysis only; a parameter reached through two levels is
  255. Callees other than the three SYSTEM string leaves have no modelled
  effect contract, which is what leaves three entries unbounded.
- The selector invariant assumes `SS` keeps the EXEPACK value, as Turbo
  Pascal runtime code does; it is checked from the bytes, not observed live.
- No accounting product, identity, readiness, membership or closure row
  changes. NC2, NC3 and NC4 remain open and are addressed separately.

Recall before analysis: Dunking Sheep memory for the 16EC closure study
(session `281c8700-5acc-438b-b93d-c061e28437eb`) and the t3667/t3668 packets
were read; the t100 census pattern for adjacent setups
([RES_03F1_02E8_IDENTITY.md](RES_03F1_02E8_IDENTITY.md)) and the
[string-leaf ABI](SYSTEM_STRING_LEAVES.md) / [StrStore ABI](SYSTEM_STRSTORE_0B25.md)
are reused.
