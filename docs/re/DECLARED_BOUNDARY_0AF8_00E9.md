# Declared boundary: `RES_0AF8_00E9` (Turbo Pascal `Halt`, process exit)

**Status: declared boundary contract. USABLE as a dependency, NEVER a credit, and TERMINAL: it never
returns.** It is never in `strict_full_c_addresses.csv` or `reviewed_function_identities.csv`; the queue
loader refuses that, and `test_strict_candidate_queue` proves it.

This is a prerequisite of the funded `RES_0899_0059` / `RES_0899_0000` wall (VP i20005). `0899:0000` (the
game's quit routine) ends `XOR AX,AX; LCALL 0AF8:00E9`, which is `Halt(0)`.

## Extent

`0AF8:[00E9,01A5)`, **188 bytes**, from the entry to the end of `INT 21h` at +01A3. Loader-expanded image
`[0B069,0B125)`, SHA-256 `7f0775420aecaecfa60b0b0ce15631a71c6755f368f04b4c5787610f4770a4e5`.

There is no return instruction in this span; the routine ends in `INT 21h` with AH=4Ch. The local helpers
at +01A5…+01ED and the text-file close at +05C1 are reached only by near `CALL` and are inside the boundary.

## What it is (traced instruction by instruction from the expanded image)

1. Entry +00E9: `XOR CX,CX; XOR BX,BX`, which sets ErrorAddr = nil. Then `DS := 0C7Ch` (SYSTEM data),
   `STI`, and `ExitCode [441C] := AX`.

   The +00E6 entry is the RunError form: `AX := FFh`, then +00E0 jumps past this. It is not part of this
   declaration.
2. ErrorAddr normalisation through the overlay list `[43FA]` when CX:BX is not nil. This does not happen
   via +00E9.
3. **ExitProc chain** (+0123): if `ExitProc [4418]:[441A]` is non-nil, Halt clears it and `InOutRes [4426]`.
   It then pushes `CS:0123` and `RETF`s into the exit procedure, which returns to +0123 to test again.
   **This runs whatever exit procedures are installed, and that code is inside this boundary.**

   A byte scan of the expanded START.EXE image and GAME.OVR, for MOV/LES forms addressing `[4418]` or
   `[441A]`, finds only this routine reading and clearing ExitProc. No installer was found. This is a
   static scan; an installer that reached the variable through a pointer would not be seen.
4. `Close(Input)` and `Close(Output)`: near calls to +05C1 with `DS:8508` and `DS:8608`.
5. It restores 19 saved interrupt vectors with `INT 21h` AH=25h, from the table at `[8708]`. The vector
   numbers come from the CS table at +01EE: `00 02 1B 21 23 24 34–3F 75`.
6. If ErrorAddr is non-nil, it prints `Runtime error NNN at SSSS:OOOO.` through `INT 21h` AH=06h. This does
   not happen via +00E9.
7. `MOV AX,[441C]; MOV AH,4Ch; INT 21h`: terminate with the exit code. **No path returns to the caller.**

## How a composing model uses it

A caller whose path reaches `LCALL 0AF8:00E9` stops there with a named **EXIT** stop. The stop carries the
CPU and RAM at the call (the far return address pushed, AX = the exit code). Nothing after the call is
modelled, because nothing after it executes.

In `0899:0000` the `POP BP; RETF` after `LCALL 0AF8:00E9` (+0031/+0032) is therefore **dead code**: no
execution reaches it. It is not "unreachable but possible". Key 03 in `0899:0059` (`PUSH CS; CALL 0000`)
likewise never returns. par-a recorded this under the existing CALLEE_NONRETURNING contract.

A model must **not** treat this boundary as returning. Any row whose closure reaches `0899:0000` has a stop
on that path that is a process exit (VP i20005).

The differential for such a caller hooks the `LCALL` target, or the first instruction at `0AF8:00E9`, and
compares state at that point in both oracles.

## NOT modelled

- The ExitProc chain and every procedure it would call.
- Text-file close, vector restoration and the DOS terminate itself.
- Any DOS-side effect of process exit.
- The RunError entry (+00E6/+00E0) and the runtime-error message path.

## Limits

- Composable, never credited, terminal.
- A composing caller records the EXIT stop and this boundary in its own evidence document.
- The "no ExitProc installer" observation comes from the static byte scan described above. It is not
  needed for the declaration: the chain is inside the boundary either way.

## Proposed declaration (TEXT ONLY; the acceptance lands it)

    strict_candidate_queue.DECLARED_BOUNDARY_ENDPOINTS:  "RES_0AF8_00E9": "docs/re/DECLARED_BOUNDARY_0AF8_00E9.md",

## Reviewer record (added by the review lane, not the author)

Checked by par-e from the loader-expanded image on main `7cf02cd7` (2026-10-20):
- the extent `0AF8:[00E9,01A5)` is 188 B, image `[0B069,0B125)`, SHA-256 `7f077542…a4e5`;
- the entry is `XOR CX,CX; XOR BX,BX; MOV DX,0C7Ch; MOV DS,DX; STI`;
- the span ends `MOV AX,[441C]; MOV AH,4Ch; INT 21h` at +01A3;
- the vector-number table at +01EE is `00 02 1B 21 23 24 34–3F 75` (19 bytes).

**Correction to "There is no return instruction in this span":** there is one, a `RETF` at +013F. It is the ExitProc
dispatch from step 3 (`MOV AX,0123h; PUSH CS; PUSH AX; PUSH ES; PUSH BX; RETF`): a far jump into the exit procedure,
which returns to +0123. It is not a return to Halt's caller. The behavioural claim stands: no path returns to the
caller, and the boundary is terminal.
