# Turbo Pascal SYSTEM FilePos at 0AF8:1593: raw CPU, runtime coverage (t29)

**Runtime coverage, not credit**, and no identity claim. This is the last endpoint of the file-I/O package (VP t29, i20228). On
par-a's 440-row export, 77 rows have `0AF8:1593` in their closure. Per t29, the unlocked rows are measured **after** the
coverage line lands and reported as a **delta**, never as a credit.

`pr_system_filepos_1593` (`src/recovered/system_filepos_1593_cpu.c`) models FilePos `0AF8:[1593,15AA)` together with its
position helper `0AF8:[15DA,161F)`. The helper is shared with FileSize (+15AA) and Eof (+15C5); those are separate endpoints
and are not in this model's starts. The record division composes the **covered LongDiv model** (`system_longdiv_0a76_cpu`,
`RUNTIME_COVERED_ENDPOINTS (0x0AF8, 0x0A76)`).

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| FilePos | `0AF8:[1593,15AA)` | 23 / 9 | `60365ee418d46db02cab8c9d35bc2b9bd4121e5e7f993b6d2c83174c344c0c9a` |
| position helper | `0AF8:[15DA,161F)` | 69 / 32 | `337cdaa3ab878cf422eccc440796d2eceac214d3234178d952eb1a6cd135afa5` |
| LongDiv (composed) | `0AF8:[0A76,0AE4)` | 110 / 55 | `131b97677c63a6daa216f4219868f045485e35f999a551feb16daf4ec50d69e5` |

The audit asserts:

- the three `INT 21h` (+15F2, +1600, +160E);
- the two calls (`CALL 15DA`, `CALL 0A76`);
- the three conditional sites.

## Behaviour, with the VP's gotchas (i20228, kept verbatim)

1. **The shared helper does THREE LSEEKs (4201h current, 4202h end, 4200h restore), so a one-LSEEK model FAILS THE EVENT
   LOG.**
   - `INT 21h` AX=4201h with CX:DX = 0 reads the current position, and `PUSH DX; PUSH AX` saves it.
   - AX=4202h with CX:DX = 0 seeks to the end. `POP BX; POP CX` recovers the current position, and `PUSH DX; PUSH AX` saves
     the end.
   - AX=4200h with CX:DX = the current position restores it.
   - `POP CX; POP BX` leaves CX = the end's low word and BX = its high word. `RET` keeps the restore's flags and DX:AX.
   - **No carry is checked between the LSEEKs.** An error code from 4201h becomes the restore's CX:DX. Only the restore's
     carry reaches FilePos.
2. **The result divides by recsize via LongDiv 0A76 (covered) unless recsize = 1.**
   - On the restore's carry (`JB 15A7`), FilePos returns DX:AX as the restore left it, with no division and no InOutRes.
   - Otherwise CX := recsize (f+4).
   - CX = 1: return DX:AX as it is.
   - Otherwise: BX := 0; `PUSH CS; CALL 0A76`, which computes DX:AX := DX:AX DIV 0:recsize. **This is a signed LongInt
     divide**, so positions ≥ 2³¹ divide as negative values.
3. **Not open** (magic f+2 ≠ D7B3h): the helper sets InOutRes `DS:[4426]` := **103**, DX:AX := 0, STC, and makes no LSEEK.
   FilePos returns 0 with CF set.
4. `RETF 4`.

## Contract and stops

- **Declared environment input: INT 21h AH=42h** at 15F2h, 1600h and 160Eh, in that order. The 137C conditions:
  - **Fail-closed AH list:** only 42h is listed. Any other AH is an **ENV** stop; it is unreachable, because AX is a constant
    at each INT. The Unicorn hook refuses any other interrupt, any other AH, and a fourth LSEEK.
  - **Continuation check:** each answer must return at the INT + 2 with the same CS, SS, SP and BP. Otherwise **SERVICE**,
    stopped at that INT.
  - **What the service may write:** nothing. An LSEEK writes no user memory. DOS's frame below SP is outside the model, and the
    native run masks it.
- **DIVIDE (EXIT-class):** recsize 0 passes `CMP CX,1` and reaches LongDiv with divisor 0. LongDiv's runtime-error path
  (`AX := 00C8h; JMP 00E2`, RunError 200) does not return. The stop is the CPU as LongDiv's model leaves it: CS:00E2, AX = 00C8h.
- **TRANSFER:** the helper's `RET` and LongDiv's `RETF` reread their slots. With SS = DS and entry SP 4428h, the helper's slot is
  InOutRes, so a not-open file stores 103 there and the RET goes to 0067h. CS:IP is where control went. FilePos's helper is
  entered directly by `CALL` with no `PUSH BP` first, so its alias SP is 4428h, against 442Ah for Seek and BlockRead.
- **Flags:** the final flags are the restore's (carry path), `CMP CX,1`'s (recsize 1), LongDiv's (division), or the not-open
  path's `XOR`/`STC`. Nothing is masked.
- Exactly 1 MiB of RAM.
- Entry SP must lie in `[000Ah,FFF8h]`: the pushes reach entry SP − 0Ah inside LongDiv, and the argument reaches entry SP + 7.
  Outside that range the call returns **ARGUMENT** and changes nothing.
- Stops: RETURNED, DIVIDE, TRANSFER, SERVICE, ENV (unreachable here), ARGUMENT.

## Validation

Receipt: 2026-10-20 23:23:16–23:23:25 PDT (`resources/system-filepos/final.json`), on a branch from main `71eb763b`.

**Unicorn: 94 cases** (83 RETURN, **10 DIVIDE**, **1 TRANSFER**). Each compares unmasked CPU14, all 1,048,576 RAM bytes and
**the ordered LSEEK event log**: (IP, AX, BX, CX, DX) at each INT, C equal to the original.

- The original LongDiv bytes run in the original and reach 50 LongDiv starts. Those starts are counted under LongDiv's own row,
  not this one.
- Starts **41/41** (9 FilePos + 32 helper); conditional outcomes **6/6** by successor.
- **89 independent checks** compare the status, the three-LSEEK log (with the restore's CX:DX = the first answer), InOutRes,
  DX:AX and the carry against a separate statement of FilePos's effect, including the signed division.
- Cases:
  - positions 0, 12345h and FFFE0001h;
  - recsize 1, 2, 7 (with a remainder), 128 and FFFFh;
  - signed positions 80000000h and FFFFFF00h;
  - errors on each LSEEK, on all three, and on the restore with recsize 0 (no division);
  - a restore that answers differently from the current position;
  - not open and unassigned;
  - **DIVIDE**, including at position 0;
  - six flag profiles × {open, closed, recsize 1, divide};
  - SP bounds; SS = DS; an A20 stack;
  - FileRec straddles across offset FFFF and A20; the FileRec over the arguments;
  - **the TRANSFER alias**, and the same slot with an open file;
  - 40 random cases.

**Native DOSBox-X: 42 cases equal** (34 RETURN, **8 DIVIDE**). Each compares CPU14, **the native LSEEK event log**, a 1 KiB
stack window, the FileRec and DS:4420–442F.

- **The declared INT 21h hook** records (return IP, AX, BX, CX, DX) for each AH=42h, then answers the case's next DX:AX and
  carry (CF is edited in the IRET frame). Other functions chain to DOS.
- **DIVIDE capture, a change to the native copy only.** LongDiv's runtime-error target `CS:00E2` is outside every piece. The
  harness writes a far JMP there to a capture routine, which records the registers and FLAGS with CS:IP = 6000:00E2.
- **The DOS frame of each real INT 21h** (6 bytes below the INT-time SP) is outside the contract and masked at the recorded
  address. There are no flag masks.
- RESULT.BIN `1b7287055df76ff31fd16513034b9e08ac44aaca0effecd8b34d4b072aaf621b`.
- Binary: `/usr/bin/dosbox-x` 2026.01.02, `99d74601…`. It is **not** the pinned `5a9c7c53` build, which is declared.
- The straddle, overlap, TRANSFER and random cases are compared by Unicorn only.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **25 hand-derived checks** pass. CTest 1/1. They cover:

- the three LSEEKs, with each one's IP, AX, BX, CX:DX and SP;
- the division;
- recsize 1, with CX and BX after the pops;
- the signed division;
- the unchecked 4201h error feeding the restore;
- the restore's carry skipping the division;
- not open;
- **DIVIDE** at 00E2h with AX = 200;
- **TRANSFER** to 0067h;
- SERVICE on the second LSEEK;
- ARGUMENT, including the FileSize IP 15AAh, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0AF8_1593',)`. `--compile-only` builds the source (with the composed LongDiv) with `-Werror` before any
media access.

    python3 tools/overlay/system_filepos_1593_cpu_audit.py --compile-only
    python3 tools/overlay/system_filepos_1593_cpu_audit.py --unicorn --native --output resources/system-filepos/final.json

## Proposed coverage row (text only; the acceptance lands it)

    strict_candidate_queue.RUNTIME_COVERED_ENDPOINTS:  (0x0AF8, 0x1593): "system FilePos (system_filepos_1593_cpu, #<this PR>)"
