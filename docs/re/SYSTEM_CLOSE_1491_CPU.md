# Turbo Pascal SYSTEM Close(var f) at 0AF8:1491 (+ check helper 14B6): raw CPU, runtime coverage, declared INT 21h AH=3Eh

**Runtime coverage, not credit.** This is a Turbo Pascal SYSTEM library routine (the typed/untyped file `Close`). No identity or credit is claimed, and this change does not edit
`RUNTIME_COVERED_ENDPOINTS`; adding `(0x0AF8, 0x1491)` is the accepting review's edit. It is the second of the six SYSTEM file-I/O endpoints of the second keystone (t29), on the
declared-service pattern of [GetTime](DOS_GETTIME_00A2_CPU.md) and [Intr](DOS_INTR_000B_CPU.md); the first, [IOCheck 04A9](SYSTEM_IOCHECK_04A9_CPU.md), is merged.

`pr_system_close_1491` (`src/recovered/system_close_1491_cpu.c`) models START.EXE `0AF8:[1491,14B6)` and its near check helper `0AF8:[14B6,14C5)`.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| Close + helper | `0AF8:[1491,14C5)`, loader-expanded image `[0C411,0C445)` | 52 (37 + 15) / 17 (13 + 4) | `83ba3ec7cf3aabfd785f101768657a686f1915e65f5895a82be1d91030dc8858` |

The audit asserts the 17 instructions exactly, the two returns (`RETF 4` at `14B3`, the helper's `RET` at `14C4`), the single `INT 21h` at `14A6` and that `AH` is set only by `MOV AH,3Eh` at `14A4` immediately
before it, no port or far transfer, the four conditional jumps, and no loader relocation. The helper `14B6` is shared by five call sites (this one at `1497` and `147B`, `14D7`, `150D`, `1569`), so it has its own model and evidence: [SYSTEM_FILECHECK_14B6_CPU.md](SYSTEM_FILECHECK_14B6_CPU.md). **This model composes that one** (`pr_system_filecheck_14b6`) instead of re-describing the four instructions; its receipt was re-run unchanged after the change.

## Behaviour

The file variable is `[handle WORD][magic WORD]` (and more, beyond what this routine touches). Entry is a far call with `[SP+4]` = the variable's offset and `[SP+6]` its segment.

1. `MOV BX,SP; LES DI,SS:[BX+4]`.
2. `CALL 14B6`: the helper compares `ES:[DI+2]` with `D7B3h`. When it **differs**, `InOutRes` (`DS:4426`) := `0067h` (103, "file not open") and the helper returns with ZF clear. The `CALL` pushes `149Ah` at `SS:[SP-2]`
   and the helper's `RET` rereads it.
3. Magic **not** `D7B3h`: `RETF 4`, the magic unchanged, **no DOS call**. (So a closed, input or output file also gets 103 here: only `D7B3h` is accepted.)
4. Otherwise `BX := ES:[DI]` (the handle). If `BX` is **above 4 (unsigned)**: `AH := 3Eh` and `INT 21h`; when CF is set `InOutRes := AX`. **Handles 0..4 (the DOS standard handles) are never closed.**
5. **Then, whatever the DOS result, `ES:[DI+2] := D7B0h` (closed).** This is the clause a model gets wrong: the magic is set even when DOS fails (CF set), and even for handles 0..4 that were never closed.
6. `RETF 4`.

### The two gotchas, as test and differential facts

| Gotcha | How it is exercised |
| --- | --- |
| `INT 21h 3Eh` only when handle `> 4` | C test: handles 0..7, `0FFFFh`, `8000h`, `00FFh` (unsigned compare, `4` does not call, `5` does); differential: all eleven handles; one mutant (`JBE` as `JB`) is caught |
| magic := `D7B0h` regardless of DOS | C test: DOS error (CF, AX = 6) gives `InOutRes = 6` **and** magic `D7B0h`; differential: 96 INT cases incl. CF set; the mutant that skips the magic store on error is caught |

## Statuses and the declared service

`RETURNED` (`RETF 4` ran), `ARGUMENT` (nothing changed), `SERVICE` (the service refused or returned a non-continuation; stopped at the INT, IP `14A6`, `AH = 3Eh`, `BX` = the handle, **nothing after it applied**) and
`TRANSFER` (the helper's near `RET` popped an IP other than `149Ah`: its return slot at `SS:[SP-2]` was overwritten, e.g. by `MOV [4426],67h` when `SS == DS` and entry `SP = 4428h`; the model stops there rather than guess).

**INT 21h AH=3Eh is a DECLARED environment input, and the only AH this body issues (fail-closed AH list: `{3Eh}`).** The service receives the CPU at the INT and returns the complete CPU at the continuation `14A8`. DOS
preserves every register but `AX` and changes only `CF` in the flags: the model refuses (SERVICE) a result whose `CS`, `IP`, `SS`, `SP`, `BP`, `BX`, `CX`, `DX`, `SI`, `DI`, `DS` or `ES` differs from the INT-time value, whose
`IP` is not `14A8`, or whose flags differ in any bit but CF. **What the service may write:** `AX` (the DOS error code when CF is set) and `CF`, nothing else, and **it must not write RAM**. DOS's own interrupt frame
and stack use below `SP` are outside this model (stated limit; the native comparison excludes them, below). Every operand is reread from RAM in source order, so a variable that overlaps `InOutRes` or the stack is followed as written.

Profile: exactly 1 MiB of A20-off RAM, `SP` in `[0002,FFF8]` (the helper's slot and the 8-byte frame never wrap), code immutable, offsets wrap at 16 bits inside a segment (the magic word's offset `DI+2` wraps at `FFFF`, the
handle word at `DI = FFFF` straddles linearly), physical addresses at 1 MiB.

## Callers

Census method: segment-wide by construction, asserted by the checker (raw candidate list equals the expected decoded list): **one near call** (`+1442`, inside the shared Reset/Rewrite body at `141F`, which closes an already
open file before reopening it), **nine direct overlay far calls** (units `0036` at `05E5`; `006F` at `08D1`, `09C6`, `0AB9`, `0C4F`, `0D8A`, `0E88`, `18E5`, `20BF`) and **four resident far calls** decoded to three resident
functions: image `4354` (`+43CA`; the same function that calls `017B`), image `830B` (`834A`) and image `8707` (`870F`, `871E`). These establish use, not identity.

## Validation

Receipt: 2026-10-20/21, main `e7362e73` plus this branch.

**Unicorn, 324 cases + 5 service refusals + 5 ARGUMENT**, comparing unmasked CPU14, all 1,048,576 RAM bytes, the stop status and the number of `INT 21h` issued (96; Unicorn's interrupt hook plays the service with the case's `AX` and `CF`): `RETURN` 321, `TRANSFER` 3.
Starts **17/17**, conditional outcomes **8/8**, accounted by successor. **226** cases whose stack and variable do not alias are also checked against a separately written statement of the contract. Cases: 11 handles x 7 magics x 4 service answers x 2
`InOutRes` values; SP `0002 0003 0100 FFF8 FFF7`; the variable's offset at `FFFC..FFFF` (wrap and straddle); data segments `0000`, `FFFF`, `1234` and the code segment; an A20 stack and variable; a variable on the stack; random RAM; the variable on `InOutRes`
itself (handle or magic word); and `SS == DS` with SP `4420..4435` (the return slot, the frame and `InOutRes` alias). The five refusals (refusal, wrong continuation IP, changed `BX`, changed `ZF`, changed `ES`) each stop at the INT with the magic and `InOutRes` untouched.

**Mutation check:** six deliberate changes are each caught: magic not set on a DOS error (`goto` out of the CF branch), `JBE` as `JB` (handle 4 would call DOS), `InOutRes` 67h as 66h, the CF test inverted, the magic stored as `D7B1h`, and the continuation check ignoring flags.

**Native DOSBox-X, 48 runs equal** (`INT` path 18, no-`INT` path 30), unmasked on what it captures: all 14 CPU words (flags included), a 256-byte stack window and the whole 64 KiB data segment (**65,792 RAM bytes captured; 982,784 bytes omitted from the native comparison**). For measured COM segment `0814` the captured physical half-open
ranges are `[17040,17140)` and `[50000,60000)`; the exact native masks are `[00000,17040)` 94,272 bytes, `[17140,50000)` 233,152 bytes and `[60000,100000)` 655,360 bytes, total 982,784. The harness does not replay DOS, BIOS, code or capture machinery as identical RAM outside its controlled windows; full RAM
comparison comes from the original-byte differential.

- **Declared stub:** a COM installs an `INT 21h` hook that answers `AH=3Eh` with the case's `AX` and `CF` (by editing the stacked flags) and returns by `IRET`; every other function chains to DOS. DOS itself is never asked to close anything.
- **DOS-side stack bytes excluded:** in the 18 `INT` runs the 8 bytes below the INT-time `SP` (the 3-word INT frame and the stub's saved `BP`) are DOS-side and are taken from the native record, the stated limit above. The no-`INT` runs compare that region exactly (the helper's `149Ah` slot).
- Native cases are synthetic frames and data. Stack-in-data aliasing, the A20-wrapped data segment, variable offsets near `FFFF`, `TRANSFER` and the SP bounds rest on the Unicorn run.
- RESULT.BIN SHA-256 `75f81606fca976a1981acbbcb7d4c3099c70218d752ec2003e6508ef6609732b`.

### Declared substitution: the DOSBox-X binary

The native oracle is `/usr/bin/dosbox-x`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, `DOSBox-X version 2026.01.02 SDL2`, with `core=normal`, `cputype=386`, `cycles=max`. It is **not** the pinned source build `5a9c7c53481c2e933948039c33256776653f4faf`; the receipt's `dosbox_x` block records
this. The pinned build was neither rebuilt nor re-run. This body uses only `MOV`, `LES`, `CMP` (word), `JNE/JBE/JAE/JE`, near `CALL`/`RET`, `INT 21h` and `RETF 4`: no shift, rotate or `MUL`, and the SP bounds exclude stack-offset wrap. `LES` is used, but only through `SS:[BX+4]` with the SP bound keeping the pointer below offset `FFFC`, where the
pinned build's `LES` wrap difference does not apply. All 14 words, flags included, matched in all 48 runs. That does not prove agreement with the pinned build on states the cases do not reach; a reviewer who needs the pinned result must re-run the native harness with that binary.

## Unlocked-row delta (a delta, never a credit)

Measured with `price_row_closures.Pricer` over all 440 queue rows. Each variant regenerates the queue export on the same tree with the endpoints added to the runtime-covered set **in a scratch run only** (no registry edit), because rows that call an endpoint directly carry their readiness in the export:

| Covered in the scratch run | Rows whose uncovered SYSTEM set changes | Rows left with no uncovered SYSTEM endpoint |
| --- | ---: | ---: |
| `04A9` (merged, #1306) | 77 | 0 |
| `1491` (this change) | 82 | 0 |
| `04A9` + `1491` | 82 | 0 |
| all six file-I/O endpoints (`1410 1491 14FB 1563 04A9 1593`) | 82 | **17** |

So `1491` alone unlocks nothing; the package unlocks 17. **Correction:** the merged `04A9` document's table gave 78 changed rows for `04A9` + `1491`; that figure was measured against a stale export (it did not regenerate readiness for direct callers) and is corrected to 82 in the same change. "No uncovered SYSTEM endpoint" is not "composable": other unready bodies, the pending RunError declaration and identity may remain.

## Checker, gates and reproduction

`system_close_1491_cpu_audit.py` declares `CHECKER_ROWS=('RES_0AF8_1491',)` and implements `--compile-only` through the same `library()` build, printing `CHECKER_BUILD_OK` with no media.

Gate results on current main (`e7362e73`): `check_checker_builds.py RES_0AF8_1491 --require-current-main` PASS (1 actual checker build for 1 row); ASan/UBSan C test 93 checks, exit 0; focused CTest `poolrad_system_close_1491_cpu_tests` 1/1 passed; combined receipt
`--unicorn --native`: 324 cases + 5 service refusals + 5 ARGUMENT, 17/17 starts, 8/8 outcomes, 226 reference-checked, 0 masked RAM bytes (Unicorn); native 48 runs, RESULT.BIN `75f81606...732b`, 982,784 bytes omitted. The pairing gate is run on the committed change.


    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/system_close_1491_cpu_audit.py --unicorn --native --output resources/close-evidence.json
    python3 tools/overlay/check_checker_builds.py RES_0AF8_1491 --require-current-main --output resources/close-checker-build.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_system_close_1491_cpu.c src/recovered/system_close_1491_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test
    cmake -S . -B resources/build -DBUILD_TESTING=ON && cmake --build resources/build --target poolrad_system_close_1491_cpu_tests -j2
    ctest --test-dir resources/build -R '^poolrad_system_close_1491_cpu_tests$' --output-on-failure

Under the crew's host rule every step runs through `poolrad-heavy.sh`, one checker build at a time, after a load check. Media is read from ignored `resources/DOS/POOLRAD/`; no original or extracted bytes and no capture binaries are committed. The analysis tools need the `capstone` and `unicorn` Python modules.
