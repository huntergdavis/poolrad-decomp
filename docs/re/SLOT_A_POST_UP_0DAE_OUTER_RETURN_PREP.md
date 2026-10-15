# Saved slot-A post-Up outer-return preparation

**Classification:** bounded static C model of U3BD `+0DAE..+0DB4`.
The native input at `+0DAE` is measured; the C successor is a stack-derived
**prediction pending a native destination stop**. No full-view, UI/frame,
or native-to-C successor parity claim is made.

Before work, `deja "Poolrad post-Up U3BD +0DAE MOV SP BP POP BP RETF 6
outer caller return"` found no matching indexed session. The
[flush-entry frame](SLOT_A_POST_UP_FLUSH_ENTRY_GATE.md) and
[wrapper-return proof](SLOT_A_POST_UP_05FB_WRAPPER_RETURN.md) supply the
reached caller path. The separate resident display transfer remains outside
this model.

## Original span and measured input

Ignored DOS v1.3 `START.EXE` SHA-256:
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Ignored `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Pinned ignored slot-A save SHA-256:
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

Original `GAME.OVR` `[0x2F6A5,0x2F6AB)` maps to U3BD
`+0DAE..+0DB4`: 6 bytes, 3 instructions, SHA-256
`36916f0bed44fe934eeabdd0115011a4de8eab35e6758714d66c7343f824ed3d`.
The loaded `2D14` span is byte-identical. The three instructions are
`MOV SP,BP`, `POP BP`, and `RETF 6`.

The hash-pinned native exactly-one-Up run stopped at `2D14:0DAE` after
the separate resident transfer and local wrapper return. Its measured
registers include `SS=1D16`, `BP=3EE0`, `SP=3EC8`, `AX=0100`,
`DI=4EC0`, and `FLAGS=7246`; conventional RAM SHA-256 was
`b8d808d1469791646d57003438b094f0d9fee0459442021c3747a5635025bc8c`.
The stack words read from that RAM at `SS:3EE0`, `:3EE2`, and `:3EE4`
are `3EF0`, `022E`, and `2E35` respectively. Executing the three
original instructions therefore predicts `CS:IP=2E35:022E`, `BP=3EF0`,
and `SP=3EEC`, with no RAM write. The C model reads the destination
from the supplied stack; it does not hardcode these values.

The established [first-view handoff](SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md)
maps loaded `2E35` to unit `0x3B4` (original base `GAME.OVR[0x2DB87]`).
Static code at `0x3B4:+022E..+0230` is a two-byte jump to `+029C`,
SHA-256
`0dc9570b3f951ec51f3eb6cada4814932492c3dfcdf96a5af266585bddfe32e0`.
At `+029C..+02A0` the four-byte, three-instruction epilogue is
`MOV SP,BP; POP BP; RETF`, SHA-256
`d938592002da1e8f84eae35df938d5b693d827276f6e045d142340640128831a`.
Both spans equal the loaded `2E35` image. The measured `+0DAE` RAM
holds `SS:3EF0=3EF8`, `SS:3EF2=3497`, and `SS:3EF4=3582`; these
predict a later return to `3582:3497`. The planned native stop must
first confirm `2E35:022E` and then this distinct UI return path before
either continuation is modeled or called parity.

Synthetic strict tests cover stack reset to BP, far-return word order,
the six-byte callee argument cleanup, unchanged AX/DI/flags and RAM,
and missing first/later stack words. A private test feeds the actual
`+0DAE` RAM input to C and checks the predicted state. A separate
native stop at the reached successor is still required for parity.

## Reproduction

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
p = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
r = Path('resources/oracle-runs/slot-a-post-up-exit-p9-119/post-up-0dae-ram-private.bin').read_bytes()
original = p[0x2F6A5:0x2F6AB]
loaded = r[0x2D14 * 16 + 0x0DAE:0x2D14 * 16 + 0x0DB4]
print(len(original), sha256(original).hexdigest(), original == loaded)
PY
cmake -S . -B resources/build-post-up-05fb-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-05fb-p9 --target \
  poolrad_overlay_u3bd_post_up_0dae_outer_return_tests
ctest --test-dir resources/build-post-up-05fb-p9 \
  -R '^poolrad_overlay_u3bd_post_up_0dae_outer_return_tests$' \
  --output-on-failure
resources/build-post-up-05fb-p9/poolrad_overlay_u3bd_post_up_0dae_outer_return_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/post-up-0dae-ram-private.bin
```

Original media, save, debugger output, screenshots, and RAM images remain
ignored under `resources/`.
