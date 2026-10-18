# Unit 0x38 +2915 roll gate and entries 89/90/91: raw-CPU full state

`overlay_u38_roll_gate_2915_cpu.c` holds raw-CPU, full-state models of four
complete `GAME.OVR` unit `0x38` routines. They became composable once #912
(the U38 `+0000` helper) and #920 (`RES_00BA_26C2`) merged. They compose,
on the same RAM and CPU words:

- the raw-CPU `RES_00BA_26C2`;
- the merged U9E entry-8 dice chain, through the loaded stub `00B0:0048`;
- the merged raw-CPU helper `+0000`.

The VP approved this cluster on 2026-10-17. Schema 7 shows no inline
coverage, model or branch for any of the four. No gameplay meaning is
inferred, and the recall attempt for this lane found nothing to reuse.

## Exact originals

GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; START.EXE is
65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. Unit `0x38`
is at GAME file `0x0EC5B`.

| Routine | GAME file | Bytes / insns | Exit | SHA-256 |
| --- | --- | --- | --- | --- |
| `+2915` (not public) | `0x11570..0x115D0` | 97 / 42 | `RETF 2` | `cb60388e99335ac9a8e38064055e5f9553f4f9929ae3909387213abea80eb48f` |
| entry 89 `+2976` | `0x115D1..0x115DE` | 14 / 8 | `RETF 0Ah` | `76360e934858c188426d363bac708c9912b80e76a17f7c7e242a9cd98c9fef53` |
| entry 90 `+2984` | `0x115DF..0x115EC` | 14 / 8 | `RETF 0Ah` | `679c7643ef79fb918e59ff8562a0dab28dd2928dffcc2294604dae0a58582b60` |
| entry 91 `+2992` | `0x115ED..0x11610` | 36 / 19 | `RETF 0Ah` | `c33cf31af11f33e75a0b77d2f4f8d35f86c03e9d7a999332b316d27f5e8a57c9` |

The calls are as follows; all three far-call segment words are
fixup-backed:

- `+291F LCALL 00BA:26C2`;
- `+295F` and `+299B LCALL 00B0:0048`;
- `+296D`, `+29A8` and `+29AF PUSH CS; CALL +0000`;
- `+297D` and `+298B PUSH CS; CALL +2915`.

## Complete ABIs and ordered effects

roll(n, s) means `MOV AL,n; PUSH AX; MOV AL,s; PUSH AX; LCALL 00B0:0048`.
helper(k) means `MOV AL,k; PUSH AX; PUSH CS; CALL +0000`.

**`+2915`.** It is entered after `PUSH AX; PUSH CS; CALL +2915`, so `[SP]` is
IP, `[SP+2]` is CS and `[SP+4]` is a word whose low byte is b.

1. `PUSH BP; MOV BP,SP; SUB SP,2`.
2. r := `RES_00BA_26C2`(`DS:681B`), stored in `[BP-1]`. SI := 11 − r and
   DX := 5·SI.
3. `[BP-2]` := the low byte of b − DX.
4. If `DS:6817` is nonzero, or `DS:6819` bit 3 is set, the routine runs
   roll(1, 64h). If AL is not above `[BP-2]`, it then runs helper(0).
5. `RETF 2`.

The final flags are those of `OR AL,AL` when the roll is skipped, of
`CMP AL,[BP-2]`, or of the helper.

**Entries 89 and 90** (`+2976`, `+2984`). Each pushes AL = 32h (or 0Fh),
then CS, and near-calls `+2915`. Then `POP BP; RETF 0Ah`.

**Entry 91** (`+2992`). roll(1, 64h). Unless AL is above 5Ah, it runs
helper(35h) and then helper(0Bh). Then `POP BP; RETF 0Ah`.

**Composition.** Each far call is read from RAM: `9A`, offset, segment, plus
the loaded `EA E3 0D` stub for the dice. Each call pushes CS and the next
IP, relative to the entry IP, and every callee must return there.
Otherwise the result is one of these statuses:

- **`CODE`.** A reached site or stub is not in its loaded form.
- **`CALLEE`.** The dice chain stopped, for example at its SYSTEM-call cap.
- **`TRANSFER`.** A callee returned elsewhere.

The `SUB`/`SHL`/`ADD`/`XOR` flag writes in `+2915` are overwritten by the
`CMP [6817],0` before any read, so they are folded. Every operand is reread
from RAM in source order. Offsets wrap at 16 bits and physical addresses at
1 MiB. `LES` pairs and the `RETF` frame are read physically contiguous.

## Validation and parity review

**Focused C test.** `test_overlay_u38_roll_gate_2915_cpu.c` runs under CTest
with ASan/UBSan, and compiles with `-Wconversion -Werror`. It passes 323
checks:

- the skip path with exact `[BP-2]`, SI, DX and `OR` flags;
- forced always and never roll outcomes over 19 seeds;
- entries 89 and 90 with their pushed key words;
- both outcomes of entry 91 over 64 seeds, with CF and ZF clear on the skip;
- `CALLEE` at a zero SYSTEM-call cap;
- `CODE` at a broken stub;
- the argument guards.

**Unicorn differential.** `tools/overlay/u38_roll_gate_2915_audit.py`
places the hash-checked expanded START.EXE image at load segment 0, so
SYSTEM and `RES_00BA_26C2` run from their original bytes. It patches the
stub to its loaded form and loads units 0x9E and 0x38. Every case compares
the entire 1 MiB RAM and all 14 CPU words with no mask:

| Routine | Returning cases | Instruction starts | Conditional outcomes |
| --- | --- | --- | --- |
| `+2915` | 214 | 42/42 | 6/6 |
| `+2976` | 214 | 8/8 | 0/0 |
| `+2984` | 214 | 8/8 | 0/0 |
| `+2992` | 214 | 19/19 | 2/2 |

Per routine, the cases cover:

- 160 random worlds;
- 48 directed worlds for both gate tests and both roll outcomes;
- four stack offsets;
- a random background and preserved DF/IF.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **120
cases**, 30 per routine. Each case compares all 14 CPU words, 64 stack bytes
and every DS byte the case wrote, with no mask. At start-up the COM copies:

- SYSTEM to absolute `0AF8:11C0`;
- the U9E span to `8000:0DE3`;
- a loaded stub to `7F00:0048`;
- `RES_00BA_26C2` to `9000:26C2`;
- unit 0x38 to paragraph `8800`, with the three fixup-backed segment words
  relocated loader-style.

The result SHA-256 is
`03b63aedc86542de72348bc3692bbd72b3bd8f0c1dfa89a86d273b9a93e78347` with
COM segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R 'u38_roll_gate_2915|u38_helper_cluster|res26c2' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u38_roll_gate_2915_audit.py --unicorn --native --output resources/u38r-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x38` `+2915`, `+2976`, `+2984` and `+2992`, composing `RES_00BA_26C2`, U9E `+0DE3` and the helper `+0000` |
| `source_candidates` | `src/recovered/overlay_u38_roll_gate_2915_cpu.c` |
| `signals` | Exact hashes/CFGs, fixup-backed far calls, whole-RAM/CPU differential with SYSTEM, the dice chain and the resident callee live and full instruction and branch-outcome coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Identity follows the queue: `game_evidenced` for `+2915` and `+2992`, `game_or_other_library_unresolved` for entries 89 and 90. Natural saved-game reach is unmeasured. |

No classifier or count ledger is changed. Identity and strict credit remain
the verifying lane's decision.
