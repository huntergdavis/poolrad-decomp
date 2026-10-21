# Turbo Pascal SYSTEM BlockRead at 0AF8:14FB: raw CPU, runtime coverage (t29)

**Runtime coverage, not credit**, and no identity claim. This is the file-I/O package's BlockRead endpoint (VP t29, i20228). On
par-a's 440-row export, 78 rows have `0AF8:14FB` in their closure. Per t29, the unlocked rows are measured **after** the
coverage line lands and reported as a **delta**, never as a credit.

`pr_system_blockread_14fb` (`src/recovered/system_blockread_14fb_cpu.c`) models the **BlockRead entry** of START.EXE's shared
BlockRead/BlockWrite body `0AF8:[14FB,1563)`.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| BlockRead/BlockWrite | `0AF8:[14FB,1563)` | 104 / 45 | `e015b11ca2dff4700351966c73ece1b3153fb7221d80ced1a9f292f0c8aadf12` |
| open check (composed) | `0AF8:[14B6,14C5)` | 15 / 4 | `24d6d6fd5079eeb043c5f3a598502fa40ba66a4e12c919717ccdf847f362d363` |

The audit asserts:

- the single `INT 21h` (+1529);
- the one `MUL` (+151B) and the one `DIV` (+1531);
- the single near `CALL 14B6` (+150D);
- the seven conditional sites.

**The BlockWrite entry is a separate endpoint.** Its two instructions (+1502 `MOV BL,40h`, +1504 `MOV CX,65h`) are excluded
from this model's starts, and the audit asserts that they never run. BlockRead's `JMP 1507` at +1500 skips them.

**The open check is composed, not re-proved.** `CALL 14B6` runs par-d's shared model `pr_system_filecheck_14b6` (#1313,
`system_filecheck_14b6_cpu`). Unicorn runs the original helper bytes. Because the callee is modelled, no two-phase seam is
needed (the #1311 technique is for an unmodelled callee).

## Behaviour, with the three gotchas (VP i20228, kept verbatim)

1. **The LOW WORD of count*recsize, with the overflow DROPPED.** `MUL WORD ES:[DI+4]` forms DX:AX, and `MOV CX,AX` keeps only
   AX. For example, 200h × 100h = 20000h asks DOS for **0** bytes, and 3 × 6000h = 12000h asks for 2000h.
2. **DIV by recsize = 0 raises INT 0, which leads to RunError 200: an EXIT-class stop.** After a successful read,
   `XOR DX,DX; DIV WORD ES:[DI+4]` turns bytes into records. With recsize 0 and count ≠ 0, that DIV faults. Turbo Pascal's
   INT 0 handler then ends the program, so nothing after the DIV runs in this body. With count = 0 the body jumps past the
   read and the DIV, so recsize 0 is harmless there.
3. **The read service WRITES RAM, so its contract must PERMIT the buffer write.** INT 21h AH=3Fh stores the file's bytes at
   DS:DX. The contract permits exactly that range and nothing else (see below).

The sequence:

- BL := 3Fh; CX := 100 (the InOutRes for a short read).
- `PUSH BP; MOV BP,SP; LES DI,f; CALL 14B6`. Unless the magic is D7B3h (open), InOutRes `DS:[4426]` := **103**, and then
  `*result := 0` when result is not nil.
- If count = 0: no INT; AX := 0.
- Otherwise:
  - `PUSH DS; PUSH CX`;
  - CX := the low word of count·recsize; DS:DX := buf; AH := 3Fh; BX := the handle f+0;
  - `INT 21h`, then `POP CX; POP DS`;
  - on carry: InOutRes := AX, then `*result := 0` when result is not nil;
  - otherwise: AX := bytes read DIV recsize.
- When result is not nil: `*result := AX`. When result is nil and AX ≠ count: InOutRes := CX = **100**.
- `POP BP; RETF 0Eh`.

Every word the body reads after the INT (recsize, count, the result pointer, the return frame) is **reread from RAM**. So a
buffer that overlaps any of them changes what follows. The audit constructs each such overlap.

## Contract and stops

- **Declared environment input: INT 21h AH=3Fh**, on the 137C conditions:
  - **Fail-closed AH list:** only 3Fh is listed. Any other AH is an **ENV** stop; it is unreachable, because AH := BL = 3Fh at
    the INT. The Unicorn hook refuses any other interrupt or AH (except INT 0, below).
  - **Continuation check:** the result must be at 152Bh with the same CS, SS, SP and BP. On no carry, AX must be ≤ CX (a read
    returns at most what was asked). Otherwise **SERVICE**, stopped at the INT with the buffer unchanged.
  - **What the service may write: exactly the buffer.** The service is handed the CX bytes at linear DS·16+DX (contiguous,
    with the 1 MiB wrap, as DOSBox-X's `MEM_BlockWrite`). It may change any of those bytes, on success or on error. The model
    writes them back, and **no other byte can change**. DOS's own frame below SP is outside the model, and the native run masks
    it.
- **DIVIDE (EXIT-class):** the CPU is at the faulting DIV: IP 1531h, DX = 0, CX and DS popped, BP and the frame pushed. Nothing
  after the DIV runs. Unicorn reports INT 0 at 1531h, and DOSBox-X pushes 1531h in its INT 0 frame (measured).
- **TRANSFER:** the check's RET rereads its slot. With SS = DS and entry SP 442Ah, the slot is InOutRes, so a not-open file
  stores 103 there and the RET goes to 0067h. CS:IP is where it returned. (The same alias exists in Seek's merged inline helper;
  see the report to the VP.)
- **Flags:**
  - MUL's undefined SF/ZF/AF/PF are set as DOSBox-X's 386 core sets them. They reach only the service's input flags.
  - DIV leaves the flags unchanged.
  - The body overwrites both before it reads a flag (`XOR DX,DX`, `OR DX,DI`, `CMP`).
- Exactly 1 MiB of RAM.
- Entry SP must lie in `[0006h,FFEEh]`: pushes reach entry SP − 6, and the arguments reach entry SP + 11h. Outside that range
  the call returns **ARGUMENT** and changes nothing.
- Stops: RETURNED, DIVIDE, TRANSFER, SERVICE, ENV (unreachable here), ARGUMENT.

## Validation

Receipt: 2026-10-20 23:15:41–23:15:51 PDT (`resources/system-blockread/final.json`), on a branch from main `c4480522`.

**Unicorn: 105 cases** (93 RETURN, **11 DIVIDE**, **1 TRANSFER**). Each compares unmasked CPU14 and all 1,048,576 RAM bytes,
including every byte the declared read wrote.

- Starts **47/47**: 43 in the body plus 4 in the helper, with BlockWrite's two excluded and asserted never run.
- Conditional outcomes **14/14** by successor.
- **90 independent checks** compare the status, the read's operands (AH, handle, CX = (count·recsize) mod 2¹⁶, DS:DX),
  InOutRes, `*result` and the buffer contents against a separate statement of BlockRead's effect.
- Cases:
  - full, short, exact-multiple and zero-byte reads, with and without a result pointer;
  - count 0 (including recsize 0);
  - both overflow shapes and FFFFh × FFFFh;
  - recsize 1 and FFFFh;
  - read errors (with bytes written before the error);
  - not open and unassigned;
  - **DIVIDE** with and without a result pointer;
  - a read error with recsize 0 (no DIV);
  - six flag profiles × {open, closed, divide};
  - SP bounds; SS = DS; an A20 stack;
  - FileRec, buffer and result straddles across offset FFFF and A20;
  - **buffers overlapping:**
    - the FileRec: recsize zeroed by the read leads to DIVIDE, and a changed recsize changes the DIV;
    - the count argument;
    - the result pointer;
    - the return address;
    - InOutRes;
  - the result pointer on the buffer and on the FileRec;
  - **the TRANSFER alias**, and the same SS = DS slot with an open file;
  - 40 random cases.

**Native DOSBox-X: 43 cases equal** (35 RETURN, **8 DIVIDE**). Each compares CPU14, a 1 KiB stack window, the FileRec,
DS:4420–442F, a 1 KiB buffer window and the result window.

- **The declared INT 21h hook** writes the case's bytes at DS:DX for AH=3Fh, then sets AX and CF (CF is edited in the IRET
  frame). Other functions chain to DOS.
- **The declared INT 0 hook** captures DIVIDE. It records the registers as they were at the fault, CS:IP and FLAGS from the
  frame, and SP above the frame. The frame's IP is **1531h**, equal to the model's stop.
- **The DOS frame of each real INT 21h, and the INT 0 frame** (6 bytes below the interrupt-time SP), are outside the contract
  and masked at the recorded address. There are no flag masks.
- RESULT.BIN `683c5493608bbae6cee058ee4cc2eb6a875d24ebe19c792ceb51b09a2c19603d`.
- Binary: `/usr/bin/dosbox-x` 2026.01.02, `99d74601…`. It is **not** the pinned `5a9c7c53` build, which is declared.
- The overlap and straddle cases, TRANSFER and the random cases are compared by Unicorn only.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **32 hand-derived checks** pass. CTest 1/1. They cover:

- the read, with the CPU at the INT;
- the permitted write landing, with the byte after it untouched;
- short reads, with and without a result pointer;
- both overflow shapes;
- count 0;
- a read error that keeps the bytes it wrote;
- not open (103);
- **DIVIDE**, with its exact CPU, and recsize 0 with count 0;
- **TRANSFER** to 0067h;
- SERVICE, both declined and AX > CX;
- ARGUMENT, including the BlockWrite IP 1502h, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0AF8_14FB',)`. `--compile-only` builds the source (with the composed check) with `-Werror` before any
media access.

    python3 tools/overlay/system_blockread_14fb_cpu_audit.py --compile-only
    python3 tools/overlay/system_blockread_14fb_cpu_audit.py --unicorn --native --output resources/system-blockread/final.json

## Proposed coverage row (text only; the acceptance lands it)

    strict_candidate_queue.RUNTIME_COVERED_ENDPOINTS:  (0x0AF8, 0x14FB): "system BlockRead (system_blockread_14fb_cpu, #<this PR>)"

## Acceptance of the coverage row (added by the review lane, par-e, VP i20278/i20284/i20294)

`(0x0AF8, 0x14FB): "system BlockRead (system_blockread_14fb_cpu, #1315)"` lands in `RUNTIME_COVERED_ENDPOINTS` in the
same commit as [`RES_0AF8_00DF`](DECLARED_BOUNDARY_0AF8_00DF.md). The `DIVIDE` stop raises INT 0, SYSTEM sets INT 0's
vector to `0AF8:00DF`, and the `00E2` declaration excludes `00DF`, so this row could not land before `00DF` was declared.
`RUNTIME_COVERED_TERMINALS` records `14FB → RES_0AF8_00DF`. This is readiness only, never a credit.

- **Audit reproduced at `f902b02a`:**
  - **Unicorn:** 105 cases (`RETURN` 93, `DIVIDE` 11, `TRANSFER` 1). Starts 47/47 (43 in the body), outcomes 14/14,
    90 independent checks, CPU14 + 1 MiB, masks 0.
  - **Native:** 43 cases (`RETURN` 35, `DIVIDE` 8), RESULT.BIN `683c5493…` equal to this document.
- **Composed:** par-d's shared `14B6` check is linked in.
- **ASan/UBSan** with `-Werror -Wpedantic`: 32 checks.
- **Checker build:** `check_checker_builds RES_0AF8_14FB --require-current-main` passes.

**The t29 package delta, measured by the review lane** (regenerated exports and the closure pricer; dated: main
`d9000b50` plus this PR):

| State | Rows with no uncovered SYSTEM endpoint |
| --- | ---: |
| main, before the last four | 343 |
| + `1410`, `1563`, `1593` | 343 (0 freed) |
| + `14FB` (this commit) | **364 (+21)** |

- **The freed set equals the VP's list** (i20278): 21 rows, none missing and none extra, matching par-b's independent
  measurement.
- **All 21 are held by `14FB` alone.** In the three-row state, each one's only uncovered SYSTEM endpoint is `0AF8:14FB`.
- **Direct-callee queue:** 440 / 52 / 9 is unchanged. Three rows' direct targets turn `runtime_covered`, with no
  composability flip.

**What "free of SYSTEM endpoints" does NOT mean.** It does not mean one body from composing.
- **Direct unready callees** per row: 1 for four rows (`0025_0341`, `0025_0411`, `0025_1A6F`, `03B9_05D9`), 2 for
  seven, and 3 to 16 for the other ten.
- **Closures:** every one of the 21 still holds 13 to 161 uncovered bodies, from 1,835 B (`0025_0341`) to 58,104 B
  (`002F_01F6`).
