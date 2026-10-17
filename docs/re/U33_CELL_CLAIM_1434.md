# Unit 0x33 +1434 cell claim

`pr_u33_cell_claim_1434` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+1434..+1576` and composes its one far call to unit
`0x3C6` public entry 16 (`+0CB9`, the merged
[scan model](U3C6_0CB9_SCAN.md) with its folded entries 15, 1 and 6)
through the overlay stub `03E0:0070`. It checks one position of an
11 by 6 placement grid, records slot q's map origin, asks `+0CB9` for the
best neighbouring class and any occupant, and claims the position when
both allow it. Its only caller is the [`+1577` search](U33_PLACE_SEARCH_1577.md).
The recall attempt for this lane found nothing to reuse.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x3C6 | GAME file base `0x3073A`, stub paragraph `03E0`; entry 16 is `+0CB9`, entry 15 `+0C5F`, entry 1 `+0000`, entry 6 `+04BE` |
| Unit 0x33 | GAME file base `0xC5D1`; `+1434` is an internal prologue, not a public entry |
| Function | GAME file `0xDA05..0xDB47`; **323 bytes, 138 instructions, eight conditional branches, one far call** |
| Function SHA-256 | `b01749df6b4fe4ee1168676128f0ff9804b5dc9d3ceecbe8d642ee3085e30683` |
| Far call | `+150E` `LCALL 03E0:0070` (entry 16), segment word fixup-backed |
| Exit | `RETF 0Ch` at `+1574`; exact end `+1577` exclusive |

Every branch target lies inside the body, and `+1577` is the next
prologue. Decoding every unit-0x33 routine from its own prologue finds
exactly one aligned `PUSH CS; CALL +1434`: `+19C7` inside `+1577`. The
body reads the availability table `DS:4440`, the map byte `DS:4658`, the
slot origin table `DS:5F27/5F28`, the slot far-pointer table `DS:65B9`
and the class weights `DS:2744`. No gameplay meaning is inferred; "cell
claim" names the effect.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP, `[SP+2]`
return CS and six argument words whose low bytes are read: `[BP+6]` r,
`[BP+8]` b8, `[BP+0A]` b10, `[BP+0C]` y, `[BP+0E]` x and `[BP+10]` slot
q. `RETF 0Ch` leaves SP sixteen bytes above entry. The routine pushes BP
and reserves four bytes: result `[BP-1]`, occupant `[BP-2]` and class
`[BP-3]`.

Signed x outside 0..`0Ah` or y outside 0..5 returns AL = 0. Otherwise
the index `108h`·sign(`DS:4658`) + `42h`·r + `0Bh`·sign(y) + sign(x) is
formed with three `MUL`s around a pushed x, and a zero availability byte
`DS:[4440+index]` returns AL = 0. Otherwise `DS:[5F27+4q]` := x + 6·b10
+ 5·b8 + `16h` and `DS:[5F28+4q]` := y + 5·b8 + `0Ah`, and the body pushes
the far pointer `DS:[65B9+4q]`, k = 8 and the stack addresses of `[BP-2]`
and `[BP-3]` and far-calls entry 16. A nonzero occupant, a zero class or a
class weight `DS:[2744+4·class]` of `FFh` returns AL = 0. Otherwise the
index is recomputed, the availability byte is cleared and AL = 1. The
final flags are the deciding `CMP`'s on every refusal and `ADD DI,AX`'s
on a claim. AH, BX, CX, DX, SI, DI and ES return as the last index
computation, slot write or `+0CB9` left them.

The far call is the loaded-state transfer: the site must hold `9A`, stub
offset `0070` and the relocated segment word, and the stub there must be
`EA 0CB9` and the unit's segment, else `CODE`. The merged-in public
`+0CB9` model then runs on the same RAM and CPU words with its RETF
policy `CONTIGUOUS_PHYSICAL`, the frame reading this model also uses.
Its declared LES seam and code-alias stops report `CALLEE` with its
status in `*callee_status`, and a frame that does not come back reports
`TRANSFER`. The `+0CB9` model is a merged, source-only model that is not
strict-credited, so this body's strict credit depends on the reviewer
accepting that composition. Offsets wrap at 16 bits and physical
addresses at 1 MiB. Every operand is reread from RAM in source order.
Flag writes that a later instruction always overwrites (the `MUL`s, the
index `ADD`s before a `CMP`, the shifts) are folded. Null RAM/CPU or RAM
that is not exactly 1 MiB is rejected before mutation.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`, also run under
ASan/UBSan) uses synthetic RAM with only the far-call site and stub
bytes. It covers a range refusal with its `CMP` flags, a closed
availability byte, a claim (origin bytes, cleared availability byte,
class and occupant in the frame), an occupied neighbourhood, a weight of
`FFh`, a site that is not the loaded original (`CODE`) and the host
guards.

The original-byte oracle passes **213 returning cases**, each
comparing the entire 1 MiB RAM and all 14 CPU words with no flag mask.
The image holds the original unit 0x33 code, the three unit 0x3C6 spans
`+0000..+0067`, `+04BE..+052A` and `+0C5F..+0E10`, and the loaded-state
stubs. The cases are random worlds (availability density, slot count and
classes, probe offsets, class weights, grid and occupant cells, arguments,
flags and ES), directed occupied, heavy-class and claimable
neighbourhoods, two low and three high stack offsets, SS, DS and the
grid across A20, the grid at offset wrap, a random background and
preserved DF. All 138 instruction starts and all 16 conditional outcomes
are reached.

The DOSBox-X normal-core/386 synthetic oracle passes **24
cases** for this body in the shared [`+1577`](U33_PLACE_SEARCH_1577.md)
run, comparing all 14 CPU words, a 256-byte frame window and the
availability and slot ranges with no mask.

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+1434..+1576` with unit `0x3C6` entry 16 composed |
| `source_candidates` | `src/recovered/overlay_u33_cell_claim_1434.c` |
| `signals` | Exact hash/CFG, one aligned caller, the documented globals, whole-RAM/CPU differential with full instruction and branch-outcome coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `+0CB9` is a merged, uncredited model under its CONTIGUOUS_PHYSICAL policy; loaded-state stubs only; natural saved-game reach unmeasured |

No classifier or count ledger is changed. Identity and strict credit
remain the verifying lane's decision.
