# U77 +28BE: raw-CPU weighted LongInt sum composing SYSTEM multiply and divide

`pr_u77_weighted_28be_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x77` routine `+28BE`, entry 11. It is the full-state
counterpart of the provisional value model
[`pr_u77_weighted_28be`](U77_WEIGHTED_28BE.md), which models only the
returned DX:AX and treats the runtime arithmetic as supplied 32-bit
operations. This model leaves that file unchanged.

The VP assigned the routine to this lane on 2026-10-17 (i17655), after
Astra confirmed no ownership. Its callees are the SYSTEM LongInt multiply
`0AF8:0A5B` (PR #955) and divide `0AF8:0A76` (PR #947), both merged
raw-CPU models. The classifier lists it as `helper_candidate`, grade A.
Identity and strict credit are the verifying lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; expanded image at load segment 0 places SYSTEM at `0AF8` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U77 `+28BE` | GAME file `0x238F8..0x23972`; **123 bytes of code, 44 instructions, one conditional branch**; `RETF 4`; code SHA-256 `39dbab268eac0658980e3ef072352603eefac1cfb2b532412376cf2e970081c4` (the same code bound as the provisional document) |
| Trailing bytes | `+2939..+299B` (99 bytes) are length-prefixed text data used by the next entry; they are not executed |
| Far calls | `+28FD` `LCALL 0AF8:0A5B` and `+2922` `LCALL 0AF8:0A76`; each segment word is fixup-backed |
| Callers | U77 `+1F52` (site `+1FDA`, `PUSH CS; CALL` within the unit); far calls through the stub from U1E `+00BF` (site `+0172`), U23 `+0361` (site `+03C2`) and U6C `+29AE` (site `+29EA`) |

U7F `+00A8` ([U7F_WEIGHTED_00A8.md](U7F_WEIGHTED_00A8.md)) is the companion
sum over a seven-LongInt record. It has the same weights and divisor and
three of the same callers.

## ABI and ordered effects

The routine is entered after a far call with `SS:[SP]` the return IP,
`[SP+2]` the return CS and `[SP+4]/[SP+6]` a far record pointer P. It
returns with `RETF 4`. After `PUSH BP; MOV BP,SP; SUB SP,0Ah`:

1. The LongInt sum at `BP-8` := 0 and j := 0 (a byte at `BP-9`).
2. For j = 0..4:
   - `CBW; SHL AX,1; LES DI,[BP+6]; ADD DI,AX` reloads P on each pass.
     BX:CX := the word `P^[88h + 2j]`, zero-extended.
   - DX:AX := w[j], the LongInt at `DS:0D24h + 4j`.
   - The multiply leaves the low 32 bits of the product, which is added
     to the sum with `ADD`/`ADC` to memory.
   - `CMP BYTE [BP-9],4; JNE` closes the loop.
3. DX:AX := sum + 100 and BX:CX := `DS:[0D30h]` (that is, w[3]). The
   signed, truncating divide leaves the quotient in DX:AX.
4. The quotient is stored at `BP-4`, reloaded, and `MOV SP,BP; POP BP;
   RETF 4`.

The result is (Σ w[j]·P^.word[88h+2j] + 100) div w[3], in wrapping 32-bit
arithmetic. BX:CX end as the remainder, SI:DI as |w[3]| and ES as P's
segment. Every operand is reread from RAM in source order. The audit points
the record at j, at the sum and into the stack, and drives the weights under
the stack, where the call frames rewrite w[3] mid-loop. j and the sum have
no writer but this routine, so the loop always runs five times and the
model takes no host cap.

**Stops.**

- `CODE`: a reached `LCALL` whose opcode or entry word is not `9A 5B 0A`
  or `9A 76 0A` stops before it, with IP at the site.
- `CALLEE`: w[3] = 0 makes the divide jump to its runtime-error target
  `00E2h`, and `*callee_status` is 2.
- `TRANSFER`: a callee's `RETF` popped a CS:IP other than the call's next
  instruction. This happens only when a segment-end stack puts a return
  frame across offset FFFFh.

**Flags.** The final flags are the divide's. At a `CODE` stop they are
those of the last flag writer before the site:

| Site | Flags at a `CODE` stop |
| --- | --- |
| `+28FD` | the second `SHL DI,1`: CF is the bit shifted out, OF = MSB ⊕ CF, and AF is clear as Unicorn leaves it (architecturally undefined) |
| `+2922` | `ADC DX,0` |

The `SUB`, `XOR`, first `SHL`, `ADD DI,AX`, `INC`, `CMP` and memory
`ADD`/`ADC` writes are overwritten before any stop and are folded. DF is
not touched, so it reaches the divide's `PUSHF`. Entry FLAGS keep bit 15
clear, as on a 386. Offsets wrap at 16 bits and physical addresses at
1 MiB. The `LES` pairs, `POP` and this routine's `RETF` frame are read
physically contiguous, and each callee keeps its own contract. Null RAM or
CPU, or RAM that is not exactly 1 MiB, returns `ARGUMENT` before any
change.

## Validation

**Focused C test.** `test_overlay_u77_weighted_28be_cpu.c` runs under CTest
with ASan/UBSan and compiles with `-Wconversion -Wsign-conversion -Werror`.
It uses no game bytes: only the two `LCALL` encodings, synthetic records and
synthetic weights. An independent reference of the wrapping weighted sum,
the truncating division and the divide's flags predicts every register, the
sum, j, the stored result and every byte outside the stack window. It
passes 18,028 checks:

- 3,000 seeded records and weights;
- wrapping products, and (0 + 100) div −1;
- the divide-by-zero `CALLEE`;
- each `CODE` site with its flags;
- the segment-end `TRANSFER` at the multiply's `RETF`;
- a relocated entry;
- the argument guards.

**Unicorn differential.** `tools/overlay/u77_weighted_28be_cpu_audit.py`
places the expanded START.EXE image at load segment 0, so the multiply and
divide run live from the unrelocated far calls. It loads unit `0x77` and
compares the entire 1 MiB RAM and all 14 CPU words with no mask. `CODE` is
decided from the live site bytes.

| Stops | Cases |
| --- | --- |
| Returns | 223 |
| `CODE` | 4 |
| `CALLEE` (divide by zero) | 3 |
| `TRANSFER` | 1 |

Coverage is 44/44 instruction starts and 2/2 conditional outcomes. The
callees are covered too: multiply 16/16, divide 55/55 with 16/16 outcomes.
The cases cover:

- 200 random worlds;
- seven divisor edges, −2^31 div −1 and three zero divisors;
- the record on j, on the sum and on the stack;
- weights under the stack and on the frame;
- stacks at both segment ends;
- SS, the record and DS across A20;
- a random background and a relocated entry;
- four rewritten sites.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **120
cases** with no mask, 49 of them with negative quotients.

- The COM copies unit `0x77` to paragraph `8800`. It relocates both
  `LCALL` segment words to `9000` and copies the multiply and divide there
  at their own offsets.
- Each case compares all 14 CPU words and a 128-byte stack window. The
  window holds the sum, j, the result and every callee frame.
- The result SHA-256 is
  `a71958e2aeb108414a378470bc9502c132a4c369a621d97d959f2d46c4dafdc5`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u77_weighted_28be_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u77_weighted_28be_cpu_audit.py --unicorn --native --output resources/u77-28be-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U77 `+28BE..+2938`, composing SYSTEM `0AF8:0A5B` and `0AF8:0A76` |
| `source_candidates` | `src/recovered/overlay_u77_weighted_28be_cpu.c` with `src/recovered/system_longmul_0a5b_cpu.c` and `src/recovered/system_longdiv_0a76_cpu.c` |
| `signals` | Exact hash and CFG, whole-RAM/CPU differential with the SYSTEM routines live and full instruction and branch-outcome coverage of the routine and its callees, every reachable stop kind, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `helper_candidate`; identity is the verifying lane's call. The provisional value model stays as it is. The game meaning of the record words and weights is not asserted. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.
