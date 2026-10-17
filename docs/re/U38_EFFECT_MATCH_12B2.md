# Unit 0x38 entry 41: conditional effect update at +0x12B2

## Provenance and ownership

This is PM's selected disjoint candidate after +0x1276 and +0x11F9. The
target-specific `deja "poolrad U38 +12B2 entry41 calls +0C06 effect strict C
parity"` query timed out after 20 seconds without a session result.
The caller graph reuses [the U38 effect-table installer](U38_EFFECTS_INSTALL_31A2.md),
[the dispatcher decode](OVERLAY_UNIT38_EFFECT_ENTRIES.md), and
[the merged +0C06 leaf's evidence](FUNCTION_LEDGER.md). The routine bytes,
fixup, branch paths, and parity below were independently remeasured.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

U38 public entry 41 is the complete 73-byte code body `[+0x12B2,+0x12FB)`,
`GAME.OVR` file `[0xFF0D,0xFF56)`, SHA-256
`fdc58872484dc92452c0eb042ec51727f3543e989c6fed48c30876899ddc061b`.
All 28 instructions decode contiguously. The wrapper has no loader fixup;
its sole call is the near call at `+0x12E8` to U38 `+0x0C06`. That verified
callee is `[+0x0C06,+0x0C15)`, 15 original bytes, SHA-256
`435bd9fac807d24f064b6bf223817b188e737a5b6ddb9a8b37d5635842a52fe8`.

The original U38 +31A2 installer at +3415 writes the public stub
`0041:00ED` to DS `[0x68EC]/[0x68EE]`, effect-table item 49. Its segment
immediate is a loader fixup at +3419. U9E +0000 dispatches by an indirect
`LCALL [DI+0x6828]` after scaling its index. This is a fixup-backed installed
pointer and a decoded indirect call path. Neither a constant item-49 call
nor a reached game frame for this entry has been measured. The installer
does not execute the entry. Ownership therefore remains game-evidenced and
provisional; strict identity/count credit is withheld.

## Complete behavior

The wrapper reads byte `+3` of far pointer B at `[BP+8,+0xA]`, isolates bit
four, and stores the zero-or-one result in its SS local byte. It reads byte
`+0x10E` of far pointer A at `[BP+0xC,+0xE]`. If equal, it pushes all five
original arguments and CS, then near-calls the verified U38 +0C06 body.
That body increments DS bytes `[0x6816]` and `[0x6822]` and returns `RETF 10`
to the wrapper's far-compatible frame. If unequal, the wrapper decrements
DS `[0x6822]` then `[0x6816]`. Both paths exit with `RETF 10` and exact
register, stack, and defined flag state. The C model composes the callee,
including its push/pop BP and far return, rather than substituting only
its two state writes.

## Verification and limits

`tests/recovered/check_overlay_u38_effect_match_12b2.py` checks the pinned
media hashes, entry indexes, exact extents and original code hashes, absence
of wrapper fixups, and the sole near-call site. Twelve cases execute both
original byte bodies in Unicorn and compare all 1 MiB RAM and 14 CPU fields
with C. They cover both equal and unequal branches, increment/decrement
overflow and zero, DS/SS aliasing, and an odd near-boundary stack frame.

Four DOSBox-X normal-core synthetic far-entry frames also execute both
original bodies. All 14 CPU words, 52 stack bytes, and DS `[0x6816]` and
`[0x6822]` match C. Generated COM, copied original bytes, logs, and capture
stay in ignored `resources/u38-12b2-parity/native/`. Capture SHA-256:
`f595d0512a87f3188a78275b224bcb1b2013ffc7e5e2decbff8b6224311b8cf1`.
Focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest passes.

The native frames are synthetic direct entries, not item-49 dispatches or
reached gameplay. This branch makes no strict identity or count claim.
