# Turbo Pascal SYSTEM I/O check at 0AF8:04A9: raw CPU, runtime coverage, TERMINAL RunError stop

**Runtime coverage, not credit.** This is a Turbo Pascal SYSTEM library routine (the `{$I+}` check after each I/O call). No identity or credit is claimed, and this change does
not edit `RUNTIME_COVERED_ENDPOINTS`; adding `(0x0AF8, 0x04A9)` is the accepting review's edit. It is the first of the six SYSTEM file-I/O endpoints that travel together in the
second keystone (t29), on the declared-service pattern of [GetTime](DOS_GETTIME_00A2_CPU.md) and [Intr](DOS_INTR_000B_CPU.md), except that this routine makes no DOS call itself:
its only environment edge is a **terminal transfer** into RunError.

`pr_system_iocheck_04a9` (`src/recovered/system_iocheck_04a9_cpu.c`) models START.EXE `0AF8:[04A9,04B7)`.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| SYSTEM I/O check | `0AF8:[04A9,04B7)`, loader-expanded image `[0B429,0B437)` | **14** / 5 | `ed42ecda2f0160a0b18cabfa9716d9676927298a45a01bb362328f2241302924` |

The size is measured: the task text said 11 B; the body is `CMP WORD [4426],0` (5), `JNE` (2), `RETF` (1), `MOV AX,[4426]` (3) and `JMP 00E2` (3) = 14. The image is unpacked with
`resident_loader_map.unpack`. The audit asserts the five instructions exactly, the single `RETF` at `04B0` (the only return), the one conditional jump, no INT/port/call, and no loader
relocation in the body.

## Behaviour and stops

`CMP WORD DS:[4426],0` tests `InOutRes` as a whole WORD. Zero: `RETF` (IP and CS popped, SP + 4). Non-zero: `MOV AX,[4426]` (re-read) and `JMP 00E2`.

| Status | When | State |
| --- | --- | --- |
| `RETURNED` | `InOutRes == 0` | `RETF` executed; flags of `CMP 0,0`: ZF and PF |
| `RUNERROR` | `InOutRes != 0` | **TERMINAL.** `AX := InOutRes`, IP = `00E2`, SP and CS unchanged, nothing at `00E2` executed; the model has no resume entry |
| `ARGUMENT` | host or profile violation | nothing changed |

Flags are those of `CMP word,0`: CF, OF and AF clear, SF/ZF/PF from `InOutRes`; neither the `RETF` nor the `JMP` changes them. SP must be in `[0000,FFFC]` so the `RETF` frame never
wraps; outside it the call returns `ARGUMENT`.

### The model must not return from a non-zero InOutRes, and that is asserted three ways

1. **C test:** all 65,535 non-zero `InOutRes` values are driven and none returns `RETURNED` (66,106 checks under ASan/UBSan).
2. **Original-byte differential:** 127 `RUNERROR` cases (every single-bit value, 80 random values and edge values, under seven flag images and the stack/data aliasing cases) match the original, which reaches `00E2` and not `RETF`
   in every one; 19 `RETURN` cases are exactly the zero-`InOutRes` ones.
3. **Static proof from the bytes (the checker):** a walk over every path from `00E2` inside the SYSTEM segment (87 instructions, callees treated as returning to the flow) finds **one `RETF` and one `RET`**: the `RETF` at `013F` is
   the ExitProc dispatch (`MOV AX,0123; PUSH CS; PUSH AX; PUSH ES; PUSH BX; RETF`, so the exit procedure returns to `0123`, not to the I/O check's caller), the `RET` at `01B2` is a local helper reached by `CALL`; the only interrupts are
   `INT 21h` at `0164` (print) and `01A3`, whose `AH` is set by `MOV AH,4Ch` at `01A1` (terminate). No path returns to the caller of `IOCheck`.

### What 00E2 is, and what is NOT yet declared

`00E2` is `POP CX; POP BX; JMP 00ED`: the RunError entry that takes the caller's return address as `ErrorAddr`, then continues inside the Halt routine that
[DECLARED_BOUNDARY_0AF8_00E9.md](DECLARED_BOUNDARY_0AF8_00E9.md) covers (`[00E9,01A5)`). That declaration says the `+00E6` RunError form "is not part of this declaration" and that the print path "does not happen via +00E9". **Via `00E2` the
ErrorAddr print path does run**, so composing `IOCheck` reaches a terminal that is *not yet declared*: a `RES_0AF8_00E2` declared boundary (RunError, TERMINAL, EXIT class like Halt, prints through `INT 21h` AH=06h) or an extension of the Halt declaration is needed
before a row that reaches `04A9` can compose it as a ready dependency. That declaration is a separate change (registry plus document) and is not made here.

## Callers

Census method: segment-wide by construction, asserted by the checker (raw candidate list equals the expected decoded list): **no near transfer** to `04A9` in the SYSTEM segment, **no direct overlay far call**, and **12 resident far calls** decoded to three
resident functions (linear decode from the nearest function start lands on each call on an instruction boundary): image `85ED` (3 sites: `86B2`, `86CF`, `86EF`), image `8707` (2: `8714`, `8723`) and image `8736` (7: `87CC`, `87EA`, `8808`, `8826`, `8885`, `890D`, `892B`).
These establish use, not identity.

## Validation

Receipt: 2026-10-20, main `968d023b` plus this branch.

**Unicorn, 146 cases + 5 ARGUMENT**, comparing unmasked CPU14 and all 1,048,576 RAM bytes and the stop status: `RETURN` 19, `RUNERROR` 127. Starts **5/5**, conditional outcomes **2/2** (`JNE` taken and not taken), accounted by successor. **116** cases whose stack does
not alias the data segment are also checked against a separately written statement of the contract (RETURN iff `InOutRes == 0`, the popped frame, `AX := InOutRes` and IP `00E2` otherwise, and an independently computed flag image). Cases: 100 distinct `InOutRes` values
(zero, all 16 single bits, `0067` (103), `0100`, `7FFF`, `8000`, `FFFF`, patterns, 80 random) under seven flag images; SP `0000 0001 0100 FFFA FFFB FFFC`; data segments `0000`, `FFFF` (A20 wrap), `1234` and the code segment; an A20 stack; random RAM; and the frame overlapping
`InOutRes` itself (`SS == DS`, SP `4420`..`442A`).

**Mutation check:** four deliberate changes to the model (the `RunError` IP, a byte-for-word `InOutRes` test, the `RETF` SP step, the ZF test) are each caught by the differential.

**Native DOSBox-X, 48 runs equal** (`RETURN` 8, `RUNERROR` 40), unmasked on what it captures: all 14 CPU words (flags included), a 256-byte stack window and the whole 64 KiB data segment (**65,792 RAM bytes captured; 982,784 bytes omitted from the native comparison**). For measured COM segment
`0814` the captured physical half-open ranges are `[17040,17140)` and `[50000,60000)`; the exact native masks are `[00000,17040)` 94,272 bytes, `[17140,50000)` 233,152 bytes and `[60000,100000)` 655,360 bytes, total 982,784. The harness does not replay DOS, BIOS, code or capture machinery as
identical RAM outside its controlled windows; full RAM comparison comes from the original-byte differential.

- **Declared stub:** a `JMP FAR` at `8800:00E2` into a capture routine records the state at the transfer; **RunError itself is never executed**, so this says nothing about 00E2 and beyond.
- Native cases are synthetic frames and data. Stack-in-data aliasing, the A20-wrapped data segment and the SP bounds rest on the Unicorn run.
- RESULT.BIN SHA-256 `3ab300ab282c4d1ad3dce4c3be99ccda2dff1b917ebdc29762cadc2b54c8d651`.

### Declared substitution: the DOSBox-X binary

The native oracle is `/usr/bin/dosbox-x`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, `DOSBox-X version 2026.01.02 SDL2`, with `core=normal`, `cputype=386`, `cycles=max`. It is **not** the pinned source build `5a9c7c53481c2e933948039c33256776653f4faf`; the receipt's `dosbox_x` block records this.
The pinned build was neither rebuilt nor re-run. This body uses only `CMP` (word), `JNE`, `RETF`, `MOV` and `JMP`: no shift, rotate, `MUL`, `LES` or other instruction with a known flag-convention difference, and the SP bounds exclude stack-offset wrap. All 14 words, flags included, matched in all 48
runs. That does not prove agreement with the pinned build on states the cases do not reach; a reviewer who needs the pinned result must re-run the native harness with that binary.

## Unlocked-row delta (a delta, never a credit)

Measured with `price_row_closures.Pricer` over all 440 queue rows, products regenerated from the pinned media, with `(0x0AF8, 0x04A9)` added to the runtime-covered set **in a scratch run only** (no registry edit):

| Measure | Rows |
| --- | ---: |
| rows whose closure reaches uncovered `04A9` (reproduces par-b's 77) | 77 |
| rows that lose an uncovered SYSTEM endpoint when `04A9` alone is covered | 77 |
| rows left with **no** uncovered SYSTEM endpoint by `04A9` alone | **0** |
| rows left with no uncovered SYSTEM endpoint by `04A9` + `1491` | 0 (78 rows change) |
| rows left with no uncovered SYSTEM endpoint by all six file-I/O endpoints | 17 (82 rows change) |

So `04A9` alone unlocks nothing; it removes one of six blockers from 77 rows, as the package framing predicted. "No uncovered SYSTEM endpoint" is not "composable": other unready bodies, a pending RunError declaration and identity may remain.

## Checker, gates and reproduction

`system_iocheck_04a9_cpu_audit.py` declares `CHECKER_ROWS=('RES_0AF8_04A9',)` and implements `--compile-only` through the same `library()` build, printing `CHECKER_BUILD_OK` with no media.
GATE_PENDING

    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/system_iocheck_04a9_cpu_audit.py --unicorn --native --output resources/iocheck-evidence.json
    python3 tools/overlay/check_checker_builds.py RES_0AF8_04A9 --require-current-main --output resources/iocheck-checker-build.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_system_iocheck_04a9_cpu.c src/recovered/system_iocheck_04a9_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test
    cmake -S . -B resources/build -DBUILD_TESTING=ON && cmake --build resources/build --target poolrad_system_iocheck_04a9_cpu_tests -j2
    ctest --test-dir resources/build -R '^poolrad_system_iocheck_04a9_cpu_tests$' --output-on-failure

Under the crew's host rule every step runs through `poolrad-heavy.sh`, one checker build at a time, after a load check. Media is read from ignored `resources/DOS/POOLRAD/`; no original or extracted bytes and no capture binaries are committed. The analysis tools need the `capstone` and `unicorn` Python modules.
