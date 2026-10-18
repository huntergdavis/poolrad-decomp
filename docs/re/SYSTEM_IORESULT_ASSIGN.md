# SYSTEM leaves 0AF8:04A2 and 0AF8:13E2: raw-CPU IOResult and Assign

`system_ioresult_assign_cpu.c` holds raw-CPU, full-state models of two
call-free Turbo Pascal SYSTEM leaves in the START.EXE SYSTEM segment (`0AF8`
at load segment 0). They reuse the types of `system_runtime_leaves_cpu.h`.
The VP accepted them as option (B) on 2026-10-18 (i17914, acknowledged).

In the strict-queue snapshot taken at 03:06 PDT on 2026-10-18, both are
`runtime_unmapped`:

- IOResult is called by 4 game_evidenced rows (U6F `+069E`, `+1D58`,
  `+01B8`, `+02AE`).
- Assign is called by 3 game_evidenced rows (U6F `+1602`, `+069E`, `+1D58`).

Those rows have other unready callees, so this is runtime coverage, not an
immediate composable row.

## Exact original

| Leaf | Span | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| IOResult | `04A2..04A8` | 7 / 3 | `d70153d7282a717eca08c439be05fb911569333751907973b73ca0b475e08d1b` |
| Assign | `13E2..140F` | 46 / 23 | `94e00b1b9e01bc846824e0f7eb23bc8d3c80c28b089995524ce99045af95ffa7` |

START.EXE is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

## ABI and effects

Both leaves are entered after a far call: `SS:[SP]` holds the return IP and
`[SP+2]` the return CS.

**IOResult.** The code is `XOR AX,AX; XCHG [4426h],AX; RETF`.

- AX := the word `DS:[4426h]` (InOutRes), which becomes 0.
- The arithmetic flags are `XOR AX,AX`'s (ZF = PF = 1, the rest 0).

**Assign.** The frame holds the name S (far) at `[SP+4]` and the file record
F (far) at `[SP+8]`; `RETF 8`. It runs `MOV BX,SP; PUSH DS`, loads F and S
(`LES`, `LDS`), and runs `CLD`. Then:

1. F's first word := 0 and its second := `D7B0h`.
2. The next 22 words := 0.
3. `min(length(S), 79)` characters of S go to F+48, followed by a NUL byte.

At return:

- AX = 0, CX = 0, BX = the entry SP.
- ES:DI points past the NUL and SI past the copied characters.
- DS returns through the stack (`PUSH DS … POP DS`, reread from RAM).
- The arithmetic flags are `XOR AL,AL`'s.

Because the name is read after the record is cleared, a name stored inside
the record's first 48 bytes is read back as cleared.

**Addressing.** Word stores, the `LES`/`LDS` pairs, `POP` and the `RETF`
frame are read or written physically contiguous. Offsets wrap at 16 bits,
physical addresses at 1 MiB.

**Arguments.** Null RAM or CPU, or RAM that is not exactly 1 MiB, returns
`ARGUMENT` before any change.

## Validation

**Focused C test.** `test_system_ioresult_assign_cpu.c` runs under CTest with
ASan/UBSan. The source compiles with `-Wconversion -Wsign-conversion
-Wshadow -Werror`. It passes **20** hand-predicted checks:

- IOResult over `0ED7h` flags;
- Assign with "PHLAN";
- the 79-character clamp;
- a name inside the record that the clearing empties;
- the guards.

**Unicorn differential.** `tools/overlay/system_ioresult_assign_audit.py`
runs each leaf from its original bytes in the expanded START.EXE image. After
the leaf's `RETF` it compares the entire 1 MiB RAM and all 14 CPU words with
**no mask**.

- **Cases: 359.** 154 IOResult and 205 Assign, every instruction, both
  outcomes of Assign's `JBE`.
- **Inputs.**
  - Random InOutRes words, including DS across A20.
  - Name lengths 0–255, every fifth length plus the clamp edges.
  - Names placed apart from the record, inside it, before it, and
    wrapping across segment ends.
  - SS across A20, DF set on entry, and random backgrounds.
- **Separation.** Every stack frame stays apart from the data.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **128
cases** with no mask.

- **Loading.** The COM copies the SYSTEM span `0400..14FF` to paragraph
  `7000`.
- **Each case.** It rebuilds one 64 KiB data segment and alternates the two
  leaves from a single call site.
- **Comparison.** Each case compares all 14 CPU words, the stack window and
  the whole data segment.
- **Result.** The result SHA-256 is
  `0bbdf4ef183c936c9a8316d8f276d1fd1b4814b3453cd9c76333a84942389376`, with
  COM segment `0814`.

Original bytes and generated images stay under ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_system_ioresult_assign_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/system_ioresult_assign_audit.py --unicorn --native --output resources/system-ioresult-assign/report.json
```

No classifier, queue or count ledger is changed. Adding `(0x0AF8, 0x04A2)`
and `(0x0AF8, 0x13E2)` to `RUNTIME_COVERED_ENDPOINTS` is the verifying
lane's call.
