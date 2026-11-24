# OVL_0038_0233 (unit 0x38, slot 7): static evidence, native capture held

**Verdict: STATIC EVIDENCE ONLY.** Bounds, control flow, callee sites, stack balance under cited contracts, fixups and
record accesses are measured and audited below. **No native capture, no store replay, no C composition, no credit, no
parity, no function identity or count, no acceptance claim.** The debugger capture path is held by the eb5d0b5d
controller hold (the t4492 review-candidate chain). Nothing in this slice runs that chain.

Base: `origin/main` `b7e74f48fc91fa8406f970cdf62e2954d860a064` (pinned for this slice; the 017F slice used
`d68e6aff`, and `origin/main` has moved since). Branch `evidence/ovl-0038-0233`, local, not pushed.

Ownership: sole owner per the coordinator assignment. The backlog box at `docs/re/U9E_DISPATCH_HANDLER_BACKLOG.md:62`
stays unchecked.

## Pinned owner media (hash only)

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

Unit 0x38: code at `GAME.OVR` offset **60507**, code size 14510, stub segment **0041** (from `tools/overlay/overlay_inventory.py`
at the pinned base). Handler `+0233` = file offset **61070**, length **324**, which ends at `+0377`, where the slot-8 handler
begins. Body SHA-256 `18b0cf6db70feb85e97737e9a9cccec0d5b66fec55423fc06a2638288990aa2c`.

## Bounds and control flow (measured: 119 instructions tiling 324 bytes)

- Linear sweep and recursive descent from `+0233` both cover **324 of 324** bytes. No undecodable bytes, no dead bytes.
- **One return:** `RETF 0Ah` at `+0374`, reached through the epilogue `mov sp,bp; pop bp` at `+0371`. Every path reaches
  `+0371`.
- Frame: `push bp; mov bp,sp; sub sp,0Eh` (locals below `bp`: a delta byte at `bp-1`, an accumulator at `bp-2`, a loop
  index at `bp-3`, and a 14-byte pointer buffer at `bp-0Eh`).
- Far record pointer: the handler loads `ES:DI` from its argument words `[bp+0Ch]` (offset) and `[bp+0Eh]` (segment), so
  the record is a far pointer passed by the dispatcher. The other forwarded words are unused.

## Far calls and paths

| Site | Target | Pushed before the call | Note |
| --- | --- | ---: | --- |
| `+0249` | `0AF8:0B0B` (Pascal copy, call 1) | 6 words | `[bp+0Eh]`, `[bp+0Ch]`, `SS:bp-0Eh` pointer, callback `CS:0228` (`mov di,0228h`, `push cs`, `push di`) |
| `+0254` | `00BA:16EC` (window message, call 2) | 2 words | `AL=0Ah`, `AL=1`, each pushed as a word |
| `+027D` | `00B0:0052` (unit 9E `+0E52`, list append, call 3) | 6 words | `[bp+0Eh]`, `[bp+0Ch]`, `3Eh`, `3Ch`, `FFh`, `1` |

Paths, decided by the record byte at `+14h`, which the handler increments on every entry (`inc byte ptr es:[di+14h]`
at `+025C`) before testing it:

- **Path C (count at least 14h):** call 3 at `+027D`, then the epilogue.
- **Path A (count below 0Fh):** no call 3; the epilogue. The count is not otherwise changed here.
- **Path B (count from 0Fh to 13h):** the eight-index loop, a division, and the conditional tail writes below, then the epilogue.

Calls 1 and 2 run on every path. Stack balance under the cited contracts holds on each path: after call 1, 6 − 2 = 4
words remain; call 2 pushes 2 and pops 6; on path C call 3 pushes 6 and pops 6, leaving 0.

## Path B arithmetic (read from the decode; not modelled, not checked natively)

- The loop runs for index 0 to 7 (`cmp byte ptr [bp-3],7` at `+0325`, back-edge to `+02B6`). For each index it reads the
  8-byte array at record `+96h`, and only a positive element contributes.
- Index 2 adds the element times (count − 15). Other indices add the element, doubled when count is above 0Fh.
- A divisor guard makes a zero accumulator 1 (`+032B` to `+0331`), then `idiv cx` at `+0342` divides the delta by it.
- The tail writes `+32h` and `+11Bh` (both `+= delta`) when count is below 11h, or when count is at least 11h and the
  byte at `+98h` is positive.

## Record displacements (measured)

Accessed with `es:[di + disp]`, the exact set is {`14h`, `32h`, `96h`, `98h`, `B1h`, `11Bh`}; `96h` is the base of the
8-byte index array. The record's name and layout are not established here.

## Loader fixups (measured with the repo helper)

The segment words of the three far calls, `+024C`, `+0257` and `+0280`, are fixup-backed (`lcall_segment`). Their values
are loader-assigned, and the stored values read `0AF8`, `00BA` and `00B0`.

## Callee contracts (cited from `docs/re`; not verified natively here)

| Target | Popped (cited contract) | Cited source |
| --- | ---: | --- |
| `0AF8:0B0B` | 2 words (`RETF 4`) | `OVL_0038_0B1B_CPU.md` |
| `00BA:16EC` | 6 words (`RETF 0Ch`) | `OVL_0038_070F_CPU.md`, `OVL_0038_09C0_CPU.md`, `RES_00BA_16EC_CLOSURE_IDENTITY.md` |
| `00B0:0052` | 6 words (`RETF 0Ch`) | `U9E_LIST_APPEND_0E52_CPU.md`, `U38_ROLL_APPEND_1730_CPU.md` |

These contracts are the same ones the 017F note cites. They are not verified natively in this slice.

## Dispatch link (cited, not re-measured)

`docs/re/OVL_009E_0000_HANDLERS.md` lists slot 7 at table `6844h` with stub `0041:0043`, verdict named gap "uncredited
handler OVL_0038_0233", closure 1591 bytes in 5 bodies. The dispatch frame is the same one described in the 017F note.

## Static audit and mutant check

`tests/replay/audit_ovl0038_0233_static.py --dos-dir <poolrad-decomp checkout>/resources/DOS/POOLRAD`

| Run | Result |
| --- | --- |
| baseline | 16 of 16 PASS, exit 0 |
| `exe-byte` | 1 FAIL (START hash) |
| `retf-count` | 2 FAIL (body hash; single RETF 0Ah) |
| `lcall-seg` | 2 FAIL (body hash; far-call targets) |
| `push-nop` | 3 FAIL (body hash; pushes per call 6, 2, 5; stack balance) |
| `guard-removed` | 3 FAIL (body hash; 122 instructions; divisor guard) |
| `callback` | 2 FAIL (body hash; callback set and pushed) |
| `loop-bound` | 2 FAIL (body hash; loop bound 7) |

The stack-balance check uses the cited contracts as constants. It does not test those contracts; only a native capture
can.

## Native capture: held, not run

The native capture for this handler (a forced call through the dispatcher frame, a whole-trace store replay, and a
C composition against the native entry state) is **not started**. It is held by the eb5d0b5d controller hold. Nothing here
runs the controller, the supervisor chain, the candidate session script, or any guest, pty or DOSBox process.

## Limits

- Static only. Nothing here is a native result, a parity claim, a function identity, a count, or a credit.
- Callee bodies are not modelled. The contracts are cited from other notes and are not re-derived here.
- The record layout and its name are not established. Path B's arithmetic is read from the decode and not checked
  against a model or a native run.
- Loader segment values are session-dependent (fixup-backed).
- The base moved from `d68e6aff` to `b7e74f48`. This note pins `b7e74f48`.
- The free-run verdicts for this handler (`docs/re/free_runs/OVL_0038_0233/`) are marked as fabricated citations and are
  not used as evidence.
- The local commit carries the repository's configured identity. That identity is a placeholder, not publishable, and
  the commit must be re-authored with real attribution before any publication.
