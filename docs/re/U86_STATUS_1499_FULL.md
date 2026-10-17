# U86 +1499, a status-text wrapper, as a strict full-state draft (Owner #639)

GAME.OVR unit 0x86 + 0x1499 (`OVL_0086_1499`, public entry slot 24, `game_evidenced`),
`[1499,14C6)` = file `[0x284B1,0x284DE)`, 45 bytes, 25 instruction starts, sha256
`b15a1a23510e8a7dcba47263103056c0cc60f2fe005b31658c45b6cc45425570`, ending in a plain
`retf`; a zero pad byte at +14C6 precedes the next prologue at +14C7. CS literal pool
`[148D,1499)` = a Pascal string of length 0x0B (12 bytes with the length byte), sha256
`171568e404d83c4691866d5fdbe9e293025e63dc686b9177f857454861d7507b`, loaded only by this
body; its text is not reproduced here (original-media rule) and the model never reads it.
Owner screen (relative CLEAR, body and CS pool, all thirteen queues): pT i13769, pF i13771,
pS i13772, p5 i13774, p9 i13776, pC i13779, pR i13784, p0 i13818, pP i13822, pD i14186, pH
i14194, pG i14450, pW i15479 (summary i15481); PM source GO i15517; clean rewrite GO
i16827. It is an exact twin of the merged U86 +263D (`docs/re/U86_STATUS_263D_FULL.md`):
the same frame and callees, every code offset -0x11A4.

| offset | effect |
| --- | --- |
| +1499..+149C | `push bp; mov bp,sp; sub sp,0xC` (SUB flags exact) |
| +149F..+14AE | `mov al,[0x681B]; push ax`, then `mov al,0; push ax` four times (AH carried) |
| +14AF..+14B9 | `lea di,[bp-0xC]; push ss; push di; mov di,0x148D; push cs; push di; lcall 0xAF8:0x0B0B`: the runtime string load copies the CS pool into the 12-byte temp `SS:[bp-0xC]` and `retf 4` leaves that pointer pushed |
| +14BE..+14BF | `push cs; call rel16` (original displacement 0xF401 giving +08C3): the unit-local routine (462 bytes, `retf 0xE`) with the string and the five bytes |
| +14C2..+14C5 | `mov sp,bp; pop bp; retf` |

`src/recovered/overlay_u86_status_1499_full.c` translates the 25 instructions over
registers, FLAGS and 1 MiB of RAM in source order, exactly as the merged +263D model does
(same frame and callees; every code offset -0x11A4, pool and displacement included). The
lcall is raw:
the far target is read live from CS:+14BA (offset) and CS:+14BC (segment) before the
lcall's own pushes (0x0B0B and 0x0AF8 by original default, a loader fixup); the model pushes
the caller CS and +14BE and enters it; the hook runs the callee including its `retf 4`. The
near-far call pushes CS, reads the displacement live at CS:+14C0 (+08C3 by original
default), pushes +14C2 and enters the target; the hook supplies the callee's effects except
its return and the model performs its `retf 0xE` from the live stack. Both callees are
opaque hooks: no shared ABI or callee model is added or claimed. Writes AL, DI, FLAGS (the
SUB) and SP/BP/IP/CS; ES is untouched. DS:0x681B is read only; no ownership claim. The entry
IP is neither read nor checked (IP is first written at the lcall).

## Fail-closed contracts (the +263D review findings C4 i16744/i16753, pF i16750, PM i16748/i16755)

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
  `[CS:1499, CS:14C6)` (checked before anything executes, so the alias case SS = CS,
  SP0 = 0x14DE, whose pushes would overwrite the +14BE/+14BF opcodes, now stops), and after
  each hook every body byte except the live operands (+14BA..+14BD, +14C0..+14C1) must equal
  its entry snapshot taken from RAM. A hook may patch the near displacement: the far return
  flushes the prefetch queue, so it is fetched after the hook. Self-modified code and the
  prefetch queue are otherwise not modelled.

## Smoke test

`tests/recovered/test_overlay_u86_status_1499_full.c`:

- nominal SP0 0x8000 (lcall entry SP 0x7FDC, +08C3 entry SP 0x7FE0, exit SP 0x8004, exact
  hook stack words);
- discriminating flags and AH: SP0 0x8006 makes the SUB set AF and OF and clear CF/PF/ZF/SF
  from 0x08D7 (both hooks enter with 0x0812), AH 0x5A reaches all five words, and the final
  FLAGS are the +08C3 hook's;
- aborts at each hook, a foreign return;
- stack contract: 0B0B under- and over-pop, a changed SS, +08C3 SP moved by -2 and +2 with a
  forged matching return frame, a changed BP;
- boundary edges SP0 0x0023 and 0xFFFD (stop) and 0x0024 and 0xFFFC (run);
- code aliasing: SS = CS SP0 0x14DE and its physical alias 0x6001:0x14CE, the edge SP0
  0x14E9 (stop) and 0x14EA (runs), and a hook flipping the opcode at +14C5;
- live operands patched before the run (0x0BCD:0x1234, +15C2) and by the lcall hook (+15C2);
- null RAM, short RAM, null hook (`ARGUMENT`).

Build script `tests/recovered/build_u86_1499_full.sh`. **Not compiled or run** at this
head. No strict credit: the +08C3 and SYSTEM 0B0B bodies are hooks, and callee and native
oracles are open.
