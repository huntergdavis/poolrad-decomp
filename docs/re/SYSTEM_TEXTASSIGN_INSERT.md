# SYSTEM 0AF8:04F7 and 0AF8:0C68: raw-CPU Text Assign and Insert

`system_textassign_insert_cpu.c` holds raw-CPU, full-state models of two
Turbo Pascal SYSTEM routines in the START.EXE SYSTEM segment (`0AF8` at load
segment 0):

- `04F7`, `Assign` for Text files, a leaf;
- `0C68`, `Insert`, which composes the merged Copy `0B57`, Concat `0B98` and
  StrStore `0B25` models.

The VP approved them on 2026-10-18 (i17924).

In the 03:06 PDT queue snapshot both are `runtime_unmapped`. 2 and 1
game_evidenced rows call them; those rows have other unready callees.

## Exact original

| Routine | Span | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| Text Assign | `04F7..053B` | 69 / 37 | `bebfc6bd2c1ef83464368c1d8953a0867f784e992297bad7931b7550bb18c975` |
| Insert | `0C68..0CBB` | 84 / 43 | `d57768b9caee11537d43758ba92f5e588837a76e12c8c71651c3e13dd7183bef` |

START.EXE is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

## ABI and effects

Both are entered after a far call: `SS:[SP]` holds the return IP and
`[SP+2]` the return CS.

### Text Assign (`04F7`)

The frame holds the name S at `[SP+4]` and the text record T at `[SP+8]`,
both far; `RETF 8`. After `MOV BX,SP; PUSH DS; CLD` it fills T:

| Offset | Value |
| --- | --- |
| `+0` | handle 0 |
| `+2` | mode `D7B0h` |
| `+4` | buffer size `80h` |
| `+6`, `+8`, `+10` | 0 |
| `+12`/`+14` | buffer pointer (T+80h, T's segment), as `LEA AX,[DI+74h]` computes it |
| `+16`/`+18` | open routine `CS:060E` |
| `+20` … `+47` | 14 zero words |
| `+48` | `min(length(S), 79)` characters of S, then a NUL byte |

At return AX = 0, CX = 0 and the arithmetic flags are `XOR AL,AL`'s. DS
returns through the stack and is reread.

### Insert (`0C68`)

| Frame slot | Contents |
| --- | --- |
| `[SP+4]` | Index |
| `[SP+6]` | S's maximum length M |
| `[SP+8]` | S |
| `[SP+0Ch]` | Source |

`RETF 0Ch`. After `PUSH BP; MOV BP,SP; SUB SP,200h`, the temporaries are
T1 = `SS:BP-100h` and T2 = `SS:BP-200h`. Five near calls (`PUSH CS; CALL`)
then run:

1. T1 := `Copy(S, 1, Index − 1)`;
2. T1 := T1 + Source;
3. T2 := `Copy(S, Index, 255)`;
4. T1 := T1 + T2;
5. S := T1 (StrStore with M).

Through Copy's rules:

- Index 0 puts Source first.
- An Index past the end appends Source.
- M truncates the result.

The registers and flags at return are StrStore's. The code must sit at its
original offsets (IP = `0C68h`). The leaves cannot fail once the arguments
are valid, so the only stop is `TRANSFER`, an unexercised guard.

**Addressing.** Pointer pairs, word stores, `POP` and the `RETF` frames are
read or written physically contiguous. Offsets wrap at 16 bits, physical
addresses at 1 MiB.

**Arguments.** Null RAM or CPU, or RAM that is not exactly 1 MiB, returns
`ARGUMENT` before any change.

## Validation

**Focused C test.** `test_system_textassign_insert_cpu.c` runs under CTest
with ASan/UBSan. The source compiles with `-Wconversion -Wsign-conversion
-Wshadow -Werror`. It passes **26** hand-predicted checks:

- the full TextRec layout for "PHLAN";
- inserting "ABC" into "HELLO" at 3, giving "HEABCLLO", with both
  temporaries and AX;
- index 0, giving "ABCHELLO";
- an index past the end, giving "HELLOABC";
- M = 6, giving "HEABCL";
- the guards.

**Unicorn differential.** `tools/overlay/system_textassign_insert_audit.py`
runs both routines, and every leaf Insert calls, from their original bytes
in the expanded START.EXE image. After the routine's `RETF` it compares the
entire 1 MiB RAM and all 14 CPU words with **no mask**.

- **Cases: 306.** 153 per routine.
- **Coverage.** Every instruction of both, and both outcomes of Text Assign's
  `JBE`.
- **Inputs.**
  - Text Assign: name lengths around the clamp, with the name apart from or
    inside the record, and wrapping records.
  - Insert: Index −5, 0, 1, past the end, `7FFFh` and `8000h`; M from 0 to
    255; Source apart from, inside and wrapping relative to S.
  - SS across A20, DF set on entry, and random backgrounds.
- **Separation.** Every stack frame stays apart from the data.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **128
cases** with no mask.

- **Loading.** The COM copies the SYSTEM span `0400..0CFF` to paragraph
  `7000`.
- **Each case.** It alternates the two routines from a single call site.
- **Comparison.** Each case compares all 14 CPU words, a `280h`-byte stack
  window (Insert's two 256-byte temporaries) and the whole data segment.
- **Result.** The result SHA-256 is
  `1ac3e4abff806b040cfa0f7b035598e181e1e4ead2db42df49eafde5b5f53497`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_system_textassign_insert_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/system_textassign_insert_audit.py --unicorn --native --output resources/system-textassign-insert/report.json
```

No classifier, queue or count ledger is changed. Adding `(0x0AF8, 0x04F7)`
and `(0x0AF8, 0x0C68)` to `RUNTIME_COVERED_ENDPOINTS` is the verifying
lane's call.
