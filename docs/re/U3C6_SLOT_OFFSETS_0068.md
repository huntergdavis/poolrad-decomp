# Unit 0x3C6 +0068 slot offsets

`pr_u3c6_slot_offsets_0068` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x3C6:+0068..+00E2`. For every slot from 1 to the slot
count it stores each slot's two table bytes less the two header bytes of
one far record. It is a leaf: no calls, no interrupts. The slot tables are
the ones the merged [unit 0x3C6 entries 12..15](U9E_0160.md) read. No
gameplay meaning is inferred, and "slot offsets" names the effect. The
recall attempt for this lane found nothing to reuse.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit | stub paragraph `0x3E0`, code at GAME file `0x3073A` (198,458), `0x1300` bytes |
| Function | GAME file `0x307A2..0x3081C`; **123 bytes, 51 instructions, two conditional branches, no calls** |
| Function SHA-256 | `c429a91a0c99d1b512cbf00f5ab9b7423070f03114a0f8d3de89fbd1784a3acb` |
| Branches | `+0079 JA +00DF` (no slot) and `+00DD JNE +0080` (next slot) |
| Exit | `RETF` at `+00E2`; exact end `+00E3` exclusive, the next prologue |

Both branch targets and the `+007E JMP +0083` lie inside the body. The
routine is not a public entry. Decoding every unit-0x3C6 routine from its
own prologue finds exactly one aligned `PUSH CS; CALL +0068`: `+0986`
inside `+07D4`. Capstone prints opcode `98h` as `cwde` in 16-bit mode; the
CPU executes it as `CBW`, which the model implements.

## Complete ABI and ordered effects

The API enters at `CS:IP` = unit:`+0068` after the far CALL, with `SS:[SP]`
return IP and `[SP+2]` return CS, and no arguments. `RETF` leaves SP four
bytes above entry. The routine pushes BP and reserves two frame bytes: the
count copy `[BP-2]` and the slot counter `[BP-1]`.

1. `[BP-2]` := `DS:5F2A`, the slot count n. `MOV AL,1; CMP AL,[BP-2]; JA`
   leaves at once when n is 0, with AH, DX and ES:DI unchanged.
2. Otherwise `[BP-1]` := 1, and each pass for counter s makes two stores:
   - `DS:[604A+s]` := low byte of `CBW(DS:[5F27+4s]) − CBW(G[+2])`;
   - `DS:[6092+s]` := low byte of `CBW(DS:[5F28+4s]) − CBW(G[+3])`.

   G is the far pointer `DS:6716/6718`, reloaded by `LES DI` for each
   store. Each store rereads `[BP-1]` once for the table index (`SHL DI,1`
   twice) and once for the target index.
3. `MOV AL,[BP-1]; CMP AL,[BP-2]; JNE +0080` ends the pass. `+0080`
   increments `[BP-1]` and starts the next pass at `+0083`.
4. `MOV SP,BP; POP BP; RETF`.

After the last pass AL is the counter and AH is 0. DI is the counter, and
ES is G's segment. DX holds `CBW(G[+3])`'s high byte and the last stored
byte. The final flags are the last `CMP`'s, or the entry `CMP 1,n` when n
is 0. The `SUB`, `XOR`, `SHL` and `INC` flag writes are dead. Every pass
ends in a `CMP` that rewrites CF, PF, AF, ZF, SF and OF first, so the model
folds them.

Every operand is reread from RAM in source order. Offsets wrap at 16 bits
and physical addresses at 1 MiB. `LES` and the `RETF` frame are read
physically contiguous. The two aliasing classes are reproduced, not
excluded:

- **Tables over targets.** From slot 49h the table bytes `DS:5F27+4s` and
  `5F28+4s` reach `DS:604B` and later stores, so later passes read what
  earlier passes wrote.
- **Frame over targets.** When `SS:BP-2` or `SS:BP-1` is physically a
  target byte, a store rewrites the count or the counter. The loop then
  ends early, runs extra passes or never ends.

`max_loopbacks` caps the taken `JNE +00DD → +0080` loop-backs that the
model follows. On the next one it stops before the `INC` and returns
`LIMIT`. CS is then unchanged, IP is the entry IP plus 18h, SP is BP − 2,
and every earlier effect is kept. That is the original's exact state on
reaching `+0080` that time. Without frame aliasing a call makes n passes
and n − 1 ≤ 254 loop-backs, so a cap of 254 or more never stops it. Null
RAM/CPU, or RAM that is not exactly 1 MiB, is rejected before mutation.

## Validation and parity review

The focused C11 test runs under CTest with ASan/UBSan, and compiles with
`-Wall -Wextra -Wpedantic -Wconversion -Werror`. It passes 49 checks:

- n = 0, with only the frame bytes changed and the entry `CMP`'s flags;
- n = 3 with explicit table bytes, the six stored values, DX = `FF01h`, and
  exactly two loop-backs allowed;
- n = FFh against an independent in-order reference, with the overlap;
- G at offset `FFFEh`, so its header bytes wrap to `GS:0000`/`0001`;
- the host cap after five passes, stopping at `+0080` with `CMP 5,30h`'s
  flags (`0283h`);
- an aliased frame whose rewritten count and counter still return;
- an aliased frame that never returns, stopping at the 1001st taken `JNE`
  with counter 1Dh;
- the argument guards.

The Unicorn original-byte differential runs the original unit code and the
C model on copies of one 1 MiB image. Every case compares the entire RAM
and all 14 CPU words with no flag mask. It passes **188 returning cases**:

- eleven directed counts (0, 1, 2, 7, 48h, 49h, 4Ah, 80h, C0h, FEh, FFh);
- 160 random worlds, varying the count, tables, header bytes, flags, ES
  and every entry register;
- G laid over the stored bytes and over the slot table, G across A20 and
  at offset wrap;
- six stack offsets at both segment ends, SS and DS across A20;
- a random background and preserved DF;
- two frame-aliasing worlds that return;
- a cap exactly equal to the loop-backs needed.

It also passes **5 host-cap stops**. Each matches the original stopped
before executing its matching arrival at `+0080`. One is the endless
aliasing world, after 600 loop-backs. The other four are caps of 0, 1, 7
and 2Eh on 30h slots.

The original's loop-back count is pinned for six cases: n = 0, 1 and FFh,
the exact cap, and both returning aliasing worlds. The pins show that the
rewrites took effect. All 51 instruction starts and all 4 conditional
outcomes are reached in returning runs.

The DOSBox-X normal-core/386 synthetic oracle passes **36 cases**. It
compares all 14 CPU words, the 64 frame bytes and the 1,040-byte DS area
`5F20..632F`, which holds every table byte read and every byte stored for
counts up to FFh, with no mask. The COM holds the original unit 0x3C6 code
at offset 0 of its own paragraph, with IRQs masked. The result SHA-256 is
`a772acb40cd4110822ddfe2f7bafd302d49c03097671a12b27ab92b499e4cc75` with
COM segment `0814`. Original bytes and generated images stay under
ignored `resources/`.

```sh
cmake -S . -B resources/build-0068 -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all'
cmake --build resources/build-0068 --target poolrad_overlay_u3c6_slot_offsets_0068_tests
ctest --test-dir resources/build-0068 -R poolrad_overlay_u3c6_slot_offsets_0068 --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1800s \
  python3 tools/overlay/u3c6_slot_offsets_0068_audit.py --unicorn --native \
  --output resources/u3c6-0068-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x3C6` `+0068..+00E2`, a leaf |
| `source_candidates` | `src/recovered/overlay_u3c6_slot_offsets_0068.c` |
| `signals` | Exact hash/CFG, one aligned caller, whole-RAM/CPU differential with full instruction and branch-outcome coverage, matched host-cap stops, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Frame-aliasing worlds can make the original loop forever (host `LIMIT` cap, compared at the stop); natural saved-game reach unmeasured |

No classifier or count ledger is changed. Identity and strict credit remain
the verifying lane's decision.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_03C6_0068 --require-current-main
```
