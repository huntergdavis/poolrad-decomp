# Unit 0x38 entry 41: conditional effect update at +0x12B2

## Provenance and ownership

This is PM's selected disjoint candidate after +0x1276 and +0x11F9. The
target-specific `deja "poolrad U38 +12B2 entry41 calls +0C06 effect strict C
parity"` query timed out after 20 seconds without a session result.
The caller graph reuses [the U38 effect-table installer](U38_EFFECTS_INSTALL_31A2.md),
[the dispatcher decode](OVERLAY_UNIT38_EFFECT_ENTRIES.md), and
[the merged +0C06 leaf's evidence](FUNCTION_LEDGER.md). The routine bytes,
fixup, branch paths, and parity below were independently remeasured.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

U38 public entry 41 is the complete 73-byte code body `[+0x12B2,+0x12FB)`,
`GAME.OVR` file `[0xFF0D,0xFF56)`, SHA-256
`fdc58872484dc92452c0eb042ec51727f3543e989c6fed48c30876899ddc061b`.
All 28 instructions decode contiguously. The wrapper has no loader fixup;
its sole call is the near call at `+0x12E8` to U38 `+0x0C06`. That verified
callee is `[+0x0C06,+0x0C15)`, 15 original bytes, SHA-256
`435bd9fac807d24f064b6bf223817b188e737a5b6ddb9a8b37d5635842a52fe8`.

**Factor 1 holds (A2), by the reviewer's measurement (par-e); I reproduced
only the installer bytes below.** Unit 38 `entries[41] = 12B2` (unique); entry 41's stub is `0041:00ED`
(`0x20 + 5*41 = 0xED`). The strict-credited installer `OVL_0038_31A2` stores that
stub at +3415..+3421 into DS:68EC/68EE, effect-table item `(68EC-6828)/4 = 49`, with
the segment word relocated at +3419. My byte check: GAME.OVR file `0x12070`
(unit base `0xEC5B` + `0x3415`) is `[scrubbed: span=U38_EFFECT_MATCH_12B2-1 sha256=c67f912ae8a5d0f2e3972847ca64ab9adf9e02acd0e50dd52e63e8ddcbc765c1 bytes=12 offset=UNRESOLVED]68`, and
that string occurs once. `OVL_0038_31A2` is in both credit CSVs, the same footing
that credited `OVL_0038_3134` and `OVL_0038_2E08`.

**Factor 2 holds.** The body tests record B `+3` bit `10h` against record A `+10Eh`,
near-calls the strict `+0C06` (which is the whole composition), and otherwise
decrements DS:6822/6816. DS:6822 is documented as an effect counter
(`OVERLAY_UNIT38_EFFECT_ENTRIES.md`), DS:6816 is in the helper cluster
(`U38_HELPER_CLUSTER_CPU.md`), and `+10Eh` is in the resident character docs
(`RESIDENT_CHARACTER_FIELD_ZERO.md`, `RESIDENT_PARTY_COUNT.md`, which leave its
gameplay meaning unverified).

**One limit, stated plainly:** the installer footing is what is measured. No
item-49 dispatch *execution* was observed, which is the same distinction that keeps
the 3134 entry honest. The earlier reason for withholding credit (dispatch
unmeasured) is superseded by A2 in the two-factor standard
([REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#identity-standard-two-factor-adopted-2026-10-20)).

**Status.** The row is in neither credit CSV and has no manifest declaration. Per
VP i19766 it is creditable on this evidence; the acceptance is the reviewer's to
open. This PR is the author's evidence and does not credit it.

## Complete behavior

The wrapper reads byte `+3` of far pointer B at `[BP+8,+0xA]`, isolates bit
four, and stores the zero-or-one result in its SS local byte. It reads byte
`+0x10E` of far pointer A at `[BP+0xC,+0xE]`. If equal, it pushes all five
original arguments and CS, then near-calls the verified U38 +0C06 body.
That body increments DS bytes `[0x6816]` and `[0x6822]` and returns `RETF 10`
to the wrapper's far-compatible frame. If unequal, the wrapper decrements
DS `[0x6822]` then `[0x6816]`. Both paths exit with `RETF 10` and exact
register, stack, and defined flag state. The C model composes the callee,
including its push/pop BP and far return, rather than substituting only
its two state writes.

## Verification and limits

`tests/recovered/check_overlay_u38_effect_match_12b2.py` checks the pinned
media hashes, entry indexes, exact extents and original code hashes, absence
of wrapper fixups, and the sole near-call site. Twelve cases execute both
original byte bodies in Unicorn and compare all 1 MiB RAM and 14 CPU fields
with C. They cover both equal and unequal branches, increment/decrement
overflow and zero, DS/SS aliasing, and an odd near-boundary stack frame.

Native corroboration is the 48-case suite in the Exhaustive re-audit below.
Focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest passes.

The native frames are synthetic direct entries, not item-49 dispatches or
reached gameplay. This branch makes no strict identity or count claim.

## Completeness pilot: no readiness gain measured

Author measurement on main `926f3b5d57ca48deb9734ce98720a16b44f2b9bc`:
**full executable body extent, with a 12-case harness covering 28/28 wrapper
instruction starts, 6/6 composed +0C06 starts, and 2/2 feasible conditional
outcomes.** That pilot is historical: the Exhaustive re-audit below sweeps every `B[+3]` x `A[+10Eh]` pair exhaustively and adds edge and sampled cases for counters, flags, registers and aliases; see *What is exhaustive and what is not*. The 12-case default run is unchanged.
The cases compare all 1,048,576 RAM bytes and 14 CPU fields each. This is a measured coverage statement, not new identity
or strict-C acceptance; reviewer acceptance remains separate from the author claim.

The original `GAME.OVR` slice is `[65293,65366)` (`[0xFF0D,0xFF56)`),
U38 `[+12B2,+12FB)`, 73 bytes / 28 instructions, with the body hash above.
The final three-byte `RETF 10` begins at +12F8; the next prologue and public
entry begin exactly at +12FB. The excluded tail is **zero bytes** (`[12FB,12FB)`),
so the ledger needs no extent repair. The composed +0C06 body is 15 bytes /
6 instructions. Both outcomes at +12D5 were observed: +12D7 and +12ED.

The coverage observation is reproducible without changing the cases or comparisons:

```sh
python3 tests/recovered/check_overlay_u38_effect_match_12b2.py --coverage
python3 tests/recovered/native_overlay_u38_effect_match_12b2.py
```

These commands require the ignored, hash-pinned media and the checker's existing
Capstone/Unicorn dependencies; the second additionally needs DOSBox-X and binutils.
The new `--coverage` output records starts/outcomes; it changes no parity masks
or expected results. The current native run is described in the Exhaustive re-audit.

Counterfactual: adding the three-field row `OVL_0038_12B2,full,docs/re/U38_EFFECT_MATCH_12B2.md`
to `partial_callee_models.csv` leaves **460 candidates / 58 composable** and the
entire ready-ID set unchanged; zero callers become ready and zero callee-readiness
records change. There are no decoded direct callers to +12B2. The installed
effect-table item 49 remains behind U9E +0000's unresolved indirect dispatch;
this declaration would not resolve that dispatch. +12B2 itself was already
composable through its sole strict +0C06 callee.

Per PM decision i19015, **no manifest row is added**. Future declarations must
first demonstrate at least one newly ready caller; a zero delta is recorded and
the declaration skipped. This pilot does not authorize bulk declarations.
The existing implementation/evidence was located through Dunking Sheep session
`01a1386e-bea7-7883-8ddd-06fd2be8db9b`; span, coverage, native result and queue delta
were remeasured against the current source and pinned media.

## Exhaustive re-audit

The checker gains an `--exhaustive` mode (the default 12-case run is unchanged).
It runs the original wrapper and the original +0C06 bytes in Unicorn and compares
the C model on all 14 CPU words and the **entire 1,048,576-byte RAM** each time. The
original is now mapped twice over one buffer, so A20-off wrap is reproduced exactly
as the C model's 20-bit mask does. The wrapper reads two data bytes and writes two
counters, so the sweeps are:

| Sweep | Cases | What varies | Equal branch taken |
| --- | ---: | --- | ---: |
| data bytes | 65,536 | **every** `B[+3]` value x **every** `A[+10Eh]` value (256 x 256) | 256 |
| counter edges | 72 | `[6816]` x `[6822]` in {00, 01, 7F, 80, FE, FF}, both branches | 36 |
| flags | 96 | each defined flag set alone and cleared, plus 64 random patterns (TF excluded) | 45 |
| registers | 64 | AX, BX, CX, DX, SI, DI, ES, BP random | 40 |
| aliases / wraps | 34 | A or B aliasing each counter, each other, the wrapper code, the return and argument frames; 16-bit offset wrap; A20 wrap; DS = SS | 14 |
| original run | 12 | the earlier fixed cases | -- |

**65,814 cases, zero mismatches.** Coverage over them: wrapper **28/28 starts**,
composed +0C06 **6/6 starts**, **2/2 conditional outcomes**, no missing start or
outcome. The equal branch (call into +0C06) is taken in exactly 256 of the 65,536
data-byte cases, one matching byte per value, as the code predicts.

What is exhaustive and what is not. The branch depends only on the two data bytes,
so that part is exhaustive. The counter update is an independent INC or DEC of a
byte and the flags after it depend on that byte and the incoming CF, so the counters
and flags are covered by their edge values and sweeps, not by all 2^16 counter pairs
or all 2^16 flag words; registers and pointer values are sampled. That is a
coverage statement, not a proof over every machine state.

### Exclusions, each with the state that would make it feasible

1. **Pointer or counter bytes in the video aperture `A0000..AFFFF`** (65,536 physical
   bytes). Feasible if far pointer A or B pointed into video memory, or DS/SS/CS were
   such that a counter or stack access landed there (DS values `997F..A97E` put
   `DS:6816` there; 4,096 segment values, similar for `DS:6822`). Excluded because this
   model is flat-RAM and the aperture is a device whose reads load latches; the wrapper
   draws nothing, so no device oracle is attached.
2. **Trap flag and interrupts**: TF set raises a single-step trap; no interrupt, fault
   or DMA is modelled. Every other flag pattern in the sweep is compared exactly.
3. **Self-modification**: a counter, the local byte or a write path aliasing the
   executed code (the 73-byte wrapper or the 15-byte callee). The wrapper only
   reads through A and B; it writes the two counters, its own local byte and the stack
   words it pushes (arguments, the far frame and the callee's saved BP).
4. **Frames that wrap**: the return frame and arguments are assumed not to cross
   `SS:FFFF`. **Arbitrary-PC resume** is not provided.

### Native, ownership and registration

Native corroboration is a **48-case DOSBox-X suite** (it replaced an earlier
four-frame run, which is not relied on). **The native
harness is the par-b lane's contribution**, offered on branch `claude/par-b-12b2` and
taken here unchanged (cherry-picked with its author and message); I reproduced
it on this host before relying on it. Cases: 24 `B[+3]` x `A[+10Eh]` pairs over both
branches, 12 counter-edge cases (`[6816]` x `[6822]` in {00,01,7F,80,FE,FF}) and 12 flag
cases, with six initial-flag profiles (`0002`, `00D7`, `08D7`, `0402`, `0893`, `0003`).
Every case runs the original wrapper and the original +0C06 bytes as a far call with a
synthetic return frame, with interrupts off from the fill until the record is taken.
Compared per case: all 14 CPU words, and **three whole regions refilled with `5A` before
each case** (`2000..2FFF`, `6000..6FFF`, `E800..EFFF`; 10,240 bytes) against the C model,
**with zero masks**. Result: **48/48 equal** (18 take the equal branch);
RESULT.BIN SHA-256 `2896dac6efb60b539fe84ed87ea2800751fe7fb7a7ffbca1523963f90dbe523b`
(par-b's value and my reproduction agree).

Native scope is partial: the other **1,038,336 backing bytes are not compared natively**
(the harness's own code and buffers live outside the three regions); the full 1 MiB
comparison is the Unicorn sweeps above. The native run used this host's **DOSBox-X
2026.01.02 (`/usr/bin/dosbox-x`, SHA-256 `99d74601...2a0df3`), not the pinned
`5a9c7c53` build**; the harness records that in its report (`pinned_build_used: false`).
Frames are synthetic direct entries, not item-49 dispatches or reached gameplay.

Ownership is unchanged by this change: see the identity paragraph at the top
(Factor 1 by A2 through the credited +31A2 installer, Factor 2 on the
argument-dereference rule). No manifest row is added here; this PR is the author's evidence and credits nothing. The checker already declares
`CHECKER_ROWS = ("OVL_0038_12B2",)` and a media-free `--compile-only`; neither changed.

## Reviewer acceptance record (added by the review lane, not the author)

Reproduced by par-e at author head `a0fbce1a` (evidence), with the ownership paragraph checked at the merged head
`ce8e785c`. The credit is applied on main `a534f5d1`, 2026-10-20.

- **Unicorn, exhaustive:** 65,814 cases, all passing (65,536 `B[+3]` x `A[+10Eh]` values, 72 counter edges, 96 flag,
  64 register and 34 alias cases, and the 12 originals). The equal branch is taken in 256 of the 65,536 data cases.
  Each case compares CPU14 + 1,048,576 B.
- **Coverage:** wrapper 28/28 starts, composed `+0C06` 6/6, 2/2 outcomes, nothing missing.
- **Native:** 48/48 (18 equal-branch) over 10,240 B, flag masks 0, RESULT.BIN `2896dac6…`, declared substitution
  (`/usr/bin/dosbox-x` `99d74601…`, `pinned_build_used: false`).
- **Exclusion arithmetic:** DS `997F..A97E` (4,096 values) puts `DS:6816` in `A0000..AFFFF`, as stated.
- **Identity (A2) by reviewer measurement:** the installer bytes `[scrubbed: span=U38_EFFECT_MATCH_12B2-2 sha256=c67f912ae8a5d0f2e3972847ca64ab9adf9e02acd0e50dd52e63e8ddcbc765c1 bytes=12 offset=UNRESOLVED]68` sit at
  GAME.OVR `0x12070` and occur once. The credit rests on the installer footing; no item-49 dispatch execution was
  observed (stated above).
- "This branch makes no strict identity or count claim" in *Verification and limits* refers to the author branch,
  which does not self-credit. The credit is this acceptance's.
