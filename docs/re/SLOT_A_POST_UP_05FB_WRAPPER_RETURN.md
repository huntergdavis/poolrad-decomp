# Saved slot-A post-Up display-wrapper return

**Classification:** bounded C parity for U3BD `+05FB POP BP; +05FC RETF`
after the external resident transfer returns. The next reached caller IP is
`+0DAE`. This model does not execute the resident transfer or the outer
`+0DAE` return, and makes no full-view or framebuffer parity claim.

Before work, `deja "Poolrad post-Up U3BD +05FB wrapper return +0DAE
resident display transfer"` found no matching indexed session. The prior
[flush-entry gate](SLOT_A_POST_UP_FLUSH_ENTRY_GATE.md) supplies the reached
far-call frame and the unresolved resident boundary; the older
[display-transfer audit](SLOT_A_U40C_DISPLAY_TRANSFER.md) identified the
wrapper. The native stops below independently establish the return.

## Original-byte and caller proof

Ignored DOS v1.3 `START.EXE` SHA-256:
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Ignored `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Pinned ignored slot-A save SHA-256:
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

| Reached unit span | Original `GAME.OVR` range | Bound | SHA-256 |
| --- | --- | --- | --- |
| `+05FB..+05FD` | `[0x2EEF2,0x2EEF4)` | 2 bytes / 2 instructions | `05303d46935f1d6bc7f492470521a209a51050b261d452a788f042866b5d72cd` |
| next `+0DAE..+0DB4` | `[0x2F6A5,0x2F6AB)` | 6 bytes / 3 instructions, not modeled here | `36916f0bed44fe934eeabdd0115011a4de8eab35e6758714d66c7343f824ed3d` |

Both spans equal loaded `2D14` code in the post-Up RAM snapshot. The
`+05FB` stop follows the relocated `0C88:1B52` resident far call from
`+05F6`; the resident implementation is owned separately. The wrapper's
stack at `SS=1D16`, `SP=3EC2` held saved BP `3EE0` at `+3EC2`, return IP
`0DAE` at `+3EC4`, and return CS `2D14` at `+3EC6`. These words were read
from the guest RAM image at the actual breakpoint.

## Same-run native and C parity

The private debugger restored pinned slot A to the visible
`(15,4,N; 10:50)` frame, armed only `BP 2D14:05FB` in the input loop,
and sent exactly one Up. It stopped at `2D14:05FB` before `POP BP`:
`AX=0100`, `BX=3EA6`, `CX=0000`, `DX=03CF`, `SI=1166`, `DI=4EC0`,
`BP=SP=3EC2`, `SS=1D16`, `DS=14A0`, `FLAGS=7246`. The debugger then
armed only `BP 2D14:0DAE` and resumed the same run. At `2D14:0DAE`,
`BP=3EE0`, `SP=3EC8`, `AX=0100`, `DI=4EC0`, `FLAGS=7246`.

Both conventional RAM images, `0x00000..0x9FFFF`, were byte-identical,
SHA-256
`b8d808d1469791646d57003438b094f0d9fee0459442021c3747a5635025bc8c`.
The C model from the first snapshot reaches the second CPU state and
matches **655,360/655,360 RAM bytes**. Synthetic strict tests also cover
the saved-BP pop, far-return word order, unchanged AX/DI/flags, and
missing first or later stack words. A missing far-return word leaves the
completed `POP BP` visible at `+05FC`.

## Reproduction

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
p = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
r = Path('resources/oracle-runs/slot-a-post-up-exit-p9-119/post-up-05fb-ram-private.bin').read_bytes()
for lo, hi in ((0x05FB, 0x05FD), (0x0DAE, 0x0DB4)):
    original = p[0x2E8F7 + lo:0x2E8F7 + hi]
    loaded = r[0x2D14 * 16 + lo:0x2D14 * 16 + hi]
    print(hex(lo), len(original), sha256(original).hexdigest(),
          sha256(loaded).hexdigest(), original == loaded)
PY
cmake -S . -B resources/build-post-up-05fb-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-05fb-p9 --target \
  poolrad_overlay_u3bd_post_up_05fb_wrapper_return_tests
ctest --test-dir resources/build-post-up-05fb-p9 \
  -R '^poolrad_overlay_u3bd_post_up_05fb_wrapper_return_tests$' \
  --output-on-failure
resources/build-post-up-05fb-p9/poolrad_overlay_u3bd_post_up_05fb_wrapper_return_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/post-up-05fb-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/post-up-0dae-ram-private.bin
```

Original media, save, debugger traces, screenshots, and RAM images remain
ignored under `resources/`.
