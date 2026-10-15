# Saved slot-A post-Up reset and second helper handoff at `0x3BD:+097F`

**Classification:** bounded, call-free C prefix `+097F..+09A7` ending
immediately before another `+047B` helper call. Same-run C/native parity
is established only at this pre-call boundary. The helper, its `+09AA`
return branch, the outer wall scan, and the eventual display flush are
native trace observations below, not effects of this C model. No complete
C view, framebuffer parity, or game-function count is claimed.

Before debugging this follow-on, bounded `deja "Poolrad slot A post-Up
U3BD 2D14:097F loop exit +09A7 +047B next UI return flush boundary"`
timed out with no reusable result. The [reviewed +0938 control model]
(SLOT_A_POST_UP_0938_CALLER_CONTROL_GATE.md) supplied the +097F branch
lead. The older [display-transfer note]
(SLOT_A_U40C_DISPLAY_TRANSFER.md) supplied a candidate +05F3 flush
path, independently checked in this exactly-one-Up replay.

## Original and loaded bound

Ignored DOS v1.3 `START.EXE` SHA-256:
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Ignored `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Pinned ignored slot-A save SHA-256:
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

| Span | Original `GAME.OVR` range | Bound | SHA-256 |
| --- | --- | --- | --- |
| Modeled `+097F..+09A7` | `[0x2F276,0x2F29E)` | 40 bytes / 16 instructions | `b0b3ba88e32bf677f4123e4757c6f2f97422e5e2069b802e98dcc8f6a7f16608` |
| External `+09A7..+09AA` call | `[0x2F29E,0x2F2A1)` | 3 bytes / 1 instruction | `d692760700364c4c9a275f51970b5fbb1b20063325b5ee2dee8b7781a253aae1` |
| External `+09AA..+09B3` return branch | `[0x2F2A1,0x2F2AA)` | 9 bytes / 3 instructions | `63818af39243c81540f6bdecba8d75c46efb4527e2d16b86ed5c32f8ad070657` |
| External `+0DAA..+0DB4` exit | `[0x2F6A1,0x2F6AB)` | 10 bytes / 5 instructions | `7dec4d4d92bb5fffbafdc4de698caef9e181d36f059162d329066df4f06867a3` |

Each listed span equals the loaded `2D14` bytes in the private +097F
RAM snapshot. The +09AA branch has the same nine byte values as the
separate +0913 branch, but its location, predecessor, and jump target
are different; this gate does not conflate them.

At `+097F`, the original code copies `SS:[BP-5]` and `SS:[BP-7]` into
the local x/y words, writes zero to the loop byte and step word, compares
the new loop byte with 3, and falls through to push the low-byte x/y
values, query byte, and CS. The `+09A7 CALL +047B` has not executed at
the modeled endpoint. The C entry preserves 16-bit offset and 20-bit
physical wrap, x86 flags, and the order of reached reads and writes.
Missing reached RAM returns `BOUNDARY` with prior effects visible.

## Same-run pre-call parity

Exactly one gameplay Up from the pinned visible `(15,4,N; 10:50)` frame
reached `+0938`, the +0913 zero branch, a second +0951 iteration, then
the third +0951 iteration's unsigned exit at `2D14:097F`. At +097F,
`CS=2D14`, `SS=1D16`, `BP=3EE0`, `SP=3EC8`, `AX=7`, `DI=4`,
`FLAGS=7246`; full 640 KiB RAM SHA-256 was
`3ecde0f8283781a66d9f8b3437e008d9a2fd2d1dc0c527636b17ba7d61ac8559`.
The loop byte was 3, step word 65530, temporary x/y words 12/7, base
x/y words 12/4, and query byte 0.

Native next stopped at `2D14:09A7` with `AX=0`, `SP=3EC0`,
`FLAGS=7293`; full RAM SHA-256 was
`39feb3c2df40c2a0869c7ed2b7312b2087a2cf0236c7460a593837dff9b95944`.
The local x/y became 12/4, loop and step became 0, and the pushed words
at `SS:3EC6..3EC0` were 12, 4, 0, `2D14` in push order. Six RAM byte
**values** differed between the two snapshots; writes of already equal
values still occurred. The C test starts from the same native +097F
snapshot and reaches +09A7 with **655,360/655,360 RAM bytes equal**,
plus matching AX, SP, FLAGS, and IP. Synthetic strict tests cover the
successful frame, missing first read, missing first push after prior
local writes, and invalid entry.

## Native-only return and flush boundary

After the external +047B call, native returned at `2D14:09AA` with
`AX=0`, `DI=004C`, `SP=3EC8`, `FLAGS=7256` and RAM SHA-256
`1267780894a2c8c2f4af6c06da116645a0619f9be50224f39a01bcc384f0eadc`.
The following zero branch reached `+09E8` with unchanged RAM and
`FLAGS=7246`. The local loop ultimately reached `+0A16` with loop byte
3 and helper-result byte 14, then jumped to `+0D7A`. The first outer
decision reached `+0DA7` and looped to `+069C`; a later outer exit
reached `2D14:0DAA` with outer counter byte 255. Its RAM SHA-256 was
`da08f953fa90451da5131907f66bd395eb5f10e1e803fd03851d814ec60bd34d`.

The reached `+0DAB CALL +05F3` entered the local flush wrapper. Its
`+05F6` far call reached `0C88:1B52` with `DS:[4A1E]=1`,
`CS:IP=0C88:1B52`, `SS:SP=1D16:3EBE`, and RAM SHA-256
`15d7868965bd21e930ca512d50ff67cde5cd56b11a73faa155987c9b4da6aadd`.
At this stop, the private stack has return frames `2D14:05FB` and
`2D14:0DAE`. The resident transfer body and framebuffer effects are
outside this gate. The +09AA return branch and the eventual post-flush
return remain separate C parity work.

## Reproduction

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
p = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
for lo, hi in ((0x097F, 0x09A7), (0x09A7, 0x09AA),
               (0x09AA, 0x09B3), (0x0DAA, 0x0DB4)):
    span = p[0x2E8F7 + lo:0x2E8F7 + hi]
    print(hex(lo), len(span), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-097f-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-097f-p9 --target \
  poolrad_overlay_u3bd_post_up_097f_reset_call_tests
ctest --test-dir resources/build-post-up-097f-p9 \
  -R '^poolrad_overlay_u3bd_post_up_097f_reset_call_tests$' \
  --output-on-failure
resources/build-post-up-097f-p9/poolrad_overlay_u3bd_post_up_097f_reset_call_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/successor-097f-exit-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/exit-09a7-precall-ram-private.bin
```

Original media, save, debugger traces, screenshots, and RAM/VRAM dumps
remain ignored under `resources/`.
