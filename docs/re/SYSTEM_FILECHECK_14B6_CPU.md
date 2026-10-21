# Turbo Pascal SYSTEM file-magic check at 0AF8:14B6: one shared raw CPU model, runtime coverage

**Runtime coverage, not credit.** This is a four-instruction Turbo Pascal SYSTEM helper. No identity or credit is claimed, and this change does not edit `RUNTIME_COVERED_ENDPOINTS`: the helper has **no far entry**, so it is not a registry endpoint
on its own; it is a dependency of endpoints. It is modelled **once, with its own evidence**, and composed by every caller (VP i20255): five call sites in four still-unmodelled endpoints plus [Close `1491`](SYSTEM_CLOSE_1491_CPU.md), which composes it
as of this change. If two callers ever need different behaviour that is a finding and this factorisation is revisited, not patched.

`pr_system_filecheck_14b6` (`src/recovered/system_filecheck_14b6_cpu.c`) models START.EXE `0AF8:[14B6,14C5)`.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| file-magic check | `0AF8:[14B6,14C5)`, loader-expanded image `[0C436,0C445)` | 15 / 4 | `24d6d6fd5079eeb043c5f3a598502fa40ba66a4e12c919717ccdf847f362d363` |

`CMP WORD ES:[DI+2],D7B3h` (7), `JE 14C4` (2), `MOV WORD [4426],0067h` (6), `RET` (1). The audit asserts exactly these four instructions, the single return (a near `RET`), no interrupt, port, call or far return, the one conditional jump, and no loader relocation.

## Contract

Entered by a **near** `CALL`: `[SP]` is the return IP. `ES:DI` is the file variable `[handle WORD][magic WORD]`; only the magic is read. When the magic is **not** `D7B3h` (the file is not open for I/O), `InOutRes` (`DS:4426`) := `0067h` (103).
Either way the flags are those of the `CMP`, so the caller's `JNE` means "magic is not `D7B3h`". The `RET` pops IP from `SS:[SP]` **after** the store, so a store that lands on the return slot (`SS == DS`, entry `SP = 4426h`) redirects the return; the model
returns that IP as it is and **the caller decides whether it expected it** (Close turns an unexpected IP into its `TRANSFER` stop). `DI+2` wraps at 16 bits inside `ES`; the word at `FFFF` straddles linearly; `DS` is the live caller value.
Exactly 1 MiB of A20-off RAM, CPU and RAM distinct host objects, `SP` in `[0000,FFFC]`, code immutable. Statuses: `RETURNED` and `ARGUMENT` (nothing changed).

## Callers (asserted)

Census method: segment-wide by construction, asserted by the checker (raw candidate list equals the decoded list, each decoded inside its entry body): **five near calls** in the SYSTEM segment, **no far entry**, **no near jump** to it:

| Call site | Body it belongs to | What the body does after the check |
| --- | --- | --- |
| `147B` | `1475` (one entry) | `INT 21h` `AH=40h` with `CX=0` |
| `1497` | `1491` Close | `INT 21h` `AH=3Eh` (modelled: [SYSTEM_CLOSE_1491_CPU.md](SYSTEM_CLOSE_1491_CPU.md)) |
| `14D7` | `14D1`, entered from `14C5` and `14CC` | untyped read/write, `AH=3Fh` / `40h` |
| `150D` | `1507`, entered from `14FB` and `1502` | typed read/write, `AH` from `BL` |
| `1569` | `1563` | `INT 21h` `AH=42h` (seek) |

These establish use. Each of those bodies must compose this model rather than re-describe the four instructions; only `1491` does so far.

## Validation

**Unicorn, 155 cases + 5 ARGUMENT**, comparing unmasked CPU14 and all 1,048,576 RAM bytes: 14 magics (including `D7B3` and every single-bit neighbour class) x two `InOutRes` values x three planted return IPs; the variable's offset at `FFFC..FFFF`; SP `0000 0001 0100 FFFB FFFC`; data segments
`0000`, `FFFF` (A20 wrap), `1234` and the code segment; an A20-aliased variable, the variable in the data segment and on the stack segment; an A20 stack; random RAM; **the return slot on `InOutRes` itself** (`SS == DS`, SP `4420..442F`, so the `MOV` redirects the `RET`); and the return slot on the magic word.
Starts **4/4**, conditional outcomes **2/2** (`JE` taken and not taken). **90** cases whose stack and variable do not alias are also checked against a separately written statement of the contract (`InOutRes := 67h` iff the magic differs, the popped IP, SP + 2, and an independently
computed CMP flag image). **Mutation check:** five deliberate changes (the compared constant, the stored value `67h` as `66h`, a byte-for-word zero test, the SP step, a hard-coded return IP) are each caught.

**Native DOSBox-X, 48 runs equal**, unmasked on what it captures: all 14 CPU words (flags included), a 256-byte stack window and the whole 64 KiB data segment (**65,792 RAM bytes captured; 982,784 bytes omitted from the native comparison**). For measured COM segment `0814` the captured physical half-open ranges are
`[17040,17140)` and `[50000,60000)`; the exact native masks are `[00000,17040)` 94,272 bytes, `[17140,50000)` 233,152 bytes and `[60000,100000)` 655,360 bytes, total 982,784. The harness does not replay DOS, BIOS, code or capture machinery as identical RAM outside its controlled windows; full RAM comparison comes
from the original-byte differential. **Declared stub:** the harness plants return IP `0100` and a `JMP FAR` at `8800:0100` captures the state there; nothing after the `RET` runs. Native cases are synthetic; aliasing, A20 wrap, offsets near `FFFF` and SP bounds rest on the Unicorn run. RESULT.BIN SHA-256
`b5e5eaddbaf62e67374c85b1341e3b46ea3bb3432f1b5ff1bc09c6ff4c51e4a8`.

**Composition regression:** after `1491` was changed to call this model instead of its own copy, the whole `1491` receipt was re-run unchanged: the same 324 cases + 5 refusals + 5 ARGUMENT, 17/17 starts, 8/8 outcomes, and the **same native `RESULT.BIN` SHA-256** (`75f81606...732b`) as before the change.

### Declared substitution: the DOSBox-X binary

The native oracle is `/usr/bin/dosbox-x`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, `DOSBox-X version 2026.01.02 SDL2`, with `core=normal`, `cputype=386`, `cycles=max`. It is **not** the pinned source build `5a9c7c53481c2e933948039c33256776653f4faf`; the receipt's
`dosbox_x` block records this. The pinned build was neither rebuilt nor re-run. This body uses only `CMP` (word), `JE`, `MOV` and a near `RET`: no shift, rotate, `MUL`, `LES` or other instruction with a known flag-convention difference. All 14 words, flags included, matched in all 48 runs. That does not prove agreement
with the pinned build on states the cases do not reach; a reviewer who needs the pinned result must re-run the native harness with that binary.

## Checker, gates and reproduction

`system_filecheck_14b6_cpu_audit.py` declares `CHECKER_ROWS=('RES_0AF8_14B6',)` and implements `--compile-only` through the same `library()` build, printing `CHECKER_BUILD_OK` with no media. (The id names the span for the build gate; it is not a registry endpoint.)
GATE_PENDING

    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/system_filecheck_14b6_cpu_audit.py --unicorn --native --output resources/filecheck-evidence.json
    python3 tools/overlay/check_checker_builds.py RES_0AF8_14B6 --require-current-main --output resources/filecheck-checker-build.json
    cc -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -I include tests/recovered/test_system_filecheck_14b6_cpu.c src/recovered/system_filecheck_14b6_cpu.c -o resources/semantic-test
    ASAN_OPTIONS=detect_leaks=1 resources/semantic-test
    cmake -S . -B resources/build -DBUILD_TESTING=ON && cmake --build resources/build --target poolrad_system_filecheck_14b6_cpu_tests poolrad_system_close_1491_cpu_tests -j2
    ctest --test-dir resources/build -R '^poolrad_system_(filecheck_14b6|close_1491)_cpu_tests$' --output-on-failure

Under the crew's host rule every step runs through `poolrad-heavy.sh`, one checker build at a time, after a load check. Media is read from ignored `resources/DOS/POOLRAD/`; no original or extracted bytes and no capture binaries are committed. The analysis tools need the `capstone` and `unicorn` Python modules.
