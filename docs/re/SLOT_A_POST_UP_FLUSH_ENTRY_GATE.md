# Saved slot-A post-Up flush call-frame entry

**Classification:** bounded C call-frame model from `0x3BD:+0DAA`
through local wrapper `+05F3` to the loaded far target
`0C88:1B52`. It stops **before** the resident display transfer executes.
The transfer's EGA state, framebuffer writes, return to `+05FB`,
and the view's `RETF` are unresolved C effects here. No complete view,
framebuffer parity, or game-function count is claimed.

Before implementation, `deja "Poolrad post-Up U3BD +0DAA +05F3 flush
wrapper 0C88:1B52 caller stack parity"` found no matching session.
The older [display-transfer audit](SLOT_A_U40C_DISPLAY_TRANSFER.md)
identified `+05F3` as a possible wrapper; this post-Up run independently
reached it from [the +09AA branch and outer scan]
(SLOT_A_POST_UP_09AA_RETURN_FLUSH_BOUNDARY.md).

## Original code and relocated target

Ignored DOS v1.3 `START.EXE` SHA-256:
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Ignored `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Pinned ignored slot-A save SHA-256:
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

| Reached unit span | Original `GAME.OVR` range | Bound | SHA-256 |
| --- | --- | --- | --- |
| `+0DAA..+0DAE` | `[0x2F6A1,0x2F6A5)` | 4 bytes / 2 instructions | `704d6c4374c1e954a1345cf47f46e0b6956388b94a4d4d04ee9b059dc3b27141` |
| `+05F3..+05F6` | `[0x2EEEA,0x2EEED)` | 3 bytes / 2 instructions | `f1416cf1866c19d9e7428d2851a85e7dff657edb40a833dc86d3c0988e96d987` |
| `+05F6..+05FB` | `[0x2EEED,0x2EEF2)` | 5 bytes / 1 instruction | `4bd88079aa4a757b881a8785a9b2760e88b3d10111e5792ae27e294b34141913` |

The first two spans equal loaded `2D14` bytes. The five-byte far-call
span has loaded SHA-256
`2aea9cd786e94d66a3e77301eb19ffae940a75e8e8269f5683473fbb133f7d2f`:
only instruction-relative offsets 3 and 4 differ. Its target offset
remains `0x1B52`, while the original unit segment `0x0464` relocates to
the live segment `0x0C88`. The C model reads both target words from the
loaded guest CS bytes at the reached `+05F6` instruction; it does not
hardcode a runtime load segment.

The ordered path is `+0DAA PUSH CS`, `+0DAB CALL +05F3`, `+05F3 PUSH BP`,
`+05F4 MOV BP,SP`, and `+05F6 CALL FAR` through the loaded pointer.
The near call pushes return IP `+0DAE`; the far call pushes return
`2D14:05FB`. The C entry checks each reached stack write and relocated
operand read. On `BOUNDARY`, it leaves prior call-frame effects and the
failing IP visible.

## Native and C entry parity

Exactly one gameplay Up from the pinned visible `(15,4,N; 10:50)` frame
reached the second helper return at +09AA, then the local/outer loops,
and finally `2D14:0DAA`. Native there had `SS=1D16`, `BP=3EE0`,
`SP=3EC8`, `AX=4`, `FLAGS=7246`; full conventional RAM SHA-256 was
`da08f953fa90451da5131907f66bd395eb5f10e1e803fd03851d814ec60bd34d`.

Native then stopped at far target `0C88:1B52`, before its first
instruction, with `BP=3EC2`, `SP=3EBE`, `AX=4`, `FLAGS=7246` and RAM
SHA-256
`15d7868965bd21e930ca512d50ff67cde5cd56b11a73faa155987c9b4da6aadd`.
The private stack words from `SS:3EBE` upward were `05FB`, `2D14`,
`3EE0`, `0DAE`, `2D14`, proving both nested returns and the saved BP.
The display-mode byte `DS:[4A1E]` was 1. Starting C from the same
+0DAA RAM and registers, the model reaches `0C88:1B52` with
**655,360/655,360 RAM bytes equal** and matching CS, IP, BP, SP, AX,
and FLAGS. Synthetic strict tests vary the loaded target segment,
verify stack order, and test missing first stack and far-operand access.
The resident transfer has not executed in either comparison endpoint.

## Reproduction

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
p = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
r = Path('resources/oracle-runs/slot-a-post-up-exit-p9-119/flush-0daa-pre-ram-private.bin').read_bytes()
for lo, hi in ((0x0DAA, 0x0DAE), (0x05F3, 0x05F6), (0x05F6, 0x05FB)):
    original = p[0x2E8F7 + lo:0x2E8F7 + hi]
    loaded = r[0x2D14 * 16 + lo:0x2D14 * 16 + hi]
    print(hex(lo), len(original), sha256(original).hexdigest(),
          sha256(loaded).hexdigest(),
          [i for i, (x, y) in enumerate(zip(original, loaded)) if x != y])
PY
cmake -S . -B resources/build-post-up-flush-entry-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-flush-entry-p9 --target \
  poolrad_overlay_u3bd_post_up_flush_entry_tests
ctest --test-dir resources/build-post-up-flush-entry-p9 \
  -R '^poolrad_overlay_u3bd_post_up_flush_entry_tests$' \
  --output-on-failure
resources/build-post-up-flush-entry-p9/poolrad_overlay_u3bd_post_up_flush_entry_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/flush-0daa-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/flush-res-1b52-entry-ram-private.bin
```

Original media, save, debugger trace, screenshots, and RAM/VRAM images
remain ignored under `resources/`.
