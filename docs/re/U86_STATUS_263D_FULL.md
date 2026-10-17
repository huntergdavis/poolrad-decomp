# U86 +263D, a status-text wrapper, as a strict full-state draft (Owner #639)

GAME.OVR unit 0x86 + 0x263D (`OVL_0086_263D`, public entry slot 47, `game_evidenced`),
`[263D,266A)` = file `[0x29655,0x29682)`, 45 bytes, 25 instruction starts, sha256
`9424f472b73c4194146f3660d44a7fbb8a44cdd9c1fecebe4e64f4f8759fccf3`, ending in a plain
`retf`; a zero pad byte at +266A precedes the next prologue at +266B. CS literal pool
`[2631,263D)` = a Pascal string of length 0x0B (12 bytes with the length byte), sha256
`294632af0303f5775883c6656b32f019d0986767b1c56fddfb86e76a901f829d`, loaded only by this
body; its text is not reproduced here (original-media rule) and the model never reads it.
Owner screen (relative CLEAR, body and CS pool): all thirteen queues, pT, pF, pS, p5, p9,
pC, pR, p0, pP, pD, pH, pG and pW, completed with pW i15479 (summary i15481); PM local
source GO i16735 on public main 29c7f9f (no U86 source/API change since 53d6b79).

| offset | effect |
| --- | --- |
| +263D..+2640 | `push bp; mov bp,sp; sub sp,0xC` (SUB flags exact) |
| +2643..+2652 | `mov al,[0x681B]; push ax`, then `mov al,0; push ax` four times (AH carried) |
| +2653..+265D | `lea di,[bp-0xC]; push ss; push di; mov di,0x2631; push cs; push di; lcall 0xAF8:0x0B0B`: the runtime string load copies the CS pool into the 12-byte temp `SS:[bp-0xC]` and `retf 4` leaves that pointer pushed |
| +2662..+2663 | `push cs; call rel16` (original displacement 0xE25D giving +08C3): the unit-local routine (462 bytes, `retf 0xE`) with the string and the five bytes |
| +2666..+2669 | `mov sp,bp; pop bp; retf` |

`src/recovered/overlay_u86_status_263d_full.c` translates the 25 instructions over
registers, FLAGS and 1 MiB of RAM in source order, with the +225D conventions (same
template, every offset +0x3E0; the frame, pool and displacement differ). The lcall is raw:
the far target is read live from CS:+265E (offset) and CS:+2660 (segment) before the
lcall's own pushes (0x0B0B and 0x0AF8 by original default, a loader fixup); the model pushes
the caller CS and +2662 and enters it; the hook runs the callee including its `retf 4`. The
near-far call pushes CS, reads the displacement live at CS:+2664 (+08C3 by original
default), pushes +2666 and enters the target; the hook supplies the callee's effects except
its return and the model performs its `retf 0xE` from the live stack. Both callees are
opaque hooks: no shared ABI or callee model is added or claimed. Writes AL, DI, FLAGS (the
SUB) and SP/BP/IP/CS; ES is untouched. DS:0x681B is read only; no ownership claim. The entry
IP is neither read nor checked (IP is first written at the lcall).

## Fail-closed contracts (review C4 i16744/i16753, pF i16750, PM i16748/i16755)

- **Stack contract (N3), `TRANSFER`.** Besides a foreign return CS:IP: after the 0B0B hook
  SS must be unchanged and SP must be its entry SP + 8 (`retf 4`: the return frame and the
  source pointer, leaving the destination pointer pushed); after the +08C3 hook SS:SP must
  still be at the return IP the model pushed; after the model's `retf 0xE` SP must be
  BP - 0xC (all 18 pushed bytes consumed, BP kept), so `mov sp,bp` cannot hide a broken
  callee.
- **RETF/stack-word policy (F3, PM i16601 ruling), `STACK_BOUNDARY`.** Every stack word the
  model pushes or pops, including the IP and CS words of both modelled RETFs (+08C3's
  `retf 0xE` and the body's `retf`), lies in `[SP0 - 0x24, SP0 + 4)`; the stack contract
  keeps both RETFs at their fixed places in it. SP0 must keep the window from wrapping the
  SS offset (0x24 <= SP0 <= 0xFFFC); otherwise nothing executes. WRAPPED_SS and offset
  0xFFFF straddle reads are UNMEASURED and not modelled; inside the window word accesses are
  contiguous, and physical addresses wrap at 1 MiB (8086). The SP arithmetic itself is 16-bit
  (SP0 0xFFFC exits with SP 0x0000). No native parity is claimed for any of this.
- **Code aliasing (F1/N4), `CODE_ALIAS`.** Only the call operands are read live; every other
  body byte is assumed original. So the stack window may not physically overlap
  `[CS:263D, CS:266A)` (checked before anything executes, so the pF alias case SS = CS,
  SP0 = 0x2682, whose pushes would overwrite the +2662/+2663 opcodes, now stops), and after
  each hook every body byte except the live operands (+265E..+2661, +2664..+2665) must equal
  its entry snapshot taken from RAM. A hook may patch the near displacement: the far return
  flushes the prefetch queue, so it is fetched after the hook. Self-modified code and the
  prefetch queue are otherwise not modelled.

## Smoke test

`tests/recovered/test_overlay_u86_status_263d_full.c`:

- nominal SP0 0x8000 (lcall entry SP 0x7FDC, +08C3 entry SP 0x7FE0, exit SP 0x8004, exact
  hook stack words);
- discriminating flags and AH: SP0 0x8006 makes the SUB set AF and OF and clear CF/PF/ZF/SF
  from 0x08D7 (both hooks enter with 0x0812), AH 0x5A reaches all five words, and the final
  FLAGS are the +08C3 hook's;
- aborts at each hook, a foreign return;
- stack contract: 0B0B under- and over-pop, a changed SS, +08C3 SP moved by -2 and +2 with a
  forged matching return frame, a changed BP;
- boundary edges SP0 0x0023 and 0xFFFD (stop) and 0x0024 and 0xFFFC (run);
- code aliasing: SS = CS SP0 0x2682 and its physical alias 0x6001:0x2672, the edge SP0
  0x268D (stop) and 0x268E (runs), and a hook flipping the opcode at +2669;
- live operands patched before the run (0x0BCD:0x1234, +2766) and by the lcall hook (+2766);
- null RAM, short RAM, null hook (`ARGUMENT`).

Build script `tests/recovered/build_u86_263d_full.sh`.

Verification receipts (PM i16788, guarded host slot 2026-10-17): this branch's header,
source and smoke test blobs, extracted alone, built with
`cc -std=c11` and the executable exited 0 printing `overlay_u86_status_263d_full: ok`; a
serial CMake build of d2942649c946fcd43a2f7c6724c4f7b48d65f612, target
`poolrad_overlay_u86_status_263d_full_tests`, exited 0 and its focused CTest passed (1/1).
Static review: pF and C4 bounded static PASS on that head (i16783/i16784, i16786/i16785).
These cover the synthetic fixtures only. The +08C3 and SYSTEM 0B0B callees are opaque
hooks; there has been no native, Unicorn, oracle or full CPU/RAM parity run, and no strict
credit is claimed.

Prior-session recall: `deja` (45 s CLI timeout) and the MCP recall (which returned only this
session) gave nothing to reuse; the template is this lane's own `claude/c1r-u86-225d-full`
(pF static PASS). Reused review evidence: C4 i16744/i16753 (original-byte screen and source
review) and pF i16750.
