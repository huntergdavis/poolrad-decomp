# SYSTEM Str 0AF8:1366 and resident 00BA:1037: raw-CPU longint-to-string and byte-to-string

`pr_system_str_1366` is a raw-CPU, full-state model of Turbo Pascal
`Str(X: Longint : Width, S)` in the START.EXE SYSTEM segment (`0AF8` at load
segment 0): its far entry, its near longint-to-decimal core `1254`, and that
core's table of ten 32-bit powers of ten at `CS:12A6`.
`pr_res_00ba_1037_cpu` is the complete resident routine `00BA:1037`, which
composes `Str` with the merged StrStore `0AF8:0B25` to turn a byte into a
Pascal string. The VP approved the pair on 2026-10-18 (i17881) and then
cleared the next step, U7D `+0565` (i17885).

In the strict-queue snapshot taken at 03:06 PDT on 2026-10-18:

- `Str` is `runtime_unmapped` and is called by 17 game_evidenced rows.
- `00BA:1037` appears in 15 game_evidenced rows.
- With the merged string leaves (#987, #1001), GetMem (#983) and U9E `+0E52`
  (#999), `00BA:1037` is the only unready callee of U7D `+0565` (139 bytes,
  game_evidenced).

## Exact original

| Routine | Span | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| Str entry | `0AF8:1366..13B0` | 75 / 39 | `e1b3b1952105b79c16db3429efa5be364318cf12c145d074f319fdccee589362` |
| Decimal core | `0AF8:1254..12A5` | 82 / 37 | `20438c4c9e2a4c979493d273262e03e5217a1b16bb3372bd48d22d00982492ec` |
| Power table | `0AF8:12A6..12CD` | 40 (10^9 … 1) | checked value by value |
| Byte to string | `00BA:1037..1072` | 60 / 28 | `b448dc38524a2573729f1c9a7cb69e667407ca95f02c826a2f65557bcee76b44` |
| StrStore (merged model, composed) | `0AF8:0B25..0B48` | 36 / 16 | `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` |

START.EXE is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

## Str: ABI and effects

Str is entered after a far call. `SS:[SP]` holds the return IP and `[SP+2]`
the return CS, then:

| Frame slot | Contents |
| --- | --- |
| `[SP+4]` | the result's maximum length M |
| `[SP+6]`/`[SP+8]` | the result S (offset, segment) |
| `[SP+0Ah]` | Width |
| `[SP+0Ch]`/`[SP+0Eh]` | X, low word first |

Str ends with `RETF 0Ch`.

1. **Frame.** `PUSH BP; MOV BP,SP; SUB SP,20h`. ES := SS, and the near
   `CALL 1254` converts X into the temporary `SS:BP-20h`.
2. **Digits.** The core negates a negative X (two's complement) after
   storing `'-'`. It finds the first power not above the value among
   10^9 … 10, or 1. Each digit is the number of subtractions before a borrow
   (`SUB`/`SBB`, then `ADD`/`ADC` back). On return CX is the number of
   characters, DI is restored, BX = 0 and SI = `12CEh`.
3. **Length.** With signed comparisons, Width := min(Width, M) and the count
   := min(count, M). The length is L := max(Width, count).
4. **Result.** S := L, then L − count spaces, then the first count
   characters. The pad path pushes CX around `REP STOSB`.
5. **Stack residue.** `PUSH DS` (`BP-22h`) overwrites the call's return word
   `+137A`. `PUSH SS` (`BP-24h`), or the pad path's `PUSH CX`, overwrites the
   core's saved DI. The full-RAM comparison checks these words.

At return:

- DX = M, BX = 0, CX = 0.
- AX = 0, or after padding `(pad & FF00h) | 20h`.
- DS is restored and ES:DI points past S.
- DF is clear and the arithmetic flags are `SUB AX,CX`'s.

The near call's return word lies below the temporary, so the core's `RET`
always comes back to `+137A`.

**Entry requirements.** The code must sit at its original offsets (IP =
`1366h`), and the power table must hold the original ten powers. Otherwise,
or for null RAM or CPU or RAM that is not exactly 1 MiB, the model returns
`ARGUMENT` before any change.

## 00BA:1037: ABI and effects

The routine is entered after a far call. `[SP+4]` holds the byte b and
`[SP+6]`/`[SP+8]` the far result R; `RETF 2` leaves R on the stack. After
`PUSH BP; MOV BP,SP; SUB SP,100h`:

1. It calls `Str(Longint(b) : 0, T, 255)`, where T is the 256-byte
   temporary `SS:BP-100h`.
2. It calls `StrStore(T, R, 255)`.
3. It returns with `MOV SP,BP; POP BP; RETF 2`.

Both `LCALL`s read their operands from RAM:

| Status | When |
| --- | --- |
| `CODE` | the bytes are not `9A 66 13` (Str) or `9A 25 0B` (StrStore); the model stops at the site with the arguments pushed |
| `CALLEE` | a callee rejected its entry (Str's power table is not the original) |
| `TRANSFER` | a callee's `RETF` came back elsewhere; an unexercised guard |

Flags at a Str site are `XOR AX,AX`'s; at the StrStore site and on return
they are the callees'. Every operand is reread from RAM in source order.
`LES` pairs, `POP` and the `RETF` frames are read physically contiguous.

## Validation

**Focused C test.** `test_system_str_res1037_cpu.c` runs under CTest with
ASan/UBSan. Both sources compile with `-Wconversion -Wsign-conversion
-Wshadow -Werror`. It passes **54** hand-predicted checks:

- `Str(12345)`, including the temporary, the saved DS and the residual SS;
- `Str(-42 : 6)`, with its padding and pushed count;
- 0, −2147483648, 10^9 and 999,999,999;
- M = 3 truncation, Width above M, and a negative Width;
- an altered table, which returns `ARGUMENT`;
- `00BA:1037` with b = 200 (every register and the flags);
- 1037's `CODE` and `CALLEE` stops;
- the guards.

**Unicorn differential.** `tools/overlay/system_str_res1037_audit.py` runs
the original bytes from the expanded START.EXE image. After each stop it
compares the entire 1 MiB RAM and all 14 CPU words with **no mask**.

- **Cases: 584.** 171 run Str directly and 413 run 1037, including all 256
  byte values.
- **Stops.** 581 return, 2 `CODE`, 1 `CALLEE`.
- **Coverage.** All 104 instruction starts of Str, its core and 1037, and
  both outcomes of all 11 conditional jumps.
- **Str inputs.** The value edges include 0, ±9/10, 65535/65536,
  999,999,999/10^9 (the `JAE` power edge), the extremes and random values.
  Width covers the signed and unsigned edges, and M is 0 … 255.
- **Placements.** Results sit apart or across segment ends. Temporaries wrap
  below 0, and there are SS-across-A20, DF-set and random-background cases.
- **Separation.** Every stack frame stays apart from the result.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **128
cases** (64 Str, 64 1037) with no mask.

- **Loading.** The COM copies the SYSTEM span `0B00..13FF` to paragraph
  `7000`, and 1037 to `7800` with its two `LCALL` segments relocated.
- **Each case.** It pushes either frame and calls from one call site with
  IRQs masked.
- **Comparison.** Each case compares all 14 CPU words, a `180h`-byte stack
  window (1037 nests Str and StrStore) and the whole 64 KiB result segment.
- **Result.** The result SHA-256 is
  `0069c9fda9dfe340db44bc33885ac696860a388a6895783ef3dfeeef0bd0afe3`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_system_str_res1037_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/system_str_res1037_audit.py --unicorn --native --output resources/system-str-res1037/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | SYSTEM `0AF8:1366` Str (with core `1254`) and resident `00BA:1037` byte to string |
| `source_candidates` | `src/recovered/system_str_1366_cpu.c`, `src/recovered/resident_00ba_byte_string_1037_cpu.c` |
| `signals` | Exact span hashes and the power table; a whole-RAM/CPU differential with no mask over every instruction and conditional outcome; composed merged StrStore; an independent C test; native synthetic execution |
| `score` | 4/4 evidence checks passed; a checklist, not a probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Str is a runtime (library) model; `00BA:1037` is a game resident routine whose credit is the verifying lane's call |

No classifier, queue or count ledger is changed.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py RES_00BA_1037 --require-current-main
```
