# Turbo Pascal Dos.Intr at 0A32:000B: raw CPU, runtime coverage

**Runtime coverage, not credit.** This is a Turbo Pascal Dos-unit library routine. No identity or credit is
claimed.

It is a prerequisite of the funded `RES_0899_0059` / `RES_0899_0000` wall (VP i20005). `0899:0000`, the
game's quit routine, restores the video mode with `Intr($10, Regs)` before `Halt`. Per par-a, Intr also
appears in three ungated rows through other bodies. It is covered on the same footing as Dos.GetTime (#1245).

`pr_dos_intr_000b` (`src/recovered/dos_intr_000b_cpu.c`) models START.EXE `0A32:[000B,006C)`.

## Media span

| Body | Span | Bytes / insns | SHA-256 |
| --- | --- | ---: | --- |
| Dos.Intr | `0A32:[000B,006C)`, loader-expanded image `[0A32B,0A38C)` | 97 / 69 | `08854b63986cc7a20465b9594ceb0ac8d86c7467dccdcf49d894a04b79dccde9` |

**The first RETF is not the return.** The audit asserts that the span holds exactly two returns:

- `RETF` at +0044. This is the *transfer* into the handler. Intr pushes a fake INT frame and RETFs to the
  vector, and the handler's own return lands at +0045, the very next instruction.
- `RETF 6` at +0069, the real return.

The audit also asserts that the span holds no INT, CALL or branch. The span therefore runs to `RETF 6`;
the "first RETF" rule would cut the routine in half. +0000…+000A, just before this entry, is `MsDos`: it
pushes 21h and falls into Intr. It is not part of this span.

## Behaviour

Entry is a far call with the stack holding `[SP]` IP, `[SP+2]` CS, `[SP+4]` the Regs offset, `[SP+6]` the
Regs segment and `[SP+8]` IntNo, of which only the low byte is read.

1. `PUSH BP; PUSH DS; MOV BP,SP`. Then the fake INT frame: `PUSHF; PUSH CS; PUSH 0045h`.
2. `XOR BX,BX; MOV DS,BX; MOV BL,[BP+0Ch]; SHL BX,1; SHL BX,1; LDS BX,[BX]` reads the vector at
   0000:IntNo·4, which is then pushed as the RETF target.
3. `LDS SI,[BP+8]; CLD`, then nine `LODSW`s load AX, BX, CX, DX, BP, SI, DI, DS and ES from the Regs record.
   Regs.FLAGS is **not** loaded. Then `CLI; RETF` into the handler.
4. At +0045, after the handler returns: `PUSHF; PUSH ES; PUSH DI; PUSH BP; MOV BP,SP; LES DI,[BP+10h]; CLD`.
   Ten `STOSW`s then store AX, BX, CX, DX, BP, SI, DI, DS, ES and FLAGS back into Regs, as the handler left
   them.
5. `POP DS; POP BP; RETF 6`.

The final AX is the stored FLAGS word, DI points past the record and ES is the Regs segment. DS and BP are
the caller's. The flags are the handler's with DF cleared.

Every operand is reread from RAM in source order. If the Regs record aliases the frame or the vector, the
original's order is followed; the audit constructs both cases.

## Contract and stops

- Exactly 1 MiB of A20-off RAM. IP = 000Bh.
- Entry SP must lie in `[0014h,FFF6h]`. The deepest push is entry SP − 14h, and the highest argument byte is
  entry SP + 9, so no frame word crosses SS:FFFF. Outside that range the call returns ARGUMENT and changes
  nothing.
- **Declared environment input: the interrupt handler.** The service receives RAM and the CPU at handler
  entry:
  - CS:IP is the vector;
  - SP is entry SP − 0Ah, at the fake frame (0045h, CS, entry FLAGS);
  - IF and DF are clear.

  It returns the complete CPU at the continuation: the body's CS, IP 0045h, the same SS, and SP = handler
  SP + 6. That holds for an `IRET`, and also for a `RETF 2` that returns live flags. Anything else, or a
  refusal, stops with **SERVICE** at handler entry.
- **The service may write RAM**, as a real handler such as INT 10h writes BIOS data and video memory. Unlike
  GetTime's DOS service, nothing is banned. Everything the handler does is environment, and the model takes
  it as given.
- Stops: RETURNED, ARGUMENT, SERVICE. The body has no conditional branch and no loop.

### Flags handed to the handler: a declared Unicorn split

The handler sees the flags of the second `SHL BX,1`:

- CF, OF and SF are 0, because IntNo·4 < 400h;
- ZF and PF follow IntNo·4;
- DF and IF are clear;
- TF and the reserved bits are unchanged.

AF after SHL is architecturally undefined. Pinned DOSBox-X computes AF = 1 for any non-zero shift; the
shipped `/usr/bin/dosbox-x` measures the same (below). Unicorn gives 0. **The model follows DOSBox-X**,
per the standing ruling that models follow DOSBox-X with a declared Unicorn split.

The split is visible only to a handler that reads its entry flags. The audit's stub C does exactly that:
`PUSHF; POP AX`, which stores them into Regs.AX. In Unicorn the split is asserted to be **exactly bit 4 of
the low byte of Regs.AX**: model 1, Unicorn 0, and nothing else differs. Stub C's own PUSHF word is
overwritten by `PUSH BP` at +0048. Natively, stub C matches the model exactly.

## Validation

Receipts: Unicorn 2026-10-20 20:09:47–20:12:33 PDT; native 20:12:41–20:14:00 PDT. Both ran on a branch
from main `b3fd5718`, which was then rebased onto `c9de4a43` with no change to these files.

**Declared stub handlers.** The handlers are real x86 bytes at the vector, and the C service reproduces
each one exactly:

| Stub | Bytes do | Returns by |
| --- | --- | --- |
| A | `XOR AX,5A5Ah; ADD BX,CX; MOV ES:[DI],DX; NOT SI; MOV BP,1234h; PUSH ES; POP DS` (registers and a RAM write) | `IRET` |
| B | `ADD AX,CX; STC` | `RETF 2` (live flags) |
| C | `PUSHF; POP AX` (observes entry flags) | `IRET` |
| D | `PUSH AX; PUSH BX; POP AX; POP BX` (stack use below SP) | `IRET` |

**Unicorn: 347 cases**, comparing unmasked CPU14 and all 1,048,576 RAM bytes. Starts **69/69**; there are
no conditional outcomes, and every case RETURNs. Cases:

- all 256 IntNo values with stub A;
- stubs B, C and D at IntNo 00, 10, 21, 61, 80 and FF;
- seven flag profiles (including IF set) × all four stubs;
- SP 0014, 0015, 8001, FFF5 and FFF6; SS = DS; an A20 stack;
- Regs straddling offset FFFF (odd offset FFF1), Regs wrapping at FFF0, and Regs across A20;
- **Regs on the IVT entry itself** (`regs-on-ivt`): the record is read after the vector;
- **Regs over the saved BP and the return address** (`alias-regs-on-frame`). The stores at +0045 rewrite the
  return IP, and the audit asserts that the RETF 6 is redirected;
- an IntNo word with high byte 0;
- 32 random cases.

The declared split appears in exactly the 13 stub-C cases, each asserted to be the single bit above.

**Native DOSBox-X: 36 cases equal**, unmasked:

- stubs A–D at user vectors 60h, 61h, 66h, 67h, F1h and FFh;
- six flag profiles × stubs A and C.

Each case compares CPU14, the whole 1 KiB stack window and the 64-byte data region (the Regs record plus
stub A's write target). Interrupts stay off from the window fill until the record is taken; IF-set
profiles are therefore Unicorn-only. Each case's vector is saved and restored around the call.

- RESULT.BIN SHA-256 `c4997fd59e6a5ff944e80393530f40c66e59402248d3d0dba70be704b4ffd462`.
- Binary: `/usr/bin/dosbox-x`, SHA-256 `99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`,
  `DOSBox-X version 2026.01.02 SDL2`. It is **not** the pinned source build `5a9c7c53`; the `dosbox_x`
  receipt block records this.
- What could differ from the pinned build: AF after SHL, which this binary gives as 1 (stub C equal), the
  same as the pinned source's `get_AF`. Word straddles are not exercised natively.

**C test.** Built with `cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
-fno-sanitize-recover=all`: no warnings, and **26 hand-derived checks** pass. CTest 1/1. They cover:

- the handler-entry CPU, the fake frame and the entry flags (0603h → 0012h; IntNo 0 → 0056h);
- the stored Regs record;
- the stack words after return;
- the final CPU;
- SERVICE on a refusal and on a wrong-SP continuation;
- ARGUMENT, including SP 0013 and FFF7, with nothing changed.

## Checker and reproduction

`CHECKER_ROWS = ('RES_0A32_000B',)`. `--compile-only` builds the source with `-Werror` before any media
access; it was verified with the media absent.

    python3 tools/overlay/dos_intr_000b_cpu_audit.py --compile-only
    python3 tools/overlay/dos_intr_000b_cpu_audit.py --unicorn --native --output resources/dos-intr/final.json

## Proposed coverage row (text only; the acceptance lands it)

    strict_candidate_queue.RUNTIME_COVERED_ENDPOINTS:  (0x0A32, 0x000B): "Dos.Intr (dos_intr_000b_cpu, #<this PR>)"

No strict, identity, manifest or driver_internal row is proposed.
