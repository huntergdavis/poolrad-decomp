# U3BD +0211 complete caller body: provisional C candidate

`pr_u3bd_place_0211` translates the complete local placement helper from
`GAME.OVR` unit `0x3BD`, offset `+0211..+031E`. Its only external dependency is
the far tile call at `+02FB`; a caller-supplied callback must execute that
callee, including its return and five-word cleanup. This candidate does not
add a strict full-C identity or function-count credit.

## Original boundary and ownership

The owner-supplied `GAME.OVR` is 206105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x3BD` begins at file offset `0x2E8F7`. The exact helper is
`GAME.OVR[0x2EB08,0x2EC16)`, 270 bytes / 108 contiguous instructions,
SHA-256 `b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.
All branches stay in this span. Its sole exit is `+031C RETF 8`. Its sole call
is `+02FB LCALL 0x428:0x002A`, returning to `+0300`. The loaded `2D14` body
is stable across the saved call, with only the far-call segment operand at
`+02FE/+02FF` relocated from `0x0428` to `0x0C4C`; its loaded SHA-256 is
`3514808da1ae78c2476bc9eabafdef19a30cb66ee9f6cc31ee8ae05aea9e1f6a`.

There are 18 raw near-call encodings to `+0211` in the same unit, at
`+0723,+073F,+0792,+0844,+0860,+08B3,+0935,+094E,+09CC,+09E5,+0AA6,+0AD5,+0B8C,+0BBB,+0C58,+0C87,+0D1A,+0D49`.
The versioned function ledger classifies these as aligned callers. Census
confirmed no active dedicated `+0211` branch or reservation; the separate
view, placement, and tile models retain their own scope. PM authorized this
isolated candidate after that screen. A bounded `deja` query timed out, so no
indexed session finding is claimed. The source map reused the versioned
[first-wall handoff](SLOT_A_U3BD_FIRST_WALL_TILE.md),
[view-four placement](SLOT_A_U3BD_VIEW4_PLACEMENT.md), and
[last-six tile composition](SLOT_A_U3BD_VIEW4_LAST6_COMPOSITION.md), then
checked the byte span and call operands independently.

## ABI and translation

Callers use `PUSH CS; CALL +0211`, allowing the helper's `RETF 8` to consume
the near return IP, pushed CS, and four argument words. At helper entry the
low bytes of `SS:[SP+4,+6,+8,+A]` are view, piece slice, y origin, and x
origin. The prologue saves BP and reserves 14 local bytes. The view byte
indexes three DS tables at `0x0C8C`, `0x0C96`, and `0x0CA0` for piece cursor,
column count, and row count. Origin bytes are sign-extended. The y-outer and
x-inner loops clip both coordinates to signed `0..10`. A clipped or zero
piece still advances the byte cursor, which wraps modulo 256. A positive
piece pushes x+2, y+2, item, 1, 1 and calls the loaded far target. The C
model reads locals and far pointers at each original instruction boundary,
so stack, table, piece, and callback aliases remain observable. It reads
the live far return words contiguously across an SS offset boundary and
performs the original `RETF 8` cleanup with a 16-bit SP wrap.

The callback receives CPU/RAM **after** the far-call return IP and CS have
been pushed and CPU CS:IP set to the loaded target. It may change all RAM
and CPU state, and must return to the helper's `+0300` with the callee's
argument cleanup. A missing or nonreturning callback leaves the exact
callee-entry state and reports a dependency status. The original's last
arithmetic FLAGS writer on every normal return is a CMP (`+0261` for an
empty row loop, otherwise `+0310`), so AF is defined at the helper return.
Undefined intermediate FLAGS, including AF after XOR and SF/ZF/AF/PF after
MUL, are retained but are not asserted as source-defined callee inputs.

## Verification and limit

The focused C11 test checks empty and clipped loops, byte-cursor wrap, a
five-word callback frame, callback effects, live far-return aliasing, and
RETF stack wrap. The original-byte differential runs 66 synthetic states through the 270 source
bytes and C model with the **same explicit no-effect tile callee**. It
compares all 1 MiB of RAM and all 14 CPU words at return. All 108 instruction
starts and both outcomes of all nine conditional branches are reached;
20 tile callbacks are exercised. This proves the caller body under that
callback contract, not the real tile callee.

The read-only saved-game prefix checker starts from a same-run natural
`2D14:0860` pre-call capture. It applies the documented near CALL's return-IP
push, runs the C candidate, and stops at the first `+02FB` far call. All
640 KiB and CPU14 match the saved native `+02FB` pre-call state after
accounting for the model's four predicted far-call return bytes. The source
capture hashes are `6d724167613df028dc0a1d70368f18d356ea33e59efd8cd64ded9a6ffacf6639`
before `+0860` and `bb427b275705cf61112d6c3265f7ec89d5804557cd701fc8ee3a41d39c88fe11`
before the first tile call. The same run records two native tile calls and
an immediate `+0863` return, but this checker does not replay the callee or
assert that whole-function return. Native captures are 640 KiB, and the
real tile callee has graphics state effects. Full 1 MiB natural entry/return
RAM, CPU14, dependency parity, and independent identity review remain open.

With the original media and saved run under ignored `resources/`, focused
checks are:

```sh
cmake -S . -B resources/build-u3bd-0211 \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u3bd-0211 \
  --target poolrad_overlay_u3bd_place_0211_tests -j 1
ctest --test-dir resources/build-u3bd-0211 \
  -R '^poolrad_overlay_u3bd_place_0211_tests$' --output-on-failure
PYTHONPATH=resources/pydeps python3 \
  tests/recovered/check_overlay_u3bd_place_0211.py \
  --original /path/to/ignored/GAME.OVR
python3 tools/replay/u3bd_place_0211_saved_prefix.py \
  --original /path/to/ignored/GAME.OVR \
  --capture-dir /path/to/ignored/slot-a-u3bd-art-effect-p5-032
```

No original media, RAM image, artwork, or debugger transcript is tracked.
