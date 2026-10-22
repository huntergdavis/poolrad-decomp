# Unit 0x86 +07D6 roll value: raw-CPU full state

`pr_u86_roll_value_07d6` translates the complete DOS v1.3 routine `GAME.OVR`
unit `0x86:+07D6..+08C2`. It composes two models on the same RAM and CPU
words:

- the merged U9E entry-8 dice chain (`+0DE3`), at five far calls;
- the merged raw-CPU [`RES_00BA_26C2`](RES_26C2_U86_0729.md) (PR #920), at
  one far call.

It returns a 16-bit value in AX. No gameplay meaning is inferred, and "roll
value" names the effect. The VP approved this target on 2026-10-17, and
schema 7 shows no inline coverage, model or branch for it. The recall
attempt for this lane found nothing to reuse.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Function | GAME file `0x277EE..0x278DA` (unit `0x86` at `0x27018`); **237 bytes, 99 instructions, seven conditional branches**; `RETF 2` |
| Function SHA-256 | `db359b4e8066b0eb0d830a85866d8f402cba123c74ada2951ba21ddcc52db656` |
| Far calls | `+07E9`, `+0809`, `+0820`, `+0846` and `+085D` `LCALL 00B0:0048` (U9E entry 8, `+0DE3`); `+0880 LCALL 00BA:26C2`; all six segment words are fixup-backed |
| Composed bytes | U9E `+0DE3..+0E51` SHA-256 `9a2681e8…` (the merged chain's pin); SYSTEM `0AF8:11C0..1246`, which equals the START.EXE bytes the chain pins in the expanded image |

## Complete ABI and ordered effects

The routine is entered after the far CALL with `SS:[SP]` return IP,
`[SP+2]` return CS and `[SP+4]` a word whose low byte k is the argument.
`PUSH BP; MOV BP,SP; SUB SP,4` builds the frame. roll(n, s) means: `MOV AL,n;
PUSH AX; MOV AL,s; PUSH AX; LCALL 00B0:0048`, then `XOR AH,AH` on the
returned AL. The result word `[BP-4]` depends on k:

| k | `[BP-4]` |
| --- | --- |
| 28h | 10 · roll(1, 6) |
| 39h or 3Dh | roll(5, 4) |
| 3Bh | 10 · roll(1, 4) + 28h |
| 3Fh | 10 · roll(2, 0Ah) when `DS:49F3` is 5; otherwise 10 · (roll(1, 0Ah) + 0Ah) |
| 43h | 5A0h |
| any other | T[5] · `RES_00BA_26C2`(k) + T[4], 16-bit, with T the 16-byte DS table entry at `31FA + 16k` |

Then `[BP-2]` := `[BP-4]`, AX := `[BP-2]`, and `MOV SP,BP; POP BP; RETF 2`.

**Composition.** Each far call is read from RAM at its site (`9A`, offset,
segment).

- **Dice calls.** They require offset `0048h` and a stub whose bytes are
  `EA E3 0D` plus a segment. They then run
  `pr_u9e_entry8_fullstate_compose_0de3` there with `max_system_calls`. That
  chain models the original SYSTEM generator and seed `DS:4428`.
- **Lookup call.** It requires offset `26C2h` and runs `pr_res_00ba_26c2_cpu`.

Each call pushes CS and the next IP, relative to the entry IP, and must
return there. Otherwise the result is one of these statuses:

- **`CODE`.** A reached site or stub is not in its loaded form.
- **`CALLEE`.** The chain stopped, for example at its SYSTEM-call cap, and
  `*callee_status` holds its status.
- **`TRANSFER`.** A callee returned elsewhere.

Every operand is reread from RAM in source order. Offsets wrap at 16 bits
and physical addresses at 1 MiB. `LES` pairs and the `RETF` frame are read
physically contiguous.

**Flags.** The final flags are the path's last `CMP` (43h), `XOR AH,AH`
(39h/3Dh), `ADD` (3Bh and the table path), or `MUL DX` (28h and 3Fh).

- **On the `MUL`-final paths,** SF, ZF, AF and PF are architecturally
  undefined.
- **The model follows** the pinned DOSBox-X normal-core profile that the
  merged U3C6 `+0CB9` model uses: CF = OF = (DX ≠ 0), ZF = (AX = 0), PF
  from all 32 product bits, and SF and AF kept.
- **Unicorn differs.** It clears AF and takes PF and SF from the low word.
  For these small products the only observed difference is AF after the
  3Fh path's `ADD`.

## Validation and parity review

The focused C11 test runs under CTest with ASan/UBSan, together with the
merged chain's tests and #920's test, nine tests in all. It compiles with
`-Wconversion -Werror` and passes 1,695 checks, holding only the call-site
and stub bytes and synthetic DS data:

- k = 43h with its flags, and the table path with exact AX, DX and flags;
- range and determinism checks on every dice path over 40 seeds, plus the
  seed step;
- `CALLEE` with the chain's `LIMIT` at a zero SYSTEM-call cap;
- `CODE` at a broken stub;
- the argument guards.

The Unicorn original-byte differential places the hash-checked expanded
START.EXE image at load segment 0, so the original SYSTEM generator and
`RES_00BA_26C2` run live. It patches the `00B0:0048` stub to its loaded
form and loads units 0x9E and 0x86. It passes **480 cases**, each comparing
the entire 1 MiB RAM and all 14 CPU words, and reaches 99/99 instruction
starts and 14/14 conditional outcomes. The cases cover:

- 24 per dice and constant k;
- every other k on the table path, with all four classes and the
  `DS:6DBF` flag off and on;
- 80 random worlds;
- four stack offsets;
- a random background and preserved DF/IF.

In 77 `MUL`-final cases, only SF, PF and AF are masked, and only in this
Unicorn comparison.

The DOSBox-X normal-core/386 synthetic oracle passes **120 cases**, 45 of
them `MUL`-final, with **no mask**. That confirms the pinned profile,
including AF kept from the preceding `ADD`. Each case compares all 14 CPU
words, 64 stack bytes, the table entry, `DS:49F3`, the seed, `DS:6DBF` and
the record bytes. At start-up the COM copies four things:

- SYSTEM to absolute `0AF8:11C0`, which is COM offset `4000h` at segment
  `0814`, kept free;
- the U9E span to `8000:0DE3`;
- a loaded stub to `7F00:0048`;
- `RES_00BA_26C2` to `9000:26C2`.

It also copies unit 0x86 to paragraph `8800`, with all six fixup-backed
segment words relocated loader-style. The result SHA-256 is
`731093ad19b9afe3a0dd7dd521a20e6f256d1467ffae16a9db69358fca5c2af6` with
COM segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R 'u86_roll_value_07d6|res26c2_u86_0729|u9e_entry8' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u86_roll_value_07d6_audit.py --unicorn --native --output resources/u86-07d6-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x86` `+07D6..+08C2`, with U9E `+0DE3` (five sites) and `RES_00BA_26C2` composed |
| `source_candidates` | `src/recovered/overlay_u86_roll_value_07d6.c` |
| `signals` | Exact hash/CFG, fixup-backed far calls, whole-RAM/CPU differential with SYSTEM, the dice chain and the resident callee live and full instruction and branch-outcome coverage, unmasked native execution including the `MUL`-final paths, independent C test |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract and the pinned `MUL` flag profile |
| `review_notes` | `MUL`'s undefined SF/PF/AF follow the pinned DOSBox-X profile; Unicorn masks only those bits on the two `MUL`-final paths. Identity is `game_evidenced` per the queue. Natural saved-game reach is unmeasured. |

No classifier or count ledger is changed. Identity and strict credit remain
the verifying lane's decision.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0086_07D6 --require-current-main
```
