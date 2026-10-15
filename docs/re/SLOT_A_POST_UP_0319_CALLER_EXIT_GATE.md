# Saved slot-A post-Up caller exit at unit `0x3BD:+0319`

**Classification:** bounded three-instruction far-return epilogue. The C
model reproduces the reached stack/register transition and leaves the same-run
640 KiB RAM image unchanged. This ends at the caller's `2D14:0938`; it does
not model the subsequent object-to-frame transfer or claim a complete view or
full-C parity.

Before analysis, `deja "poolrad slot-A post-Up U3BD +02C4 +0300 next UI
caller boundary p9 p5 object-to-frame ownership"` was run with a 30-second
bound. That invocation stalled and was stopped; no recall result was reused.
The checked-in [fourth tile-call gate](SLOT_A_POST_UP_02C4_FOURTH_TILE_CALL_GATE.md),
[inner-loop gate](SLOT_A_POST_UP_0300_TILE_LOOP_GATE.md), and
[outer-loop gate](SLOT_A_POST_UP_030E_OUTER_LOOP_GATE.md) supplied the adjacent
addresses. The branch and far return below were measured in a new native run.

## Original body and frame

Ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

Unit `0x3BD:+0319..+031F` is original `GAME.OVR [0x2EC10,0x2EC16)`:
**6 bytes / 3 aligned 16-bit instructions**, SHA-256
`a6a5a3f2477842978f1dece0f0481b66d983206511415d3dbb6f1e3954ee00f0`.
The loaded `2D14:0319..031F` bytes have the same hash.

| Offset | Operation | Reached effect |
| --- | --- | --- |
| `+0319` | `MOV SP,BP` | Discard the caller's local stack area. |
| `+031B` | `POP BP` | Restore the saved BP word. |
| `+031C` | `RETF 8` | Pop far return IP/CS and remove four argument words. |

The C gate checks each required SS word at its first reached read. A missing
saved BP returns `BOUNDARY` after `MOV SP,BP`; a missing far return word
returns it after `POP BP`. Guest offsets wrap at 16 bits. `RETF 8` commits
CS, IP, and SP only when both target words are available. AX and FLAGS are
preserved, and this body writes no guest RAM.

## Fresh native exactly-one-Up exit

A dedicated private DOSBox-X heavy-debug replay loaded the pinned save at
visible `(15,4,N; 10:50)`. The debugger breakpoint list was empty there;
only `BP 2D14:0319` was armed, then exactly one gameplay Up was sent. Native
stopped at `2D14:0319` with `SS=1D16`, `BP=3EBA`, `SP=3EAC`, `AX=6`, and
`FLAGS=7246`. The live local inner word and limit were both 4, while the
outer word and limit were both 6. This stop establishes the reached equal
exit after the fourth tile call; no cross-run RAM snapshots are paired.

The same-run stack held saved BP `3EE0` at `SS:[BP]`, return IP `0938` at
`SS:[BP+2]`, and return CS `2D14` at `SS:[BP+4]`. The private 640 KiB RAM
SHA-256 at `+0319` was
`a6ba6593261576270826513572846b090b6bd0b1b04b3391b7d3efd58723602b`.
The breakpoint was replaced with `BP 2D14:0938`, and `RUN` stopped directly
there. Native then had `BP=3EE0`, `SP=3EC8`, `AX=6`, `FLAGS=7246`, and the
same full RAM hash: **zero changed RAM byte values** across the epilogue.

The private test starts from the exact `+0319` RAM, runs the C gate, checks
the return tuple, and compares **655,360/655,360 RAM bytes** with the direct
native `+0938` dump. The synthetic strict test checks success, missing saved
BP, missing return IP or CS, NULL SS at its first reached read, and 16-bit
stack-offset wrap. It does not assign behavior to the caller after `+0938`.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
span = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()[0x2EC10:0x2EC16]
ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, 0x0319))
assert len(ins) == 3 and ins[-1].address + ins[-1].size == 0x031F
print(len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-0319-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-0319-p9 --target \
  poolrad_overlay_u3bd_post_up_0319_exit_tests
ctest --test-dir resources/build-post-up-0319-p9 \
  -R '^poolrad_overlay_u3bd_post_up_0319_exit_tests$' --output-on-failure
resources/build-post-up-0319-p9/poolrad_overlay_u3bd_post_up_0319_exit_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/0319-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/0938-return-ram-private.bin
```

At the loaded playable frame, the native command sequence was
`BP 2D14:0319`, `BPLIST`, `RUN`, one Up, `MEMDUMPBIN 0000 0000 A0000`,
`BPDEL *`, `BP 2D14:0938`, `RUN`, and a second `MEMDUMPBIN`. Original
media, save, screenshots, debugger trace, and RAM dumps remain ignored
under `resources/`.
