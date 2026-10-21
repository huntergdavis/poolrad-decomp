# Declared boundary: `RES_0AF8_00E2` (Turbo Pascal `RunError`, process exit with a runtime-error message)

**Status: PROPOSED declared boundary contract (text only; par-e lands the registration). USABLE as a dependency, NEVER a credit, and TERMINAL: it never returns.** It is never in
`strict_full_c_addresses.csv` or `reviewed_function_identities.csv`; the queue loader refuses that.

It is a **separate entry from [`RES_0AF8_00E9`](DECLARED_BOUNDARY_0AF8_00E9.md) (Halt)**, by the VP's ruling (i20243), because the two entry points behave differently: through `00E9` the runtime-error message is not printed, through `00E2` it is. Folding them would make one
document claim something true of neither. `00E2` was found while covering [IOCheck `04A9`](SYSTEM_IOCHECK_04A9_CPU.md), whose non-zero path is `JMP 00E2`; the Halt declaration expressly excludes this entry ("the RunError entry (+00E6/+00E0) and the runtime-error message path" are NOT modelled).

## Extent

| Piece | Span | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| entry | `0AF8:[00E2,00E6)`, loader-expanded image `[0B062,0B066)` | 4 | `e2ff41187088c0654f5e48c1714af6a71859757a7185c54e7394181a109057d1` |
| shared tail, from `JMP 00ED` to the print helpers | `0AF8:[00ED,01EE)`, image `[0B06D,0B16E)` | 257 | `8a92e5adb370154bde79926e5d3f20f189f4dcdd015d4d783d0f683583e01c0d` |
| data read by the tail | `0AF8:[01EE,0222)` (the 19-vector table, the message strings) | 52 | `961ee5d0f2c12a0c94237230a4824ec721f3a780fb88e1e927e42b284f334cc9` |

The entry is `POP CX; POP BX; JMP 00ED`. The tail `[00ED,01A5)` is the **same code as the Halt boundary** (188 B less its first 4 bytes), and `[01A5,01EE)` are the local print helpers the Halt document lists as inside its boundary. Neighbouring entries
`00DC` (`PUSH ES; JMP SHORT 00E2`) and `00DF` (`MOV AX,00C8h`, falling into `00E2`) feed this entry and are **not** part of this declaration. There is no return instruction on any path from `00E2` except the ExitProc dispatch `RETF` at `013F` (below).

## What it is (asserted by `tools/overlay/declared_boundary_0af8_00e2_audit.py` from the expanded image)

1. Entry `00E2`: **`AX` = the runtime error code**. `POP CX; POP BX` takes the top of the stack, the far return address of whatever called or jumped here, as `ErrorAddr` (`CX` = offset, `BX` = segment), then `JMP 00ED`.
   For a `JMP 00E2` (IOCheck at `04B4`) the top of the stack is IOCheck's own return address, so `ErrorAddr` names the caller of the failing I/O operation; for a far call it names the instruction after the call.
2. `00ED`: `DS := 0C7Ch` (SYSTEM data), `STI`, `ExitCode [441C] := AX`.
3. `ErrorAddr` is normalised through the overlay list at `[43FA]` and then made program-relative (`BX := BX - [4422] - 10h`), then stored to `[441E]` (offset) and `[4420]` (segment). This **does** happen via `00E2`, because `CX:BX` is non-nil.
4. The ExitProc chain (`+0123`) as in Halt: `PUSH CS:0123; PUSH ES:BX; RETF` is a far jump into the exit procedure, which returns to `0123`. **This is the only `RETF` reachable, and it is not a return to the caller.**
5. `Close(Input)` and `Close(Output)` (near calls to `05C1` with `DS:8508` and `DS:8608`), then the 19 saved interrupt vectors are restored with `INT 21h` **`AH = 25h`** (the loop at `0155..016A`; the `INT` at `0164`).
6. **The difference from Halt:** at `016C` `ErrorAddr` is tested (`MOV AX,[441E]; OR AX,[4420]; JE 019E`). Through `00E9` it is nil (`XOR CX,CX; XOR BX,BX`), so the message is skipped. Through `00E2` it is not, so the routine prints
   `Runtime error ` + the decimal `ExitCode` + ` at ` + the hexadecimal segment `:` offset + `.` CR LF, one character at a time through `INT 21h` **`AH = 06h`** (helpers `01A5`, `01B3`, `01CD`, `01E7`; the `INT` at `01EB`).
7. `MOV AX,[441C]; MOV AH,4Ch; INT 21h`: terminate with the exit code. **No path returns to the caller.**

The only interrupts in the extent are `INT 21h` at `0164` (`AH=25h`), `01A3` (`AH=4Ch`) and `01EB` (`AH=06h`); the audit asserts each `AH`. A walk over every path from `00E2` (80 instructions; calls treated as returning to the flow; the terminate `INT` treated as the end) finds **exactly one**
`RETF`, the ExitProc dispatch frame above, and no other return.

## Who transfers here

Census method: segment-wide by construction, asserted by the audit (raw candidate list equals the expected list): **nine near `JMP`s** in the SYSTEM segment (`0251`, `026B`, `04B4` (IOCheck), `04DC`, `04F4`, `0AE1`, `11B1`, `11B7`, `11BD`), **one short `JMP`** (`00DD`, the `00DC` entry), **no near `CALL`**, **no direct overlay far
call**, and **one resident far call**, in the resident function at image `AA20` (`PUSH BP; MOV BP,SP; MOV AX,00D1h; LCALL 0AF8:00E2`, i.e. `RunError(209)`). The other near-jump sites are SYSTEM runtime checks that were not individually identified here. So every row whose closure reaches one of those checks reaches this entry.

## How a composing model uses it

A path that reaches `00E2` stops there with a named **EXIT(RUNERROR)** stop. The stop carries the CPU and RAM at the entry (the return address on the stack, `AX` = the error code). Nothing after it is modelled, because nothing after it executes. IOCheck's `RUNERROR` stop is exactly this
state: IP `00E2`, `AX = InOutRes`, SP unchanged. A model must **not** treat this boundary as returning; the instructions after a `CALL` or `JMP` that reaches it are dead code. The differential for such a caller hooks the transfer or the first instruction at `0AF8:00E2` and compares state there.

## NOT modelled

- The ExitProc chain and every procedure it would call (inside the boundary either way).
- Text-file close, vector restoration, the message text on the screen, and the DOS terminate itself.
- Which error codes the callers pass, and what they mean.
- Any DOS-side effect of process exit.

## Limits

- Composable, never credited, terminal.
- A composing caller records the EXIT(RUNERROR) stop and this boundary in its own evidence document.
- Like the Halt declaration, "no ExitProc installer" is not needed: the chain is inside the boundary either way.
- The entry's neighbours `00DC` and `00DF` and the `00E6` Halt-255 form are outside this declaration.

## Reproduction

    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/declared_boundary_0af8_00e2_audit.py --output resources/00e2-audit.json
    python3 tools/overlay/check_checker_builds.py RES_0AF8_00E2 --require-current-main

`--compile-only` imports the tools and prints `CHECKER_BUILD_OK` with no media. Media is read from ignored `resources/DOS/POOLRAD/`; nothing derived is committed.

## Proposed declaration (TEXT ONLY; the acceptance lands it)

    strict_candidate_queue.DECLARED_BOUNDARY_ENDPOINTS:  "RES_0AF8_00E2": "docs/re/DECLARED_BOUNDARY_0AF8_00E2.md",

## Reviewer record (added by the review lane, not the author)

**Registered** by par-e at main `804a6162` as `"RES_0AF8_00E2": "docs/re/DECLARED_BOUNDARY_0AF8_00E2.md"` in
`DECLARED_BOUNDARY_ENDPOINTS`. It lands in the same commit as IOCheck `04A9`'s coverage row, whose `RUNERROR` stop is
`JMP 00E2` (VP i20256/i20258). `RUNTIME_COVERED_TERMINALS` now makes `load_queue` refuse a covered `04A9` while this
boundary is undeclared, and a test pins that refusal.

- **Static audit reproduced.** The entry, tail and data SHA-256 values equal the table above. The interrupts are
  `0164` AH=25h, `01A3` AH=4Ch and `01EB` AH=06h. 80 instructions are reachable, the one `RETF` is `013F`, and the
  census of nine near `JMP`s, the `00DD` short `JMP` and the one resident far call at image `AA20` is the same.
- **Independent walk that ENTERS the calls** (the audit's walk treats them as returning). The walk follows every near
  call, treats `PUSH CS; CALL` as a far frame, and stops only at the `AH=4Ch` `INT`. It reaches about 150
  instructions, including `Close` at `05C1` and the print helpers. Its exits are exactly the ExitProc dispatch `RETF`
  at `013F`, the terminate `INT` at `01A3`, and **one indirect far call that no static walk can enter**:
  `CALL FAR ES:[BX+DI]` at `0601`.
  - That call is in the helper at `05FD`, called from `Close(var Text)` with `BX = 14h` and `BX = 1Ch`, so it reaches
    the `FlushFunc` and `CloseFunc` of the Input and Output `TextRec`.
  - Those pointers are installed at run time: the SYSTEM text driver, or CRT's driver if `AssignCrt` ran.
  - **Stated limit:** "never returns to the caller" holds over the SYSTEM code. It also rests on the installed text
    driver functions and the ExitProc procedures returning normally; neither can be walked statically. The Halt
    declaration ([00E9](DECLARED_BOUNDARY_0AF8_00E9.md)) shares this tail and does not name the indirect call
    either. That does not change its terminal use, because composition stops at the entry.
- **Use:** composable, never credited, terminal. A composing caller stops at `0AF8:00E2` with `EXIT(RUNERROR)`.
