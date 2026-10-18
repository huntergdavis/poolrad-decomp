# SYSTEM 0AF8:0A76 LongInt divide (raw CPU)

`pr_system_longdiv_0a76` is a raw-CPU, full-state model of the Turbo Pascal
SYSTEM LongInt divide at loaded `0AF8:0A76`. The VP approved it on
2026-10-17 as a runtime model with **no strict or identity credit**, the
same precedent as the [SYSTEM runtime leaves](SYSTEM_RUNTIME_LEAVES.md). In
the census-2218 queue it (`runtime:2678`) is the only unready callee of
U20 `+0355`, a 372-byte body with no prior translation. The timeboxed
`deja` recalls this evening reached their cap, so nothing is reused.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`; expanded with `resident_loader_map.unpack` |
| LongInt divide | loaded `0AF8:0A76..0AE3`, file offset `0xB6EC` (unique); **110 bytes, 55 instructions, eight conditional branches**; plain `RETF`; SHA-256 `131b97677c63a6daa216f4219868f045485e35f999a551feb16daf4ec50d69e5` |

## ABI and ordered effects

The routine is entered after a far call with `SS:[SP]` the return IP and
`[SP+2]` the return CS. The dividend is in DX:AX and the divisor in BX:CX.

1. `PUSH BP`. If the divisor is 0 (`MOV BP,CX; OR BP,BX; JE`), the routine
   pops BP, sets AX := 00C8h and jumps to `00E2`. That is runtime error 200,
   which does not return. The model stops there with `DIVIDE_ERROR`.
2. `OR BX,BX; PUSHF` saves the divisor's sign (F1). A negative divisor is
   negated with `NOT; NOT; ADD 1; ADC 0`.
3. `OR DX,DX; PUSHF` saves the dividend's sign (F2), and a negative
   dividend is negated the same way.
4. DI:SI := |divisor| and BX:CX := 0. Then 33 restoring steps run:
   - `RCL CX,1; RCL BX,1` shifts the next dividend bit into the remainder;
   - `SUB CX,SI; SBB BX,DI` subtracts the divisor, and `ADD`/`ADC` restore
     on a borrow;
   - `CMC; RCL AX,1; RCL DX,1` shifts in the quotient bit.

   DX:AX ends as |quotient| and BX:CX as |remainder|.
5. `POPF` restores F2, and the remainder is negated for a negative
   dividend. `POPF` restores F1, and the quotient is negated when the signs
   differ. `POP BP; RETF`.

This is truncating division: the remainder takes the dividend's sign, and
−2^31 ÷ −1 wraps to −2^31. SI and DI keep |divisor|.

**Flags.** The final flags are F1's, or the `ADC DX,0` of the quotient
negation. `PUSHF` stores FLAGS with bit 1 set and bits 3, 5 and 15 clear,
and `POPF` loads that canonical form, as a 386 does. The loop's flag
writes are overwritten by `POPF` and are folded. The two FLAGS words stay
in RAM below the returned SP. Physical addresses wrap at 1 MiB, and the
`RETF` frame is read physically contiguous. Null RAM or CPU, or RAM that is
not exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_system_longdiv_0a76_cpu.c` runs under CTest with
ASan/UBSan and compiles with `-Wconversion -Werror`. It checks every
quotient and remainder against 64-bit truncating division, together with
the exact flags and both stacked FLAGS words. Each case compares the whole
1 MiB buffer with an expected copy. It passes 22,697 checks:

- every pair of 16 edge values;
- 3,000 seeded pairs;
- −2^31 ÷ −1;
- the zero divisor;
- the argument guards.

**Unicorn differential.** `tools/overlay/system_longdiv_0a76_audit.py`
places the expanded START.EXE image at load segment 0. It runs the original
at `0AF8:0A76` and stops at its return, or at `0AF8:00E2` for a zero
divisor. It compares the entire 1 MiB RAM and all 14 CPU words with no
mask. **778 returning and 17 divide-error cases** pass, with 55/55
instruction starts and 16/16 conditional outcomes. The cases cover:

- every pair of 17 edge values;
- 500 random pairs;
- stacks at both segment ends, SS across A20 and a random background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **472
cases**: every nonzero edge pair and 200 random pairs, with no mask. Each
compares all 14 CPU words and a 32-byte stack window that holds both
stacked FLAGS words. The COM runs the original bytes at `9000:0A76`. The
result SHA-256 is
`df1f57273d84acf71bdf717ad15e709137fe1e8d5cd71b794049a78c040db33a` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_system_longdiv_0a76_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/system_longdiv_0a76_audit.py --unicorn --native --output resources/longdiv-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | SYSTEM LongInt divide, loaded `0AF8:0A76..0AE3` |
| `source_candidates` | `src/recovered/system_longdiv_0a76_cpu.c` |
| `signals` | Exact hash and CFG, whole-RAM/CPU differential with full instruction and branch-outcome coverage including the runtime-error exit, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Runtime library code: no strict or identity credit. Next is U20 `+0355`, which composes it. |

No classifier, queue or count ledger is changed.
