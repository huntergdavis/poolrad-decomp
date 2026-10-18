# SYSTEM 0AF8:0A5B LongInt multiply (raw CPU)

`pr_system_longmul_0a5b` is a raw-CPU, full-state model of the Turbo Pascal
SYSTEM LongInt multiply at loaded `0AF8:0A5B`. The VP approved it on
2026-10-17 (i17652) as a runtime model with **no strict or identity
credit**, the same precedent as the [LongInt divide](SYSTEM_LONGDIV_0A76.md)
and the [SYSTEM runtime leaves](SYSTEM_RUNTIME_LEAVES.md). In the census on
main `11c22a0` it (`runtime:2651`) is the last unmodeled callee of two
bodies:

- U7F `+00A8`, which also calls Move and the divide;
- U77 `+28BE`, which also calls the divide.

The VP assigned both bodies to this lane (i17655) after Astra confirmed no
ownership.

**Prior work.** `deja "longmul"` finds a 2026-10-16 worker-2 "longmul"
review thread, but none of its commits or files exist in any fetched
branch of this repository. Main has no model of `0AF8:0A5B`: its resident
callers treat the routine as a returned-state callback (see
[SLOT_A_SECOND_KEY_RESIDENT_002A_CALLER_RETURN.md](SLOT_A_SECOND_KEY_RESIDENT_002A_CALLER_RETURN.md)),
and `resident_time_caller_08ae_0036` models a single unrelated `MUL`.
Nothing is reused.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`; expanded with `resident_loader_map.unpack` |
| LongInt multiply | loaded `0AF8:0A5B..0A75`, file offset `0xB6D1` (unique), immediately before the divide; **27 bytes, 16 instructions, straight-line**; plain `RETF`; SHA-256 `74b9ab3b26ee7b54bf51f067f0825dea48772933b210dc5b38897464e5f65745` |

## ABI and ordered effects

The routine is entered after a far call with `SS:[SP]` the return IP and
`[SP+2]` the return CS. The multiplicand a is in DX:AX and the multiplier b
in BX:CX.

1. `MOV SI,AX; MOV DI,DX` keep a.
2. `MUL CX` forms p = a.lo × b.lo, and `PUSH AX; PUSH DX` stack it.
3. `MOV AX,SI; MUL BX; MOV BX,AX` leaves BX := low word of a.lo × b.hi.
4. `MOV AX,DI; MUL CX; MOV CX,AX` leaves CX := low word of a.hi × b.lo.
5. `POP DX; POP AX; ADD DX,BX; ADD DX,CX; RETF`.

DX:AX is the low 32 bits of a × b, the same for signed and unsigned
operands. SI:DI keep a. The two words of p stay in RAM below the returned
SP.

**Flags.** The final flags are `ADD DX,CX`'s. The `MUL` flags and the first
`ADD`'s are overwritten and are folded, and every other flag keeps its
entry value. Offsets wrap at 16 bits and physical addresses at 1 MiB. The
`POP`s and the `RETF` frame are read physically contiguous. Null RAM or
CPU, or RAM that is not exactly 1 MiB, returns `ARGUMENT` before any
change.

## Validation

**Focused C test.** `test_system_longmul_0a5b_cpu.c` runs under CTest with
ASan/UBSan and compiles with `-Wconversion -Wsign-conversion -Werror`. It
checks every product against 64-bit multiplication. It checks both cross
products, SI/DI, the two stacked words and the exact flags against an
independent reference. Each case compares the whole 1 MiB buffer with an
expected copy. It passes 25,554 checks:

- every pair of 16 edge values;
- 4,000 seeded pairs;
- pushes that wrap below offset 0, and a high stack;
- the argument guards.

**Unicorn differential.** `tools/overlay/system_longmul_0a5b_audit.py`
places the expanded START.EXE image at load segment 0. It runs the original
at `0AF8:0A5B` and stops at its return. It compares the entire 1 MiB RAM and
all 14 CPU words with no mask. **796 returning cases** pass, with 16/16
instruction starts. The cases cover:

- every pair of 17 edge values;
- 500 random pairs;
- stacks at both segment ends, SS across A20 and a random background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **489
cases**: every edge pair and 200 random pairs, with no mask. Each compares
all 14 CPU words and a 32-byte stack window holding both pushed words. The
COM runs the original bytes at `9000:0A5B`. The result SHA-256 is
`0bd36f2e362c04456158e762533dd6c75706e13ff342b534947365922438228f` with COM
segment `0814`. Original bytes and generated images stay under ignored
`resources/`.

```sh
ctest --test-dir <build> -R poolrad_system_longmul_0a5b_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/system_longmul_0a5b_audit.py --unicorn --native --output resources/longmul-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | SYSTEM LongInt multiply, loaded `0AF8:0A5B..0A75` |
| `source_candidates` | `src/recovered/system_longmul_0a5b_cpu.c` |
| `signals` | Exact hash, whole-RAM/CPU differential with full instruction coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Runtime library code: no strict or identity credit. Next are U7F `+00A8` and U77 `+28BE`, which compose it. |

No classifier, queue or count ledger is changed.
