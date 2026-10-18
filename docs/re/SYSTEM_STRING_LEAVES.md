# SYSTEM string leaves 0AF8:0B0B and 0AF8:0B98: raw-CPU Pascal string load and concatenation

`pr_system_strload_0b0b` and `pr_system_strcat_0b98` are raw-CPU, full-state
models of two Turbo Pascal SYSTEM string leaves in the START.EXE SYSTEM
segment (`0AF8` at load segment 0). They reuse the types of
`system_runtime_leaves_cpu.h`. The VP approved them on 2026-10-18 (i17812).

In the strict-queue snapshot taken at 03:06 PDT on 2026-10-18, both are
`runtime_unmapped` targets (decimal offsets 2827 and 2968). 227 rows call one
or both (207 game_evidenced). For 41 of those rows, the leaves leave exactly
one other unready callee (39 of them game_evidenced):

- U86 `+08C3` for 26 rows;
- resident `00BA:16EC` for 5;
- GetMem `0AF8:023F` (PR #983) for U86 `+028D`.

The queue counts a runtime edge as covered only once it is in the hand-kept
`RUNTIME_COVERED_ENDPOINTS`, and the verifying lane makes that change.

## Exact original

| Leaf | Span | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| String load | `0B0B..0B24` | 26 / 12 | `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f` |
| String concatenation | `0B98..0BC3` | 44 / 19 | `c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64` |

START.EXE is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

## ABI and effects

Both leaves are entered after a far call. `SS:[SP]` holds the return IP and
`[SP+2]` the return CS. `[SP+4]`/`[SP+6]` is the source string S and
`[SP+8]`/`[SP+0Ah]` the destination D; both are read through `SS:[BX+disp]`
with BX := SP. Each leaf ends with `RETF 4`, which pops S and leaves D on
the stack. At the end:

- DF is clear;
- DS has its entry value back (through DX);
- CX is 0 and BX holds the entry SP.

`REP MOVSB` copies forward one byte at a time and rereads each source byte
after earlier writes, so overlapping strings behave as on the CPU (a
destination just past the source replicates the leading bytes). SI and DI
wrap at 16 bits.

- **Load:** copies the length byte (`LODSB; STOSB`) and then that many
  bytes. AL is the length; the final flags are `XOR CH,CH`'s.
- **Concatenation:** reads D's length into CX and S's length into AL, then
  adds AL to D's length byte. On carry the byte becomes FFh, and AL := `NOT`
  of the old length, which is how many bytes still fit. DI then moves past
  D's old text (`ADD DI,CX; INC DI`) and AL bytes of S are copied. AL is the
  count copied. The final flags are `INC DI`'s, with CF from `ADD DI,CX`.

The frame pointers (`LES`/`LDS`) and the `RETF` frame are read physically
contiguous. Null RAM or CPU, or RAM that is not exactly 1 MiB, returns
`ARGUMENT` before any change.

## Validation

**Focused C test.** `test_system_string_leaves_cpu.c` runs under CTest with
ASan/UBSan. The source compiles with `-Wconversion -Wsign-conversion
-Wshadow -Werror`. It passes 24 checks predicted by hand:

- a plain load, with every register and flag;
- an overlapping load that replicates bytes;
- a load whose SI and DI wrap past `FFFF`;
- a concatenation, with its flags;
- the 255 clamp and the exact 255 fit;
- the `ADD DI,CX` carry kept through `INC`;
- the guards.

**Unicorn differential.** `tools/overlay/system_string_leaves_audit.py`
runs each leaf from its original bytes in the expanded START.EXE image. It
compares the entire 1 MiB RAM and all 14 CPU words after `RETF 4`, with
**no mask**. That gives **414 cases** (205 load, 209 concatenation), and
every instruction of both leaves executes. The cases cover:

- random lengths 0–255;
- strings apart, overlapping in both directions, and at the same address;
- strings wrapping past `FFFF`;
- empty and full strings, the exact fit and the clamp;
- DI carry and DI reaching `8000h`;
- SS across A20, DF set on entry, and a random background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **128
cases** with no mask, alternating the two leaves.

- **Loading.** The COM copies the SYSTEM span `0B00..0BFF` to paragraph
  `7000`.
- **Each case.** It builds both strings from a random blob in one 64 KiB
  segment: apart, overlapping both ways, at the same address, or wrapping.
  It then calls the leaf with IRQs masked.
- **Comparison.** Each case compares all 14 CPU words, the stack window and
  the whole string segment.
- **Result.** The result SHA-256 is
  `c8f47a7e28b93cb9bfb7e70730cb9d11a82547e0d84bbbc80cff33b2f755d2b7`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_system_string_leaves_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/system_string_leaves_audit.py --unicorn --native --output resources/system-strings/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | SYSTEM `0AF8:0B0B` string load and `0AF8:0B98` string concatenation |
| `source_candidates` | `src/recovered/system_string_leaves_cpu.c` |
| `signals` | Exact span hashes; a whole-RAM/CPU differential with no mask over overlapping and wrapping strings; every instruction executed; an independent C test; native synthetic execution |
| `score` | 4/4 evidence checks passed; a checklist, not a probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | These are runtime (library) models; how they count toward game credit is the verifying lane's call |

No classifier, queue or count ledger is changed.
