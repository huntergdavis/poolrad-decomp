# OVL_0038_017F (unit 0x38, slot 4): static evidence and native blocker

**Verdict: BLOCKED for the native steps.** Static facts are measured and audited below. The native forced-call capture,
the whole-trace store replay, and the C composition checked against a native entry state are **not run**. The native
capture uses the debugger chain under `poolrad-compare-if-true-scratch`, which carries the header "Review candidate only.
Starts a game if executed; do not run before clearance." That clearance is not on record, so the capture waits for
coordinator clearance. **Evidence only. No parity, no function identity or count, no credit, no acceptance claim.**

Base: `origin/main` `d68e6aff3f4abc01f6cb12f17065baafda3cc03a` (confirmed by the coordinator; the owner's pin
`05db4ac9` is an ancestor of this base, 765 commits behind it). Branch `evidence/ovl-0038-017f`, not pushed.

Ownership: sole slice owner per the coordinator assignment. The backlog box at `docs/re/U9E_DISPATCH_HANDLER_BACKLOG.md`
is left unchecked; checking it is for the reviewer after the native evidence lands.

## Pinned owner media (hash only; no bytes in this note)

| Artifact | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

Unit 0x38 (from `tools/overlay/overlay_inventory.py` on this base): code at `GAME.OVR` offset **60507**, code size
**14510**, stub segment **0041**. The handler is unit offset **+017F**, which is file offset **60890**, length **68**
(`[60890, 60958)`). Body SHA-256 `62eda393ae8de0e238aba4c49606f91f4604171887ff26c3dd2695a381139fe8`.

## The body (measured; 31 instructions tiling 68 bytes)

- Entry: `push bp; mov bp,sp; sub sp,10h`. Then the frame is built for three far calls.
- Call 1, `LCALL 0AF8:0B0B`, with six words pushed: `[bp+0Eh]`, `[bp+0Ch]`, the pointer `SS:(bp-10h)`, and the far
  callback `CS:016F` (`mov di,0x16f`; `push cs; push di`).
- Call 2, `LCALL 00BA:16EC`, with two words pushed: `AL=0Ah` and `AL=1`, each as a word.
- Call 3, `LCALL 00B0:0052`, with six words pushed: `[bp+0Eh]`, `[bp+0Ch]`, `7`, `A8C0h`, `FFh`, `1`.
- Epilogue: `mov sp,bp; pop bp; RETF 0Ah` (five argument words popped by the caller's convention).
- No loops, no internal near calls, no indirect transfers. The only lcall-segment words are at unit offsets
  **+0198, +01A3, +01BB** (call start + 3).

Audited by `tests/replay/audit_ovl0038_017f_static.py` (13 checks, all pass at baseline).

## Loader fixups (measured with the repo helper)

The three segment words of the far calls are in unit 0x38's fixup set (`lcall_segment`), so their stored values are
loader-assigned. The stored values read `0AF8`, `00BA` and `00B0`. The native trace is the only source for the values
used in a session.

## Callee contracts (cited from `docs/re`; not verified natively here)

| Call | Target | Pushed here | Popped by callee (cited contract) | Cited source |
| --- | --- | ---: | ---: | --- |
| 1 | `0AF8:0B0B` (Pascal copy / StrLoad) | 6 words | 2 words (`RETF 4`) | `OVL_0038_0B1B_CPU.md`, accepted contract |
| 2 | `00BA:16EC` (window message) | 2 words | 6 words (`RETF 0Ch`) | `OVL_0038_070F_CPU.md`, `OVL_0038_09C0_CPU.md`, `RES_00BA_16EC_CLOSURE_IDENTITY.md` |
| 3 | `00B0:0052` (unit 9E `+0E52`, list append) | 6 words | 6 words (`RETF 0Ch`) | `U9E_LIST_APPEND_0E52_CPU.md`, `U38_ROLL_APPEND_1730_CPU.md` |

Under these contracts the stack balances: after call 1, 6 − 2 = 4 words remain; call 2 pushes 2 more and pops 6 (4 + 2);
call 3 pushes and pops 6. So the leftover is 0 after the last call, and the arguments of call 2 include four words that
call 1 pushed. That makes the callee's reads of its own frame depend on what is on the stack. **Only a native trace can
confirm this** (the composition needs the native post-call state at every call boundary).

## Dispatch frame (cited, not re-measured here)

`OVL_009E_0000` is 38 bytes, `RETF 0Ch`, stub `00B0:0025` (`docs/re/OVL_009E_0000_HANDLERS.md`). It forwards four words
and one byte, uses the index byte to form `DI = 4 * index`, and calls `lcall [di+6828h]`. Slot 4's pointer is at
`6838h` and points to stub `0041:0039`. The forced call would enter at `00B0:0025` with the five forwarded words and a
return address, which is the frame callers build.

## What the native capture must establish (not done)

1. Dispatcher entry at `00B0:0025` with the forwarded frame, and the path to stub `0041:0039` and into `+017F`.
2. The three call sites' actual stack words and callee pop counts, from observed `SP` at each callee entry and return.
3. Whether the callback `0AF8:0B0B` calls back into `CS:016F`.
4. Final `SP`, registers, and RAM delta for the store replay (0 unattributed required), and a C composition from the
   native entry state with 0 diffs.

## Static audit and mutant check (this note's only computed evidence)

`tests/replay/audit_ovl0038_017f_static.py --dos-dir <poolrad-decomp checkout>/resources/DOS/POOLRAD`

| Mutant (decoded input) | Result |
| --- | --- |
| none (baseline) | 13 of 13 PASS, exit 0 |
| `exe-byte` (START.EXE byte flipped) | 1 FAIL (START hash) |
| `body-byte` (retf opcode byte flipped) | 3 FAIL (body hash, instruction count, final RETF) |
| `lcall-seg` (first call segment byte flipped) | 2 FAIL (body hash, pinned targets) |
| `push-count` (second call's push replaced by NOP) | 3 FAIL (pushed words, stack balance, body hash) |

The stack-balance check uses the cited contracts as constants. It does not test those contracts; the native capture does.

## Limits

- Static only. Nothing here is a native result. No credit, no parity, no function identity or count.
- The callee bodies are not modelled. The contracts are cited from other docs and are not re-derived.
- The loader segment values for the three calls are session-dependent (fixup-backed).
- The native capture, store replay and native-entry composition are not started. They wait for clearance.
- The body hash is recorded; the body's original bytes are not printed in any tracked file.
