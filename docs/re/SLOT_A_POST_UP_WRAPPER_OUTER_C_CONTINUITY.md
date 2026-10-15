# Saved slot-A post-Up wrapper-to-outer C continuity

**Classification:** focused C composition harness, with no new guest
instruction translation. The first `+05FB` wrapper return has measured
native/C parity through `+0DAE`; the second `+0DAE` outer return is a
stack-derived C prediction until a live native successor stop. This is
not a complete view, UI frame, framebuffer, or game-function count.

Before this harness, bounded `deja` queries for the two component gates
found no indexed matches. A combined `deja` query was started but stopped
when the local index stalled during the host-load hold. The reused evidence
is the [approved wrapper return](SLOT_A_POST_UP_05FB_WRAPPER_RETURN.md)
and [local outer-return preparation](SLOT_A_POST_UP_0DAE_OUTER_RETURN_PREP.md).

## Bounds and continuity

Ignored DOS v1.3 `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The wrapper `+05FB..+05FD` is 2 bytes / 2 instructions at
`GAME.OVR[0x2EEF2,0x2EEF4)`, SHA-256
`05303d46935f1d6bc7f492470521a209a51050b261d452a788f042866b5d72cd`.
The outer return `+0DAE..+0DB4` is 6 bytes / 3 instructions at
`GAME.OVR[0x2F6A5,0x2F6AB)`, SHA-256
`36916f0bed44fe934eeabdd0115011a4de8eab35e6758714d66c7343f824ed3d`.
Both loaded spans equal the original bytes in the measured post-Up RAM.

The same-run native `2D14:05FB` input after exactly one Up had
`SS=1D16`, `BP=SP=3EC2`, `AX=0100`, `DI=4EC0`, `FLAGS=7246`.
Its 655,360-byte conventional RAM SHA-256 was
`b8d808d1469791646d57003438b094f0d9fee0459442021c3747a5635025bc8c`.
The harness passes that **same RAM object** through the two existing C
models. At the join it asserts the measured native `2D14:0DAE` state:
`BP=3EE0`, `SP=3EC8`. The second model then predicts
`2E35:022E`, `BP=3EF0`, `SP=3EEC`, with AX, DI, flags, and all guest RAM
unchanged. A separate native stop at `2E35:022E` is still required.

Synthetic tests vary the wrapper and outer far-return words and assert
that the two models share one RAM pointer, preserve the register handoff,
discard the outer call's three argument words, and write no RAM.

## Reproduction

```sh
cmake -S . -B resources/build-post-up-compose-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-compose-p9 --target \
  poolrad_overlay_u3bd_post_up_wrapper_outer_compose_tests
ctest --test-dir resources/build-post-up-compose-p9 \
  -R '^poolrad_overlay_u3bd_post_up_wrapper_outer_compose_tests$' \
  --output-on-failure
resources/build-post-up-compose-p9/poolrad_overlay_u3bd_post_up_wrapper_outer_compose_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/post-up-05fb-ram-private.bin
```

Original media, saved game, debugger output, screenshots, and RAM images
remain ignored under `resources/`. No emulator was run for this harness.
