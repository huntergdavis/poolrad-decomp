# Unit 0x38 entry 106 +2D38 record roll

`pr_u38_record_roll_2d38` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x38` public entry 106 (`+2D38..+2D87`) and composes
its one external call, unit `0x9E` public entry 9 (`+0E2E`, the strict
[roll latch](U9E_ENTRY9_ROLL_LATCH_0E2E.md)), reached through the overlay
stub `00B0:004D`; entry 9 in turn runs the strict entry-8 chain. The
recall attempt, branch search and reused work are recorded in
[U2F_PARTY_PICK_0131.md](U2F_PARTY_PICK_0131.md), whose audit covers all
three callers.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x9E entries 8 and 9 | `+0DE3..+0E51`, 111 bytes, SHA-256 `9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b`; stub paragraph `00B0`, offsets `48` and `4D` |
| SYSTEM generator | START.EXE `0xBE36..0xBEBC` (135 bytes): the `+11C0` wrapper (`e4337c9e…`), the `+120F` seed step (`f5b67b3e…`) and the multiplier word `8405` at `+1245`, as the merged [entry-8 roll evidence](U9E_ENTRY8_ROLL.md) pins them |
| Unit 0x38 | GAME file base `0xEC5B`, stub segment `0x0041`; public entry 106, stub offset `0232` |
| Function | GAME file `0x11993..0x119E2`; **80 bytes, 29 instructions, three conditional branches, one far call** |
| Function SHA-256 | `a1a5e1f7d58f39586ae3fcb4951dfe1eac7a7513a9afab2c9ad06153bc61394b` |
| Far call | `+2D6A` `LCALL 00B0:004D` (entry 9), segment word fixup-backed |
| Exit | `RETF 0Ah` at `+2D85`; exact end `+2D88` exclusive |

Every branch target lies inside the 80-byte body. The audit finds no
direct fixup-backed far call to the entry's stub: like the other unit-0x38
effect entries, it is reached through the procedure-pointer table that
the strict [effect-table installer](U38_EFFECTS_INSTALL_31A2.md) fills
(ledger procedure-pointer evidence `1`), which is the ownership the
reviewed unit-0x38 entries rely on. The body reads the active record
pointer `DS:5D92/5D94` used across the reviewed party evidence, that
record's far pointer `+CC/+CE`, and the latch bytes `6818`, `6819` and
`681C` that the entry-9 note documents as the `DS:681C` latch and its
consumers. No gameplay meaning is inferred for the `56h` byte or the sum;
"record roll" names the operation.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP, `[SP+2]`
return CS and five caller words it never reads; `RETF 0Ah` leaves SP
fourteen bytes above entry. The routine pushes BP and reserves four local
bytes for the far pointer `[BP-4/-2]`.

It loads `DS:5D92/5D94` into ES:DI, copies that record's `+CC/+CE` words
into the locals, and ORs them; when nonzero it reloads the pointer and
compares byte `+2E` with `56h`; when equal it pushes count 3 and range 8
as live AX words, calls entry 9 and stores AL at `DS:6818`. Entry 9 itself
stores the count byte at `DS:681C`, pushes count and range, and reaches
entry 8 with `PUSH CS; CALL`, so the composed callback leaves SP on that
frame for the merged entry-9 model's own RETF 4 accounting. Then AL :=
`DS:6819 AND 9`, `OR AL,AL`; when nonzero AL := `DS:681C` and the byte at
`DS:6818` gains it with `ADD`'s flags. The final flags come from that
`ADD` when it runs, otherwise from `OR AL,AL`; the pointer `OR` never
reaches the return (see the correction below). AX, BX, CX, DX, SI, ES and DI
return as the chain and the last LES left them.

Each far call is modelled as the loaded-state transfer: the relocated
segment word is read from the call site in RAM, the five-byte far jump at
that paragraph's stub offset must name the original entry offset, CS:IP
move to the jump target, and the merged chain runs on the same RAM and CPU
words: `pr_u9e_entry8_fullstate_compose_0de3` (entry 8, its loop, its
SYSTEM call at `0AF8:11C0`, the seed step at `0AF8:120F` reading and
writing the DS dword `4428`, and its RETF 4) with a host cap of 256 call
slices that a byte count can never reach. A site or stub that is not the
loaded-state original reports `CODE` before any effect; a chain stop
reports `CALLEE` with the chain's own status in `*chain_status`; a frame
that no longer returns reports `TRANSFER`. The merged chain requires entry
8 at offset `0DE3` of its segment and SYSTEM at paragraph `0AF8`, which is
where the game loads them. Offsets wrap at 16 bits; words, LES operands
and return frames are physically contiguous; physical addresses wrap at
1 MiB. Null RAM/CPU or RAM that is not exactly 1 MiB is rejected before
mutation.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`) runs the merged
chain on synthetic RAM: a `56h` record whose 3d8 lands in `6818` with the
count 3 latched and added through flag bit 3 (expected from the generator
formula), a `55h` record with no roll and preserved registers, a null
pointer with the 8-bit carry of the addition, a stub that is not a far
jump (`CODE`) and the host guards.

The original-byte oracle passes **259 returning cases**, each
comparing the entire 1 MiB RAM and all 14 CPU words with no flag mask:
present and absent pointers with five record bytes, six latch flag
patterns and two seeds each, 120 random records, latch bytes, seeds,
flags and ES values, a null pointer, the record over the latch bytes, the
record under the stack, a latch wrap, eleven stack offsets at both segment
ends, A20 wrap of the stack and DS, a random background and preserved DF.
All 29 instruction starts and all 6 conditional outcomes are reached.

The DOSBox-X normal-core/386 synthetic oracle passes **80 cases** for this
body (80 for `+0131`, 80 for `+2D38` and 60 for `+0000` in one run),
comparing all 14 CPU words, 128 frame bytes, the seed dword, the three
latch bytes `6818`, `6819` and `681C` and the 1,257-byte grid with no
mask. The COM keeps the original unit 0x9E span at `CS:0DE3`, copies the
original SYSTEM bytes to the absolute paragraph `0AF8` at start-up (COM
offset `4000` with the observed segment `0814`), holds the three original
caller spans elsewhere and relocates every far call's segment word to one
stub paragraph by the loader's arithmetic. The harness checks that its
code, fixture table and absolute SYSTEM copy stay clear of every DS global
the bodies touch (an earlier layout let the seed dword overwrite one
fixture row). Maskable interrupts are disabled from frame priming through
capture. The result SHA-256 is `2713b2d07c7a47be2771afa2e8d437b1e7a787d4991f7d3bd4148a49826151ba` with COM segment `0814`; another DOS
allocation changes the hash while retaining parity. Original bytes,
generated assembly/COM, results and logs stay under ignored `resources/`;
public fixtures are synthetic, and the public C tests run the merged chain
without any original code bytes.

```sh
cmake -S . -B resources/build-dice-callers -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-dice-callers --target poolrad_overlay_u2f_party_pick_0131_tests \
  poolrad_overlay_u38_record_roll_2d38_tests poolrad_overlay_u33_grid_roll_0000_tests
ctest --test-dir resources/build-dice-callers \
  -R 'poolrad_overlay_(u2f_party_pick_0131|u38_record_roll_2d38|u33_grid_roll_0000)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 2400s \
  python3 tools/overlay/u33_grid_roll_0000_audit.py --unicorn --native \
  --output resources/u33-0000-dice-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x38` entry 106 `+2D38..+2D87` with entries 9 and 8 and SYSTEM composed |
| `source_candidates` | `src/recovered/overlay_u38_record_roll_2d38.c` |
| `signals` | Exact hash/CFG, effect-table ownership shared with the reviewed unit-0x38 entries, the documented active-record pointer and latch bytes, whole-RAM/CPU differential with full instruction and branch coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Loaded-state stubs only; natural saved-game reach unmeasured; reached through the effect table rather than a direct far call |

No classifier or count ledger is changed. Identity and strict credit
remain the PM's review decision.

## Correction, 2026-10-21 (final flags; doc only)

An earlier version of this page said the final flags come, "when the pointer is null", from the pointer `OR` at
`+2D55`. **That was wrong.** Every path reaches `+2D72` before the return:
- the null-pointer `JE` at `+2D58` jumps there;
- the type test's `JNE` at `+2D62` jumps there;
- the matching path falls through to it after the call.

`+2D72..+2D79` is `MOV AL,[6819]; AND AL,9; OR AL,AL; JE 2D82`, so `OR AL,AL` at `+2D77` always runs, and its flags
are final unless `ADD [6818],AL` at `+2D7E` runs after it. The pointer `OR`'s flags never survive to the `RETF 0Ah`.

The C model and its differential are unaffected: the differential compares the real final flags, so the model was
already right. The credit stands, and only this prose sentence changes. Found by a free-model static re-derivation
(opencode/deepseek-v4.1-flash-free). par-d verified it against GAME.OVR (VP i20821), and par-e verified it independently
by decoding `OVL_0038_2D38` from the media.
