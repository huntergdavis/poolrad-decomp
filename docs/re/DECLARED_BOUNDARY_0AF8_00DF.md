# Declared boundary: `RES_0AF8_00DF` (Turbo Pascal divide-error handler, INT 0; runtime error 200; TERMINAL)

**Status: PROPOSED declared boundary contract (text only; par-e lands the registration). USABLE as a dependency, NEVER a credit, and TERMINAL: it never returns.** It is never in `strict_full_c_addresses.csv` or `reviewed_function_identities.csv`; the queue loader refuses that.

It is a **separate entry from [`RES_0AF8_00E2`](DECLARED_BOUNDARY_0AF8_00E2.md) (RunError)**: that declaration expressly excludes `00DF` ("the entry's neighbours `00DC` and `00DF` ... are outside this declaration"). `00DF` is what the CPU reaches when a `DIV` or `IDIV` faults, so a model with a faulting divide
(BlockRead's `DIV WORD ES:[DI+4]` at `0AF8:1531`, per the file-I/O package) stops here, and until this entry is declared its callers would compose into an undeclared terminal (VP i20285).

## THE PREMISE: this declaration rests on INT 0's vector still being SYSTEM's handler at run time

**Read this before relying on the declaration.** `00DF` is reached only through the interrupt vector: nothing calls or jumps to it (asserted). The vector is installed **once, at SYSTEM init** (`005C PUSH DS; PUSH CS; POP DS; 005F MOV DX,00DFh; 0062 MOV AX,2500h; 0065 INT 21h`, i.e. `INT 0 := CS:00DF`, with the
original saved by the `AH=35h` loop at `004E` and restored by the Halt loop at `015F`). **Anything that re-vectors INT 0 after init leaves SYSTEM, and this boundary does not describe it.** Other code installs vectors of its own (the `08AE` driver installs INT `08h`, others INT `1Bh` and `3Fh`), and a terminal claim whose
premise can be falsified at run time must name the premise. What the static evidence does and does not show:

- **Shown (asserted by the checker):** in the loader-expanded START image there are exactly eight `MOV AX,25xxh` immediates (`AL` = `08`, `08`, `1B`, `3F` outside SYSTEM; `00`, `23`, `24`, `3F` in SYSTEM's init) and one `MOV AH,25h` (the Halt restore loop, `AL` from the `01EE` table); **exactly one of them is `AL = 00`, SYSTEM's own**. GAME.OVR contains **no**
  `AH=25h` by immediate at all. No near, short or far transfer reaches `00DF`, and `MOV DX,00DFh` appears only at the install.
- **NOT shown, so part of the premise:** a computed `AL`, an `INT 21h` `AH=25h` reached through `Dos.Intr(21h, Regs)` (`0A32:000B`, whose register record is built at run time), a direct write to `0000:0000..0003`, DOS itself, a resident program or a debugger. The static scan cannot exclude them. If INT 0 is re-vectored, a composing model's EXIT(DIVIDE) stop describes a handler
  that is not running.

## Extent

| Piece | Span | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| entry | `0AF8:[00DF,00E2)`, loader-expanded image `[0B05F,0B062)` | 3 | `f5643d94e9e1bcc2aa33ced5f7eb419bf3551c9bea99e5fbcc02afb353edf860` |
| install | `0AF8:[005C,0067)`, image `[0AFDC,0AFE7)` | 11 | `431bbe40c73a086809752f5d4ffe551c1020383c911c6cfa2aed3a9de6ca80aa` |

The entry is `MOV AX,00C8h` and **falls straight into `00E2`** (`POP CX; POP BX; JMP 00ED`). Everything from `00E2` on, including the ExitProc chain, the vector restore, the message and the terminate, is the [`RES_0AF8_00E2`](DECLARED_BOUNDARY_0AF8_00E2.md) boundary; this entry adds only `AX := 200`. The checker
re-runs the whole `00E2` audit first, so any drift in that declaration fails this one too.

## What it is (asserted from the expanded image)

1. A divide error (`DIV`/`IDIV` with a zero divisor or a quotient that does not fit) makes the CPU push `FLAGS`, `CS` and `IP` (in that order, so `[SP]` = IP, `[SP+2]` = CS, `[SP+4]` = FLAGS), clear `IF` and `TF`, and load `CS:IP` from `0000:0000`, which is `0AF8:00DF`. The pushed `IP` is the faulting instruction's on the 80186 and later and the
   following instruction's on an 8086/8088: **whatever the CPU pushed becomes `ErrorAddr`**.
2. `00DF`: `MOV AX,00C8h`: the runtime error code **200** ("Division by zero").
3. `00E2`: `POP CX` (= IP), `POP BX` (= CS), `JMP 00ED`: `ErrorAddr := IP:CS` of the fault. The `FLAGS` word stays on the stack, unused: the process ends.
4. From `00ED` the path is the `00E2` boundary's: `DS := 0C7Ch`, `STI`, `ExitCode [441C] := AX = 200`, `ErrorAddr` normalisation, the ExitProc chain (its one `RETF` is the dispatch to the exit procedure, not a return to the faulting code), `Close(Input/Output)`, the 19-vector restore (`INT 21h` `AH=25h`, which puts INT 0 back), the
   `Runtime error 200 at SSSS:OOOO.` message through `INT 21h` `AH=06h`, and the terminate `INT 21h` `AH=4Ch` with exit code 200.

The handler reads no register other than `AX` (set) and the two popped words, and `DS` is loaded before any data access, so it does not depend on the state of the program at the fault. **No path returns to the faulting code**: the walk over the continuation (80 instructions) finds exactly one `RETF`, the ExitProc dispatch.

## Who reaches it

Only the CPU, through the vector: the checker asserts that there is **no near, short or far transfer** to `0AF8:00DF` anywhere in the SYSTEM segment or the START image, and that the offset `00DFh` is loaded once (the install). Every `DIV`/`IDIV` that can fault in code running with SYSTEM's vector in place reaches this entry; which ones can fault is each composing model's evidence.

## How a composing model uses it

A body with a `DIV`/`IDIV` stops at that instruction with a named **EXIT(DIVIDE)** stop when the divisor is zero or the quotient overflows. The stop carries the CPU and RAM **as the CPU leaves them for the handler**: `FLAGS`, `CS` and `IP` pushed (`SP` lowered by 6), `IF` and `TF` clear, `CS:IP = 0AF8:00DF`. Nothing after it is modelled, because nothing after it
executes; the instructions after the `DIV` are dead code on that path. A model must **not** treat this boundary as returning. The differential hooks the fault, or the first instruction at `0AF8:00DF`, and compares state there; the stop is only as valid as the premise above.

## NOT modelled

- The ExitProc chain and every procedure it would call, text-file close, vector restoration, the message on the screen and the DOS terminate (all inside the `00E2` boundary).
- Whether a given `DIV` can fault (that is the composing body's evidence), and the 8086/8088 versus 80186+ `IP` difference.
- Any re-vectoring of INT 0 after init (the premise).

## Limits

- Composable, never credited, terminal, **conditional on the premise**.
- A composing caller records the EXIT(DIVIDE) stop, this boundary, and the premise in its own evidence document.
- The `00DC`/`00E6` entries remain outside this declaration.

## Reproduction

    PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/declared_boundary_0af8_00df_audit.py --output resources/00df-audit.json
    python3 tools/overlay/check_checker_builds.py RES_0AF8_00DF --require-current-main

`--compile-only` imports the tools and prints `CHECKER_BUILD_OK` with no media. Media is read from ignored `resources/DOS/POOLRAD/`; nothing derived is committed.

## Proposed declaration (TEXT ONLY; the acceptance lands it)

    strict_candidate_queue.DECLARED_BOUNDARY_ENDPOINTS:  "RES_0AF8_00DF": "docs/re/DECLARED_BOUNDARY_0AF8_00DF.md",

## Reviewer record (added by the review lane, not the author)

(pending)
