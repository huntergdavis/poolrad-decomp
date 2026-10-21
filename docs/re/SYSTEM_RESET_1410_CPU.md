# Turbo Pascal SYSTEM Reset at 0AF8:1410: raw CPU, runtime coverage (t29)

**Runtime coverage, not credit**, and no identity claim. This is the file-I/O package's Reset endpoint (VP t29, i20228). On
par-a's 440-row export (main `04e5bc7b`), 82 rows have `0AF8:1410` in their closure. Per t29, the unlocked rows are measured
**after** the coverage line lands and reported as a **delta**, never as a credit.

`pr_system_reset_1410` (`src/recovered/system_reset_1410_cpu.c`) models the **Reset entry** of START.EXE's shared
Reset/Rewrite body `0AF8:[1410,1475)`.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| Reset/Rewrite | `0AF8:[1410,1475)`, image `[0C390,0C3F5)` | 101 / 42 | `18b5680c7d5ea7bfdee3902b7a11279f62216c07fbff9c692fb8820363b481f0` |

The audit asserts:

- the single `INT 21h` (+1456);
- the single near `CALL 1491` (Close);
- the four conditional sites.

**The Rewrite entry is a separate endpoint.** Its two instructions (+1419 `MOV AX,3C00h`, +141C `MOV DX,1`) are excluded
from this model's starts; Reset's `JMP 141F` at +1417 skips them.

## Behaviour, with the three facts a plausible model gets wrong (VP i20228)

1. **FileMode is read from `DS:[442C]`** into AL; AH := 3Dh and DX := 0.
2. The magic word at f+2 decides what happens:
   - **D7B0h** (closed): go straight to the open;
   - **D7B3h** (open): **the nested Close** (`PUSH CS; CALL 1491`) runs first, then the open;
   - anything else: InOutRes `DS:[4426]` := **102** (file not assigned), then return.
3. **The empty-name path does NO INT.** If f+30h is empty, `XCHG DX,AX` makes the handle 0 (the standard input handle;
   Rewrite's entry sets DX := 1, standard output). Otherwise the open is `INT 21h` AH=3Dh, AL=FileMode, DS:DX = f+30h.
4. On carry, InOutRes := AX (the DOS error) and the FileRec is unchanged. On success, the magic := D7B3h, f+0 := the
   handle and f+4 := recsize. `POP BP; RETF 6`.

## Contract and stops

- **Declared environment input: INT 21h AH=3Dh**, on the 137C conditions:
  - **Fail-closed AH list:** only 3Dh is listed. Any other AH is an **ENV** stop; it is unreachable here, since AH is the
    constant 3Dh at the INT. The Unicorn hook refuses any other interrupt or AH.
  - **Continuation check:** the result must be at 1458h with the same CS, SS, SP and BP; otherwise **SERVICE**, stopped at
    the INT.
  - **What the service may write:** nothing. An open writes no user memory. DOS's own frame below SP is outside the
    model, and the native run masks it.
- **The nested Close is a named stop (CLOSE)**, on the pattern of the 08AE stop-based members (VP i20246). The model stops
  with the call executed: AX, DX, ES, DI, CS and the return 1445h are pushed, and IP = 1491h.
  - To continue after Close returns, call again at **1445h** with the post-Close state.
  - The entries are 1410h and 1445h; any other IP is ARGUMENT.
  - par-d's Close model (t29) is composed in when it lands.
- Exactly 1 MiB of RAM.
- Entry SP must lie in `[000Eh,FFF6h]` at 1410h, or `[0006h,FFF6h]` when resuming. Outside that range the call returns
  ARGUMENT and changes nothing.
- Stops: RETURNED, CLOSE, SERVICE, ENV (unreachable here), ARGUMENT.

## Validation

Receipt: 2026-10-20 22:53:41–22:53:52 PDT (`final.json`), on a branch from main `e7362e73`.

**Unicorn: 68 cases, all RETURN.** Each compares unmasked CPU14 and all 1,048,576 RAM bytes. **The real Close bytes run**
inside the original, with the AH=3Eh answer declared.

- **Two-phase comparison** in the **22** cases that go through Close:
  1. the model's CLOSE stop is compared with the original's state on arrival at 1491h;
  2. the model is resumed at 1445h from the original's post-Close state, and compared again at the end.
- Starts **40/40** (Rewrite's two excluded as above); conditional outcomes **8/8** by successor.
- **63 independent checks** compare the FileRec (magic, handle, recsize) and InOutRes against a separate statement of
  Reset's effect. They include Close's own rule, **3Eh only for handles above 4**, so standard handles 0–4 are never
  closed.
- Cases:
  - closed or open × name, empty name, open error;
  - the magics 1234h, 0000h and D7B1h (unassigned);
  - Close with handles 2, 4, 5 and 9, and a Close error;
  - FileMode 0/1/2/40h/FFh; recsize 0/1/FFFFh;
  - six flag profiles × {closed, open};
  - SP bounds; SS = DS; an A20 stack;
  - a FileRec straddling offset FFFF and across A20; a wrapping name;
  - **the FileRec over the stack frame**;
  - 24 random cases.

**Native DOSBox-X: 33 cases equal** (20 RETURN, 13 at CLOSE). Each compares CPU14, the stack window, the 128-byte FileRec
and DS:4420–442F.

- The declared INT 21h hook answers 3Dh/3Eh by editing CF in the IRET frame; other functions chain to DOS.
- **The DOS frame of each real INT** (6 bytes below the INT-time SP) is outside the service contract and masked at the
  recorded address.
- **Close capture, a change to the native copy only.** For the Close-path cases, the first five bytes of `0AF8:1491`
  (`8B DC 36 C4 7F`: Close's own code, not this body's) are replaced by a far JMP to a capture routine. The bytes before
  and after are in the receipt. The resume after Close is compared by Unicorn only.
- RESULT.BIN `98f2794b9c38c36bad90aafb9df9013f082aadfe6690dce983d601462a04b102`.
- Binary: `/usr/bin/dosbox-x` 2026.01.02, `99d74601…`. It is **not** the pinned `5a9c7c53` build, which is declared.
- **One harness defect found and fixed:** a case with DF set left DF set on return. That broke the harness's own string
  copy, not the model, and the harness now clears DF after recording the flags.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **22 hand-derived checks** pass. CTest 1/1. They cover:

- the open, with the CPU at the INT: AH 3Dh, AL = FileMode, DS:DX = f+30h, CX 0;
- the empty name (no INT, handle 0, DX 3D02h);
- the DOS error (InOutRes 2) and the unassigned magic (102);
- the CLOSE stop with its exact pushed frame;
- the resume at 1445h;
- SERVICE;
- ARGUMENT, including the Rewrite IP 1419h, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0AF8_1410',)`. `--compile-only` builds the source with `-Werror` before any media access; it was
verified with the media absent.

    python3 tools/overlay/system_reset_1410_cpu_audit.py --compile-only
    python3 tools/overlay/system_reset_1410_cpu_audit.py --unicorn --native --output resources/system-reset/final.json

## Proposed coverage row (text only; the acceptance lands it)

    strict_candidate_queue.RUNTIME_COVERED_ENDPOINTS:  (0x0AF8, 0x1410): "system Reset (system_reset_1410_cpu, #<this PR>)"

The nested Close is pending par-d's Close endpoint. Until both lines land, the composition through Close is the named stop.
