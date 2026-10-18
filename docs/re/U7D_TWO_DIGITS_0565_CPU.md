# U7D +0565: raw-CPU two-digit string composing 00BA:1037 and the SYSTEM string leaves

`pr_u7d_two_digits_0565_cpu` is a raw-CPU, full-state model of the complete
GAME.OVR unit `0x7D` routine `+0565`. It turns a byte into a two-character
Pascal string:

- a leading `0` below 10;
- the first two digits from 100 up.

The VP cleared it on 2026-10-18 (i17885), after confirming ownership.

In the 03:06 PDT queue snapshot it is game_evidenced (grade B). Its only
unready callee was resident `00BA:1037`, which is modeled in PR #1004. Every
other callee is a merged raw-CPU model:

- StrStore `0B25`;
- StrLoad `0B0B` and StrConcat `0B98` (#987);
- Copy `0B57` (#1001).

## Exact original

| Item | Value |
| --- | --- |
| Span | unit `0x7D` `+0565..+05E2`, GAME.OVR file offset `0x24331` |
| Size | 126 bytes, 54 instructions; `RETF 2` at `+05E0` |
| SHA-256 | `5674e43157a56215d0d0afca75e179c3f8653d86e39560eb176aec46ec261260` |
| Calls | seven `LCALL`s: `+0576` → `00BA:1037`; `+0584`, `+05B2`, `+05D8` → `0B25`; `+059A` → `0B0B`; `+05A4` → `0B98`; `+05CA` → `0B57` |
| Constant | the one-character string `'0'` at `+0563`, just before the entry |

The inventory row for `+0565` is 139 bytes, 59 decoded instructions. It also
spans the 13 constant bytes that follow the `RETF`, which are string
constants used elsewhere.

GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; START.EXE
is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

## ABI and effects

The routine is entered after a far call:

| Frame slot | Contents |
| --- | --- |
| `SS:[SP]`, `[SP+2]` | return IP, return CS |
| `[SP+4]` | the byte b |
| `[SP+6]`/`[SP+8]` | the far result R |

`RETF 2` leaves R on the stack. After `PUSH BP; MOV BP,SP; SUB SP,12Ah` it
uses two locals: the 256-byte temporary T = `SS:BP-129h`, and s =
`SS:BP-29h` (a `string[40]`).

1. **Convert.** T := `00BA:1037(b)`. AL := b over the entry AH, and the
   whole word is pushed. Then s := T (StrStore, maximum 40).
2. **Pad.** `CMP BYTE [BP+6],0Ah`. If b < 10: T := `'0'` (StrLoad from
   `CS:0563`), T := T + s (StrConcat), then s := T (StrStore, 40).
3. **Truncate.** T := `Copy(s, 1, 2)`, then R := T (StrStore, 40).
4. **Return.** `MOV SP,BP; POP BP; RETF 2`.

Each callee leaves its result pointer on the stack for the next one, as
Turbo Pascal's string temporaries do.

**Composition.** When an `LCALL` is reached, its opcode and offset are
checked and its segment is read from RAM. The callee then runs as its merged
raw-CPU model, with the CPU passed by field copy:

| Status | When |
| --- | --- |
| `OK` | the routine returned |
| `CODE` | the opcode or offset differs; the model stops at the site with the arguments pushed |
| `CALLEE` | a callee stopped (`00BA:1037`'s `CODE`, `CALLEE` or `TRANSFER`, or an `ARGUMENT`) |
| `TRANSFER` | a callee came back elsewhere; an unexercised guard |

Every operand is reread from RAM in source order. `LES` pairs, `POP` and
the `RETF` frame are read physically contiguous. Offsets wrap at 16 bits,
physical addresses at 1 MiB.

**Flags and registers.**

- On return, the registers and flags are the last StrStore's: AX = 2,
  CX = 0, ES:DI past R, DS and DX = the entry DS, DF clear, and `XOR CH,CH`'s
  arithmetic flags.
- At a `CODE` stop the flags are the previous writer's: `SUB SP,12Ah` at the
  first site, `CMP` at StrLoad's (or, for b ≥ 10, at Copy's), and otherwise
  the previous callee's.

## Validation

**Focused C test.** `test_overlay_u7d_two_digits_0565_cpu.c` runs under
CTest with ASan/UBSan. The source compiles with `-Wconversion
-Wsign-conversion -Wshadow -Werror`. It passes **51** hand-predicted checks:

- b = 7, 0, 9, 10, 42, 240 and 255, giving "07", "00", "09", "10", "42",
  "24" and "25";
- R and both locals for each, with every register and the flags;
- a `CODE` stop at the Copy site;
- a `CALLEE` stop through `00BA:1037` and an altered power table;
- the guards.

**Unicorn differential.** `tools/overlay/u7d_two_digits_0565_cpu_audit.py`
runs the unit's original bytes; `00BA:1037` and the SYSTEM leaves run from
the expanded START.EXE image. After each stop it compares the entire 1 MiB
RAM and all 14 CPU words with **no mask**.

- **Cases: 269.** 260 return, 7 `CODE` (one per `LCALL` site), 2 `CALLEE`
  (a nested `00BA:1037` site and an altered power table).
- **Coverage.** 54/54 instruction starts, 2/2 conditional outcomes, and
  every byte value.
- **Placements.** Wrapped callee frames, SS across A20, DF set on entry, and
  a random background.
- **Separation.** Every stack frame and result string stays apart from the
  code.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **128
cases** (b = 0..23 and random) with no mask.

- **Loading.** The COM copies the SYSTEM span `0B00..13FF` to paragraph
  `7000` and `00BA:1037` to `7800`. It copies unit `0x7D` to `8800` with all
  nine `LCALL` segments relocated.
- **Comparison.** Each case compares all 14 CPU words, a `300h`-byte stack
  window and the whole 64 KiB result segment.
- **Result.** The result SHA-256 is
  `862d35b967c0a48573a1f2bf8d9fdf30c032219992251881e4cfef2dfd797271`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_overlay_u7d_two_digits_0565_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/u7d_two_digits_0565_cpu_audit.py --unicorn --native --output resources/u7d-0565/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | GAME.OVR unit `0x7D` `+0565`, a byte as a two-character string |
| `source_candidates` | `src/recovered/overlay_u7d_two_digits_0565_cpu.c` |
| `signals` | Exact span hash; a whole-RAM/CPU differential with no mask over every instruction and outcome and every byte; merged callee models composed; an independent C test; native synthetic execution |
| `score` | 4/4 evidence checks passed; a checklist, not a probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Stacked on PR #1004 (`00BA:1037`); `TRANSFER` is an unexercised guard |

No classifier, queue or count ledger is changed.
