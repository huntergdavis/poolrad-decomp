# Turbo Pascal SYSTEM Seek at 0AF8:1563: raw CPU, runtime coverage (t29)

**Runtime coverage, not credit**, and no identity claim. This is the file-I/O package's Seek endpoint (VP t29). On par-a's
440-row export (main `04e5bc7b`), 77 rows have `0AF8:1563` in their closure. Per t29, unlocked rows are measured **after**
the coverage line lands and reported as a **delta**, never as a credit.

`pr_system_seek_1563` (`src/recovered/system_seek_1563_cpu.c`) models START.EXE `0AF8:[1563,1593)` and the shared open-check
helper `0AF8:[14B6,14C5)`.

## Media spans (and a correction to my sizing)

| Piece | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| Seek | `0AF8:[1563,1593)`, image `[0C4E3,0C513)` | **48** / 19 | `598dcb76bff3948d01eb4b14d25969c11f736d4221438ffb29d9d06c9774a4af` |
| open check | `0AF8:[14B6,14C5)`, image `[0C436,0C445)` | 15 / 4 | `24d6d6fd5079eeb043c5f3a598502fa40ba66a4e12c919717ccdf847f362d363` |

My t29 sizing note (i20227) said 45 B / 18 instructions; the `RETF 8` actually ends at +1593, giving 48 B / 19.

The audit asserts:

- the single `INT 21h` (+1588);
- the two `MUL`s (+1571, +157A);
- the three conditional sites.

## Behaviour, with the fact a plausible model gets wrong (VP i20228)

1. `LES DI,f`, then `CALL 14B6`: unless the magic f+2 is D7B3h (open), InOutRes `DS:[4426]` := **103** and return.
2. **The position is n·recsize truncated to 32 bits, built from two 16-bit MULs:**
   - `CX` := the **low word** of n_high × recsize. **Its high word is dropped.**
   - `DX:AX` := n_low × recsize.
   - `CX := CX + DX`, with no carry beyond 16 bits.
   - `DX := AX`.

   The result is the 32-bit truncation of the product. A 64-bit (or even 48-bit) product would be wrong.
3. `INT 21h` AX=4200h (LSEEK from the start), BX = the handle f+0, CX:DX = the position.
4. On carry, InOutRes := AX. Then `POP BP; RETF 8`.

The MULs' SF/ZF/AF/PF are architecturally undefined. They are overwritten by `ADD CX,DX` before anything reads them, and the
model follows DOSBox-X's 386 core for them.

## Contract and stops (the 137C conditions)

- **Fail-closed AH list:** only 42h. Any other AH is **ENV**, unreachable here because AX is the constant 4200h. The Unicorn
  hook refuses any other interrupt or AH.
- **Continuation check:** the result must be at 158Ah with the same CS, SS, SP and BP; otherwise **SERVICE**.
- **What the service may write:** nothing. LSEEK writes no user memory, and DOS's frame below SP is masked natively.
- Exactly 1 MiB of RAM; IP = 1563h.
- Entry SP must lie in `[0006h,FFF4h]`. Outside that range the call returns ARGUMENT and changes nothing.
- Stops: RETURNED, SERVICE, ENV, ARGUMENT.

## Validation

Receipt: 2026-10-20 23:01:24–23:01:35 PDT (`final.json`), on a branch from main `c8c33022`.

**Unicorn: 74 cases, all RETURN.** Each compares unmasked CPU14 and all 1,048,576 RAM bytes, and the LSEEK operand log.

- Starts **23/23**; conditional outcomes **6/6** by successor.
- **70 independent checks.** Each compares the LSEEK operands against `(n·recsize) mod 2³²` computed separately, and
  checks InOutRes: 103 when not open, the DOS error, or untouched.
- Cases:
  - n 0/10, recsize 0/1;
  - **2³³ → 0**, **a high product that overflows 16 bits**, n = FFFFFFFFh × FFFFh, a "negative" n;
  - not open, unassigned; LSEEK errors;
  - six flag profiles × {open, closed};
  - SP bounds; SS = DS; an A20 stack;
  - a FileRec straddling offset FFFF and across A20;
  - **a FileRec over the arguments**;
  - 40 random cases.

**Native DOSBox-X: 24 cases equal.** Each compares CPU14, the stack window, the FileRec and DS:4420–442F.

- A declared INT 21h hook answers AH=42h by editing CF in the IRET frame.
- The DOS frame below the INT-time SP is masked per the contract.
- RESULT.BIN `8134fea72ede0af611b2ba65fcb03050e31c160530bbd8760d2eedd8c4dcaade`.
- Binary: `/usr/bin/dosbox-x` 2026.01.02, `99d74601…`. It is **not** the pinned `5a9c7c53` build, which is declared.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **16 hand-derived checks** pass. CTest 1/1. They cover:

- the LSEEK operands for 10·128;
- **both truncations**, and the carry from the low product into CX;
- the DOS error;
- not open (103, with the helper's RET word left on the stack);
- ARGUMENT, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0AF8_1563',)`. `--compile-only` builds the source with `-Werror` before any media access; it was
verified with the media absent.

    python3 tools/overlay/system_seek_1563_cpu_audit.py --compile-only
    python3 tools/overlay/system_seek_1563_cpu_audit.py --unicorn --native --output resources/system-seek/final.json

## Proposed coverage row (text only; the acceptance lands it)

    strict_candidate_queue.RUNTIME_COVERED_ENDPOINTS:  (0x0AF8, 0x1563): "system Seek (system_seek_1563_cpu, #<this PR>)"
