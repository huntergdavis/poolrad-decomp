# Turbo Pascal SYSTEM Seek at 0AF8:1563: raw CPU, runtime coverage (t29)

**Runtime coverage, not credit**, and no identity claim. This is the file-I/O package's Seek endpoint (VP t29). On par-a's
440-row export (main `04e5bc7b`), 77 rows have `0AF8:1563` in their closure. Per t29, unlocked rows are measured **after**
the coverage line lands and reported as a **delta**, never as a credit.

`pr_system_seek_1563` (`src/recovered/system_seek_1563_cpu.c`) models START.EXE `0AF8:[1563,1593)`. It **composes** par-d's
shared open-check model `pr_system_filecheck_14b6` (#1313) for the helper `0AF8:[14B6,14C5)`.

**Correction (self-flagged after #1314 merged).** The model as merged in #1314 inlined the helper and continued at 156Ch
without rereading the RET slot. The original's `RET` rereads it. With SS = DS and entry SP 442Ah, `PUSH BP` plus the `CALL` put
the slot at 4426h, which is InOutRes. For a not-open file, the helper's 103 store therefore lands on the slot, and the original
returns to **0067h**. The #1314 audit never constructed that state, so its 74/74 did not cover it. This revision composes the
shared model and names the outcome **TRANSFER**. The audit constructs the case, and against the #1314 source it fails as
`C status transfer: RETURN original TRANSFER`. BlockRead (#1315) aliases at the same slot, 4426h, from the same entry SP.

## Media spans (and a correction to my sizing)

| Piece | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| Seek | `0AF8:[1563,1593)`, image `[0C4E3,0C513)` | **48** / 19 | `598dcb76bff3948d01eb4b14d25969c11f736d4221438ffb29d9d06c9774a4af` |
| open check (composed) | `0AF8:[14B6,14C5)`, image `[0C436,0C445)` | 15 / 4 | `24d6d6fd5079eeb043c5f3a598502fa40ba66a4e12c919717ccdf847f362d363` |

My t29 sizing note (i20227) said 45 B / 18 instructions; the `RETF 8` actually ends at +1593, giving 48 B / 19.

The audit asserts:

- the single `INT 21h` (+1588);
- the two `MUL`s (+1571, +157A);
- the three conditional sites.

## Behaviour, with the fact a plausible model gets wrong (VP i20228)

1. `LES DI,f`, then `CALL 14B6`: unless the magic f+2 is D7B3h (open), InOutRes `DS:[4426]` := **103** and return. The
   helper's `RET` rereads its slot. If the store landed there, control goes to the stored value: **TRANSFER** (above).
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
- **TRANSFER:** the composed check returned somewhere other than 156Ch. CS:IP is where it returned.
- Stops: RETURNED, TRANSFER, SERVICE, ENV, ARGUMENT.

## Validation

Receipt: 2026-10-20 23:25:13–23:25:23 PDT (`final.json`), on a branch from main `71eb763b`. The #1314 receipt was 23:01:24–23:01:35 PDT on
main `c8c33022`, with 74 cases.

**Unicorn: 76 cases** (75 RETURN, **1 TRANSFER**). Each compares unmasked CPU14 and all 1,048,576 RAM bytes, and the LSEEK operand log.

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
  - **the TRANSFER alias** (SS = DS, entry SP 442Ah, not open), and the same slot with an open file;
  - 40 random cases.

**Native DOSBox-X: 24 cases equal.** Each compares CPU14, the stack window, the FileRec and DS:4420–442F.

- A declared INT 21h hook answers AH=42h by editing CF in the IRET frame.
- The DOS frame below the INT-time SP is masked per the contract.
- TRANSFER and the alias cases are compared by Unicorn only. The native case set is unchanged from #1314.
- RESULT.BIN `8134fea72ede0af611b2ba65fcb03050e31c160530bbd8760d2eedd8c4dcaade`.
- Binary: `/usr/bin/dosbox-x` 2026.01.02, `99d74601…`. It is **not** the pinned `5a9c7c53` build, which is declared.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **17 hand-derived checks** pass. CTest 1/1. They cover:

- the LSEEK operands for 10·128;
- **both truncations**, and the carry from the low product into CX;
- the DOS error;
- not open (103, with the helper's RET word left on the stack);
- **TRANSFER** to 0067h;
- ARGUMENT, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0AF8_1563',)`. `--compile-only` builds the source (with the composed check) with `-Werror` before any media access; it was
verified with the media absent.

    python3 tools/overlay/system_seek_1563_cpu_audit.py --compile-only
    python3 tools/overlay/system_seek_1563_cpu_audit.py --unicorn --native --output resources/system-seek/final.json

## Proposed coverage row (text only; the acceptance lands it)

    strict_candidate_queue.RUNTIME_COVERED_ENDPOINTS:  (0x0AF8, 0x1563): "system Seek (system_seek_1563_cpu, #1314; TRANSFER fix #<this PR>)"

## Acceptance of the coverage row (added by the review lane, par-e, VP i20278/i20284)

`(0x0AF8, 0x1563): "system Seek (system_seek_1563_cpu, #1314/#1317)"` lands in `RUNTIME_COVERED_ENDPOINTS`. It is readiness only, never a credit.

- **Audit reproduced at `f902b02a`, the #1317 TRANSFER fix included:**
  - **Unicorn:** 76 cases (`RETURN` 75, `TRANSFER` 1: the constructed `SS = DS` return-slot alias that #1314 had not
    built). Starts 23/23, outcomes 6/6, 70 independent checks, CPU14 + 1 MiB, masks 0.
  - **Native:** 24 cases.
- **Composed:** par-d's shared `14B6` check (`system_filecheck_14b6_cpu`) is linked in, not re-proved.
- **No terminal stop:** `INT 21h` AH=42h is a declared service.
- Native RESULT.BIN `8134fea7…` equals this document. The emulator is `/usr/bin/dosbox-x` 2026.01.02, not the pinned `5a9c7c53` build (a declared substitution). ASan/UBSan with `-Werror -Wpedantic`: 17 checks. `check_checker_builds RES_0AF8_1563 --require-current-main` passes.
- **Measured delta** (regenerated exports, closure pricer): with `1410`, `1563` and `1593` covered, **0** rows become free
  of uncovered SYSTEM endpoints (343 → 343). All 21 rows of the t29 prize are held by `14FB` alone (see the BlockRead
  record).
