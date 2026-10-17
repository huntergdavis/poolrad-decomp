# Unit 0x33 +02BA stamp block

`pr_u33_stamp_block_02ba` translates the complete DOS v1.3 routine
`GAME.OVR` unit `0x33:+02BA..+0376` and composes its six near calls to the
merged [`+0221` clipped writer](U33_CLIPPED_WRITER_0221.md) on the same RAM
and CPU words. It stamps glyphs at positions relative to its caller's frame
(the static link) into the 50 by 25 grid. Its sibling is
[`+0377`](U33_STAMP_SQUARE_0377.md); both are called only from `+0810`.
The recall attempt for this lane found nothing to reuse.

## Exact original, callers and ownership

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x33 `+0221` writer | `+0221..+02B9`, 153 bytes; SHA-256 `9a97dca1163b4aabd6193cea3a81427143709390e7b4aa2dec1a2a727236cd06`; the merged `pr_u33_clipped_writer_0221` |
| Function | GAME file `0xC88B..0xC947`; **189 bytes, 86 instructions, five conditional branches, six near-call sites** |
| Function SHA-256 | `a1e4f0b3959095e040c27441d56b7213f4186eaff3233baeaae20759015a7c47` |
| Near calls | `+02E1`, `+0315`, `+0327`, `+033C`, `+035E` and `+036E` `CALL +0221`, each preceded by `PUSH CS` |
| Exit | `RETF 2` at `+0374`; exact end `+0377` exclusive |

Every branch target lies inside the body, and `+0377` is the next prologue.
Decoding every unit-0x33 routine from its own prologue finds exactly one
aligned `PUSH CS; CALL +02BA`: `+087E` inside `+0810`, which the mode
dispatcher `+1244` calls for mode 1. No gameplay meaning is inferred;
"stamp block" names the effect.

## Complete ABI and ordered effects

The API enters after the far CALL with `SS:[SP]` return IP, `[SP+2]`
return CS and `[SP+4]` the caller's frame offset L; `RETF 2` leaves SP six
bytes above entry. The routine pushes BP and reserves two bytes: row
`[BP-2]` and column `[BP-1]`.

Each stamp loads AL with the column, row and glyph, pushing AX each time so
the live AH travels in the pushed words. It then pushes L and `PUSH CS; CALL
+0221`, which writes glyph + 1 at column 5·sign(`SS:[L-2]`) + 6·sign(`SS:[L-1]`)
+ 15h + column and row 5·sign(`SS:[L-2]`) + 0Ah + row, inside 50 by 25
only, and returns with `RETF 8`.

The body stamps glyph 16h for rows 2..4 and columns 0..5. Then `MOV DI,[BP+6]`
and `CMP BYTE SS:[DI-8]` select a pattern:

- **Selector 1:** for rows 2..4, glyph 4 at column row − 1 (AX = row − 1
  after `XOR AH,AH; DEC AX`), glyph 3 at column row, and glyph 0Dh at column
  row + 1.
- **Selector 3:** glyph 8 at (row 2, column 1) and glyph 0 at (row 4,
  column 5).

The final flags are the last `CMP`'s on the selector-1 and other paths,
and +0221's `ADD` on the selector-3 path. DI holds L or +0221's last DI.
AX, BX, CX, DX, SI and ES return as +0221 or the last AL load left them.

Each near call pushes CS and the return offset relative to the entry IP
(0x02BA in the game), enters the +0221 model at ip − 99h and requires its
RETF to return to the frame, else `TRANSFER`. Every operand is reread from
RAM in source order. Offsets wrap at 16 bits, physical addresses at 1 MiB,
and the RETF frame is read physically contiguous. +0221 overwrites every
arithmetic flag before it returns, so the flag writes of the loop
`INC`s and the `XOR`/`DEC`/`INC` before a call are folded. Null RAM/CPU
or RAM that is not exactly 1 MiB is rejected before mutation.

## Validation and parity review

The focused C11 test (`-Wall -Wextra -Wpedantic -Werror`, also ASan/UBSan)
checks:

- the selector-1 block and diagonal cell by cell, with `CMP`'s final flags;
- the selector-3 corner glyphs;
- the block-only extent for other selectors;
- full clipping when the caller's column byte is 30h;
- the host guards.

The original-byte oracle passes **158 returning cases**, each comparing the
entire 1 MiB RAM and all 14 CPU words with no flag mask. The cases are:

- 120 random links (selectors, column/row bytes, grid, flags, ES and every
  entry register, including AH);
- 20 directed selector/coordinate pairs;
- link offsets that wrap below SS:0000;
- six stack offsets at both segment ends;
- SS, DS and the grid across A20, and the grid at offset wrap;
- a grid laid over the caller's frame, so the first stamp rewrites `SS:[L-2]`
  and every later position follows it;
- a random background and preserved DF.

All 86 instruction starts and all 10 conditional outcomes are reached. The
DOSBox-X normal-core/386 synthetic oracle passes **40 cases** for this body
(80 with +0377 in one run), comparing all 14 CPU words, 128 frame and link
bytes and 1,264 grid bytes with no mask. The COM holds the original unit
0x33 code at one base, so every `PUSH CS; CALL` displacement is original.
The result SHA-256 is
`7468258848b41532f91c50f52237eb49454153d2b4fbf7c408f57ec30d907293` with
COM segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
cmake -S . -B resources/build-stamps -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-stamps --target poolrad_overlay_u33_stamp_block_02ba_tests \
  poolrad_overlay_u33_stamp_square_0377_tests
ctest --test-dir resources/build-stamps -R 'poolrad_overlay_u33_stamp_' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1800s \
  python3 tools/overlay/u33_stamp_02ba_audit.py --unicorn --native --output resources/u33-02ba-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33` `+02BA..+0376` with `+0221` composed at six sites |
| `source_candidates` | `src/recovered/overlay_u33_stamp_block_02ba.c` |
| `signals` | Exact hash/CFG, one aligned caller, whole-RAM/CPU differential with full instruction and branch-outcome coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `+0221` is a merged model whose strict credit is pending; natural saved-game reach unmeasured |

No classifier or count ledger is changed. Identity and strict credit remain
the verifying lane's decision.
