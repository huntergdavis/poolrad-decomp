# Saved slot-A post-Up loop successor at `0x3BD:+0951`

**Classification:** bounded, call-free loop iteration from `+0951` through
the unsigned decision at `+0901`. The C entry stops at `+0903` or `+097F`.
It does not execute the `+0903` helper, the `+0913` result branch, the
`+097F` exit body, any art transfer, or the later UI printer. This is no
complete C view or page claim.

Before this slice, a bounded `deja "Poolrad slot A post-Up U3BD +0951
second loop +0903 +097F native RAM"` search timed out with no reusable
result. The [reviewed +0938 control model]
(SLOT_A_POST_UP_0938_CALLER_CONTROL_GATE.md) provided the already verified
instruction semantics; its first assignment of IP to `+0951` is idempotent
when this new entry is called at `+0951`. The [reviewed +0913 branch]
(SLOT_A_POST_UP_0913_ZERO_BRANCH_GATE.md) provided the immediate caller
predecessor. The second iteration and both destinations were checked in a
fresh exactly-one-Up heavy-debug replay.

## Original and loaded spans

Ignored DOS v1.3 `START.EXE` SHA-256:
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Ignored `GAME.OVR` SHA-256:
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Pinned ignored slot-A save SHA-256:
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

| Reached unit span | Original `GAME.OVR` range | Bound | SHA-256 |
| --- | --- | --- | --- |
| `+0951..+097F` | `[0x2F248,0x2F276)` | 46 bytes / 17 instructions | `7f318f344e9e134a540795af7731bfec7af0751a5054c4a743c9d00fab19d96b` |
| `+08FD..+0903` | `[0x2F1F4,0x2F1FA)` | 6 bytes / 2 instructions | `c78c6abc35d1e8414221738883a556620ae23838b28b88fb24cfacbd3a6613cc` |

The original spans equal the bytes loaded at `2D14` in the private
same-run RAM snapshot. The first span increments `SS:[BP-10]`, subtracts
2 from `SS:[BP-12]`, loads the indexed `DS:2736` and `DS:273F` signed
direction bytes, and updates the local x/y words. It jumps to `+08FD`,
where an unsigned comparison with 3 selects `+0903` or `+097F`.

## Native second iteration and C parity

One gameplay Up from the pinned visible `(15,4,N; 10:50)` frame reached
`2D14:0938`, then the first `+047B` helper return, then `+0913` with
AX=0, and then `+0951`. At this second-loop entry, `CS=2D14`,
`DS=14A0`, `SS=1D16`, `BP=3EE0`, `SP=3EC8`, `AX=0`, `DI=005C`,
`FLAGS=7246`; full 640 KiB RAM SHA-256 was
`7a019f4b5fe921336f5243b5304be82b5839483317a63262d7ee03e663207d31`.
The local loop byte was 1, step word 65534, x/y words 12/5, and index 4.
The indexed direction bytes were 0/1.

Native next stopped at `2D14:0903`, so `JAE +097F` was not taken.
It had AX=6, DI=4, BP=3EE0, SP=3EC8, FLAGS=7297 and full RAM SHA-256
`f618a46cef8046eab37535ec2c471d05a911896454e2cb27dd8597bdd73829a7`.
Exactly three byte values changed: physical `0x2102E` 254→252,
`0x21030` 1→2, and `0x21031` 5→6. The C entry starts with the captured
RAM/register state and reaches the same IP and register tuple with
**655,360/655,360 RAM bytes equal** to the native +0903 snapshot.

The next `+047B` return again had AX=0 at `+0913`. That separate branch
led to a third `+0951` iteration and then native `+097F` with loop byte
3, AX=7, DI=4, FLAGS=7246. The third-loop route is an observed alternate
destination, not part of the second-loop private parity pair. Synthetic
tests cover both unsigned branch outcomes and a missing reached direction
table byte, preserving prior local writes at the boundary.

## Reproduction

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
p = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
for lo, hi in ((0x0951, 0x097F), (0x08FD, 0x0903)):
    span = p[0x2E8F7 + lo:0x2E8F7 + hi]
    print(hex(lo), len(span), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-0951-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-0951-p9 --target \
  poolrad_overlay_u3bd_post_up_0951_successor_tests
ctest --test-dir resources/build-post-up-0951-p9 \
  -R '^poolrad_overlay_u3bd_post_up_0951_successor_tests$' \
  --output-on-failure
resources/build-post-up-0951-p9/poolrad_overlay_u3bd_post_up_0951_successor_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/successor-0951-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/successor-0903-post-ram-private.bin
```

Original media, save, debugger traces, screenshots, and RAM snapshots remain
ignored under `resources/`.
