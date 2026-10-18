# U31 +128D: raw-CPU table value composing SYSTEM Move

`pr_u31_table_value_128d_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x31` routine `+128D`. The VP assigned it to this lane on
2026-10-17. In the census-2115 queue its only callee is SYSTEM Move
(`runtime:2623`), with no inline coverage, model, branch or main file. The
routine calls Move as a real far call into the raw-CPU
[`pr_system_move_0a3f`](SYSTEM_RUNTIME_LEAVES.md) from #936, now merged. The
timeboxed `deja` recall for the runtime leaves reached its
cap, so nothing is reused. The classifier lists the routine as
`helper_candidate`, grade B. Identity and strict credit are the verifying
lane's decision.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U31 `+128D` | GAME file `0xBE4D..0xBF89`; **317 bytes, 115 instructions, twelve conditional branches**; `RETF 8`; SHA-256 `6b08714fe62e86051b329de8992053d1da8d01acda98843279b10218a000f68a` |
| Call | `+12B1 LCALL 0AF8:0A3F` (Move); the segment word is fixup-backed |
| Caller | U31 `+13CA`, site `+1513`, through `PUSH CS; CALL` |

## ABI and ordered effects

The routine is entered after a far call with `SS:[SP]` the return IP,
`[SP+2]` the return CS, `[SP+4]/[SP+6]` a far pointer P and
`[SP+8]/[SP+0Ah]` a far pointer Q. It returns with `RETF 8`. After
`PUSH BP; MOV BP,SP; SUB SP,12h`:

1. c := P^[2Eh]. Move copies the 16-byte row `DS:(5582h + 16c)` into the
   local L at `SS:BP-12h`.
2. v := L[9] · L[10] (`MUL`), keeping the low byte in `[BP-2]`.
3. If P^[32h] > 0 (signed), v += 8 · P^[32h]. If L[11] > 0, v += 2 · L[11].
4. If c = 55h, let R be the far pointer Q^[108h]. If R^[0Ah]/R^[0Ch] is
   nonzero and T^[76h] > 0 for the far pointer T = R^[0Ah], then v := 8.
5. If L[14] AND 8, v += 2 · (L[5] − 1). If L[1] <= 1, v += 3.
6. v := 0 in each of these cases:
   - Q^[100h] + L[1] > 3, a signed word compare;
   - P^[3Eh] = 84h and (P^[3Dh] AND 0Fh) <> Q^[0A0h];
   - P^[3Dh] = 53h;
   - P^[36h] <> 0.
7. `[BP-1]` := v and AL := `[BP-1]`. Then `MOV SP,BP; POP BP; RETF 8`.

Every sum keeps only its low byte in `[BP-2]`. AH ends as the high byte of
Q^[100h] + L[1], and DX as Q^[100h]. CX is 3 when step 3 added the P^[32h]
term, and 0 otherwise. Move leaves BX = its entry SP (BP − 20h) and SI past
the source row. DI and ES end as P.

**Flags.** The final flags are `CMP P^[36h],0`'s, with DF clear from
Move's `CLD`. The `XOR`, `SHL`, `ADD`, `MUL`, `DEC`, `AND` and `OR` writes
are overwritten before any read and are folded. Two exceptions are
computed: `SUB SP,12h` and `ADD DI,5582h`, whose flags a `CODE` stop
exposes.

**Composition.** The far call is read from RAM: `9A 3F 0A` plus the
loader-relocated SYSTEM segment. The model pushes CS and the next IP,
relative to the entry IP, and runs the Move model at that segment. Other
site bytes return `CODE`, with IP at the site and the five argument pushes
done. A `TRANSFER` guard checks Move's return. It cannot trigger, because
Move writes only the 16-byte local above its own return frame.

Every operand is reread from RAM in source order. Offsets wrap at 16 bits
and physical addresses at 1 MiB. `LES` pairs, `POP` and the `RETF` frames
are read physically contiguous. Null RAM or CPU, or RAM that is not exactly
1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_overlay_u31_table_value_128d_cpu.c` runs under
CTest with ASan/UBSan and compiles with `-Wconversion -Werror`. Its only
game bytes are the five-byte far-call site. A byte-level reference builds
the expected 1 MiB buffer and every CPU word. It passes 18,136 checks:

- each branch toggled on its own, including the wrapped L[5] − 1 and the
  word sum with AH = 1;
- 3,000 seeded random worlds;
- another entry offset;
- a `CODE` stop with exact `ADD DI,5582h` flags;
- the argument guards.

**Unicorn differential.** `tools/overlay/u31_table_value_128d_audit.py`
places the expanded START.EXE image at load segment 0, so the original
Move runs from the unrelocated far call. It loads unit `0x31` and compares
the entire 1 MiB RAM and all 14 CPU words with no mask. **413 returning
and 3 `CODE`-stop cases** pass, with 115/115 instruction starts, 24/24
conditional outcomes and Move 9/9. The cases cover:

- 400 branch-biased random worlds;
- P and Q aliasing the copied row, the saved BP, Move's frame and the
  routine's own frame;
- stacks at both segment ends, SS and the records across A20;
- a random background and three rewritten sites.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **160
cases** with no mask. Each compares all 14 CPU words and an 80-byte stack
window. The COM copies unit `0x31` to paragraph `8800` and Move to
`9000:0A3F`. It relocates the fixup-backed segment word to `9000`,
loader-style. The result SHA-256 is
`4892d9b06c12762c143f24e41ba692cc7cc1117cafc0bd927ab29077450b8ffa` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u31_table_value_128d_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u31_table_value_128d_audit.py --unicorn --native --output resources/u31-128d-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | U31 `+128D..+13C9`, composing SYSTEM Move `0AF8:0A3F` |
| `source_candidates` | `src/recovered/overlay_u31_table_value_128d_cpu.c` with `src/recovered/system_runtime_leaves_cpu.c` |
| `signals` | Exact hash and CFG, fixup-backed far call, whole-RAM/CPU differential with Move live and full instruction and branch-outcome coverage, `CODE` stops, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | `helper_candidate`; identity is the verifying lane's call. The record fields' game meaning is not asserted. Natural saved-game reach is unmeasured. |

No classifier, queue or count ledger is changed.
