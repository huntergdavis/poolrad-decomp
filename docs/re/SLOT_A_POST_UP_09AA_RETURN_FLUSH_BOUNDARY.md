# Saved slot-A post-Up second helper return at `0x3BD:+09AA`

**Classification:** bounded caller continuation of the distinct `+09A7`
call to `+047B`, modeled only through the three instructions at
`+09AA..+09B3`. C stops at `+09E8` for AL=0 or `+09B3` otherwise. The
local loop, outer scan, `+05F3` flush wrapper, resident display transfer,
and UI page return are separate boundaries. There is no complete C
view, framebuffer parity, or game-function count claim.

Before implementation, `deja "Poolrad saved slot A post-Up U3BD +09AA
helper return zero branch +09E8 flush path"` found no matching session.
The [reviewed +0913 branch](SLOT_A_POST_UP_0913_ZERO_BRANCH_GATE.md)
supplied the flag calculation pattern, but +09AA has its own caller and
jump target. The [preceding +097F gate]
(SLOT_A_POST_UP_097F_RESET_CALL_GATE.md) supplied the measured pre-call
handoff; this return and the later flush were observed in the same native
exactly-one-Up run.

## Byte and caller bound

Ignored DOS v1.3 `START.EXE` SHA-256:
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Ignored `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Pinned ignored slot-A save SHA-256:
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

| Reached span | Original `GAME.OVR` range | Bound | SHA-256 |
| --- | --- | --- | --- |
| `+09A7..+09AA` caller call | `[0x2F29E,0x2F2A1)` | 3 bytes / 1 instruction | `d692760700364c4c9a275f51970b5fbb1b20063325b5ee2dee8b7781a253aae1` |
| `+09AA..+09B3` C gate | `[0x2F2A1,0x2F2AA)` | 9 bytes / 3 instructions | `63818af39243c81540f6bdecba8d75c46efb4527e2d16b86ed5c32f8ad070657` |
| `+0DAA..+0DB4` native flush-call exit | `[0x2F6A1,0x2F6AB)` | 10 bytes / 5 instructions | `7dec4d4d92bb5fffbafdc4de698caef9e181d36f059162d329066df4f06867a3` |

These original spans equal the loaded `2D14` bytes in the ignored RAM
snapshot. The +09AA instructions write AL to `SS:[BP-15]`, compare that
byte with zero, and branch to +09E8 when equal. The identical byte hash
for +0913 is a duplicate encoding, not evidence that the two reached
locations share a caller or successor.

## Same-run C/native return parity

Exactly one gameplay Up from the pinned visible `(15,4,N; 10:50)` frame
reached `+09A7` from the third +0951 loop exit. The external +047B call
returned to `2D14:09AA` with `AX=0`, `DI=004C`, `BP=3EE0`, `SP=3EC8`,
`CS=2D14`, `SS=1D16`, `FLAGS=7256`; full 640 KiB RAM SHA-256 was
`1267780894a2c8c2f4af6c06da116645a0619f9be50224f39a01bcc384f0eadc`.
The native zero branch stopped at `2D14:09E8` with unchanged full RAM,
AX=0, SP=3EC8, and FLAGS=7246. The C model starting from the same
+09AA RAM/register state reaches +09E8 with **655,360/655,360 RAM
bytes equal** and the same AX, SP, and FLAGS. The reached write stores
zero over an already zero byte; unchanged value is not a missing write.
Synthetic strict tests cover zero and nonzero AL, parity/sign flags,
a missing reached write, and an invalid entry.

## Next native UI boundary

The +09E8 local loop eventually reached +0A16 with loop byte 3; its
direct jump reached +0D7A. The first outer decision looped via +0DA7
to +069C. The later exit reached +0DAA, called +05F3, and the wrapper
entered the display transfer at `0C88:1B52`. At that far entry,
`DS:[4A1E]=1`, `SS:SP=1D16:3EBE`; the private stack held nested return
frames `2D14:05FB` and `2D14:0DAE`. This proves the flush call is live
on this post-Up route. It does not establish any C implementation of
the transfer or a post-flush page comparison. The older
[display-transfer audit](SLOT_A_U40C_DISPLAY_TRANSFER.md) describes
its separate first-view byte path.

## Reproduction

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
p = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
for lo, hi in ((0x09A7, 0x09AA), (0x09AA, 0x09B3), (0x0DAA, 0x0DB4)):
    span = p[0x2E8F7 + lo:0x2E8F7 + hi]
    print(hex(lo), len(span), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-09aa-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-09aa-p9 --target \
  poolrad_overlay_u3bd_post_up_09aa_return_branch_tests
ctest --test-dir resources/build-post-up-09aa-p9 \
  -R '^poolrad_overlay_u3bd_post_up_09aa_return_branch_tests$' \
  --output-on-failure
resources/build-post-up-09aa-p9/poolrad_overlay_u3bd_post_up_09aa_return_branch_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/exit-09aa-return-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/exit-09e8-zero-ram-private.bin
```

Original media, save, debugger trace, screenshots, and RAM/VRAM images
remain ignored under `resources/`.
