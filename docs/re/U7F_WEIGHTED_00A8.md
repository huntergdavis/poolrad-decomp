# U7F +00A8: raw-CPU weighted LongInt sum composing SYSTEM Move, multiply and divide

`pr_u7f_weighted_00a8_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x7F` routine `+00A8`, entry 12. The VP assigned it to this
lane on 2026-10-17 (i17655), after Astra confirmed no ownership. In the
census on main `11c22a0` its callees are:

- SYSTEM Move `0AF8:0A3F` (PR #936);
- the LongInt divide `0AF8:0A76` (PR #947);
- the LongInt multiply `0AF8:0A5B`, whose model is PR #955, the base of this
  PR.

The routine has no inline coverage, model, branch or main file. The
classifier lists it as `helper_candidate`, grade A. Identity and strict
credit are the verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; expanded image at load segment 0 places SYSTEM at `0AF8` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U7F `+00A8` | GAME file `0x24D31..0x24DB8`; **136 bytes, 51 instructions, one conditional branch**; `RETF 4`; SHA-256 `5e0a9a9ed7fed15b6f13500048537d395828dd67c3600c3b2e3534fc6eda00ea` |
| Far calls | `+00BC` `LCALL 0AF8:0A3F`, `+00F4` `LCALL 0AF8:0A5B`, `+0119` `LCALL 0AF8:0A76`; each segment word is fixup-backed |
| Callers | entry 12 through the unit's stub, from U1E `+00BF` (site `+019B`), U23 `+0361` (site `+03FF`) and U77 `+1F52` (site `+200E`) |

## ABI and ordered effects

The routine is entered after a far call with `SS:[SP]` the return IP,
`[SP+2]` the return CS and `[SP+4]/[SP+6]` a far pointer P. It returns with
`RETF 4`. After `PUSH BP; MOV BP,SP; SUB SP,26h`:

1. `Move(P^, v, 28)` copies seven LongInts into the local array v at
   `BP-20h`.
2. The LongInt sum at `BP-24h` := 0 and j := 0 (a byte at `BP-25h`).
3. For j = 0..4: `CBW` and two `SHL DI,1` form 4j. BX:CX := v[j], and
   DX:AX := w[j], the LongInt at `DS:0D24h + 4j`. The multiply leaves the
   low 32 bits of the product, which is added to the sum with `ADD`/`ADC`
   to memory. `CMP BYTE [BP-25h],4; JNE` closes the loop.
4. DX:AX := sum + 100 (`ADD AX,64h; ADC DX,0`) and BX:CX := `DS:[0D30h]`.
   Note that `0D30h` is w[3]'s address. The signed, truncating divide
   leaves the quotient in DX:AX.
5. The quotient is stored at `BP-4`, reloaded, and `MOV SP,BP; POP BP;
   RETF 4`.

The result is (Σ w[j]·v[j] + 100) div w[3], in wrapping 32-bit
arithmetic. BX:CX end as the remainder, SI:DI as |w[3]| and ES as SS (from
Move). Every operand is reread from RAM in source order, so w, v and the
divisor may alias the frame. The audit drives the weights under the stack,
where the call frames rewrite w[3] and w[4] mid-loop. j and the sum have
no writer but this routine, so the loop always runs five times and the
model takes no host cap.

**Stops.**

- `CODE`: a reached `LCALL` whose opcode or entry word is not
  `9A 3F 0A`, `9A 5B 0A` or `9A 76 0A` stops before it, with IP at the
  site. The sites move with the entry IP.
- `CALLEE`: w[3] = 0 makes the divide jump to its runtime-error target
  `00E2h`. The CPU is left as the divide model leaves it, with AX = 00C8h,
  and `*callee_status` is 2.
- `TRANSFER`: a callee's `RETF` popped a CS:IP other than the call's next
  instruction. No callee writes its own return frame. This therefore
  happens only when a segment-end stack puts a return frame across offset
  FFFFh, because the pushes wrap but the `RETF` reads physically
  contiguous memory.

**Flags.** The final flags are the divide's. At a `CODE` stop they are
those of the last flag writer before the site:

| Site | Flags at a `CODE` stop |
| --- | --- |
| `+00BC` | `SUB SP,26h` |
| `+00F4` | the second `SHL DI,1`: CF is the bit shifted out, OF = MSB ⊕ CF, and AF is clear as Unicorn leaves it (architecturally undefined) |
| `+0119` | `ADC DX,0` |

Every other write, including `XOR`, `INC`, `CMP` and the memory
`ADD`/`ADC`, is overwritten before any stop and is folded. Entry FLAGS keep
bit 15 clear, as on a 386: the divide's `PUSHF`/`POPF` canonical form
clears it. Offsets wrap at 16 bits and physical addresses at 1 MiB. The
`LES` pair, `POP` and this routine's `RETF` frame are read physically
contiguous, and each callee keeps its own contract. Null RAM or CPU, or RAM
that is not exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u7f_weighted_00a8_cpu.c` runs under CTest
with ASan/UBSan and compiles with `-Wconversion -Wsign-conversion -Werror`.
It uses no game bytes: only the three `LCALL` encodings, synthetic records
and synthetic weights. An independent reference of the wrapping weighted
sum, the truncating division and the divide's flags predicts all of the
following:

- every register;
- the copied record, the sum, j, the stored result and the saved BP;
- every byte outside the stack window.

It passes 39,043 checks:

- 3,000 seeded records and weights, mixing small, negative and full-range
  values;
- (0 + 100) div −1, and wrapping products;
- the divide-by-zero `CALLEE`;
- each `CODE` site with its flags;
- the segment-end `TRANSFER` at Move's `RETF`;
- a relocated entry;
- the argument guards.

**Unicorn differential.** `tools/overlay/u7f_weighted_00a8_audit.py` places
the expanded START.EXE image at load segment 0, so the three SYSTEM
routines run live from the unrelocated far calls. It loads unit `0x7F` and
compares the entire 1 MiB RAM and all 14 CPU words with no mask. `CODE` is
decided from the live site bytes.

| Stops | Cases |
| --- | --- |
| Returns | 222 |
| `CODE` | 5 |
| `CALLEE` (divide by zero) | 3 |
| `TRANSFER` | 1 |

Coverage is 51/51 instruction starts and 2/2 conditional outcomes. The
callees are covered too: Move 9/9, multiply 16/16, divide 55/55 with 16/16
outcomes. The cases cover:

- 200 random worlds;
- seven divisor edges, −2^31 div −1 and three zero divisors;
- the record on the frame and on the sum;
- weights under the stack and on the frame;
- stacks at both segment ends;
- SS, the record and DS across A20;
- a random background and a relocated entry;
- five rewritten sites.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **120
cases** with no mask, 52 of them with negative quotients.

- The COM copies unit `0x7F` to paragraph `8800`. It relocates the three
  `LCALL` segment words to `9000` and copies Move, the multiply and the
  divide there at their own offsets.
- Each case compares all 14 CPU words and a 128-byte stack window. The
  window holds the record copy, the sum, the result and every callee frame.
- The result SHA-256 is
  `fed6d3285bbc439facb7bfc87221e08636bf8a1e4f79232f8e10353a15bcd018`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u7f_weighted_00a8_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u7f_weighted_00a8_audit.py --unicorn --native --output resources/u7f-00a8-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U7F `+00A8..+012F`, composing SYSTEM `0AF8:0A3F`, `0AF8:0A5B` and `0AF8:0A76` |
| `source_candidates` | `src/recovered/overlay_u7f_weighted_00a8_cpu.c` with `src/recovered/system_runtime_leaves_cpu.c`, `src/recovered/system_longmul_0a5b_cpu.c` and `src/recovered/system_longdiv_0a76_cpu.c` |
| `signals` | Exact hash and CFG, whole-RAM/CPU differential with the SYSTEM routines live and full instruction and branch-outcome coverage of the routine and its callees, every reachable stop kind, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `helper_candidate`; identity is the verifying lane's call. The game meaning of the record and weights is not asserted. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_007F_00A8 --require-current-main
```

## Pinned flag split after #1433 (2026-10-22, no claim moves)

After #1433 the model's `shl1` sets AF per the pinned rule; raw Unicorn leaves it clear, so the Unicorn audit failed on main (`flags mismatch +00A8 code-00f4-1: C=0256, original=0246`, AF). Model edit in the same PR: #1433 had written the AF line four times and left the comment 'AF clear as Unicorn leaves it'; it is now written once and the comment states the pinned rule (no behaviour change; census row carried).

The audit now declares the split through the shared `tools/overlay/pinned_shift_flags.py`: it tracks the last FLAGS-writing instruction the original executed (decoded from the original bytes at run time) and requires C to equal Unicorn **exactly**, except that after a word `DIV`/`IDIV` the six arithmetic bits must equal the pinned `instructions.h:655-673/743-761` values, and after a shift with a non-zero count AF must be **set** (pinned `get_AF` = `count & 1Fh`). A model that reverted to AF=0 fails. Nothing is masked; the audit prints how many stops used the split.

Run (PR #1446, `--unicorn`): **231** original-byte cases (222 returns, 5 CODE, 3 CALLEE, 1 TRANSFER), full 1 MiB RAM and 13 CPU words equal; the split was used at **2** stops, both after a shift. The DOSBox-X native run compares against the pinned core and needs no split.

**Per-credit sentence (OVL_007F_00A8): evidence did not depend on AF after a shift**; those 2 stops hold C to AF=1 and every other stop is exact. Credit stands.

Its audit now also calls the helper's `finish()` after emulation (2026-10-23), so a word divide that is the very last instruction before a stop has its pinned bits settled; re-run: 231 cases, unchanged.
