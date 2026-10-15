# Saved slot-A post-Up third `02FB` tile call

**Classification:** third reached tile-plane call after the approved second
`0300..030E` loop gate. The existing C tile-plane model matches the complete
14,135-byte offscreen object and 1,176-byte dirty-metadata span at this call's
direct return. This is a bounded object-value result, not whole-CPU, complete
view, display, or full-C parity.

Before debugging, `deja "Poolrad slot A post-Up second 2D14 0300 030E caller
next page writer A000 offscreen page flush U3BD one Up"` was run with a
25-second bound and returned no reusable result. The checked-in
[two-call evidence](SLOT_A_POST_UP_02FB_TWO_TILE_GATE.md) and
[second-return gate](SLOT_A_POST_UP_0300_TILE_LOOP_GATE.md) supplied the
immediately preceding call order. The earlier
[view-four writer audit](SLOT_A_POST_VIEW4_FIRST_OFFSCREEN_WRITER.md)
identified the resident masked-store path but concerns a different frame.

## Pinned source and reached path

Ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

| Original span | File range | Bound | SHA-256 |
| --- | --- | --- | --- |
| unit `0x3BD:+02FB..+0300`, far call | `GAME.OVR [0x2EBF2,0x2EBF7)` | 5 bytes / 1 aligned instruction | `64940f6941110590c5125977fe8e435e68b57a1fc5b998a494f76b176f0050ec` |
| unit `0x40C` entry 2 | `GAME.OVR [0x32392,0x324E2)` | 336 bytes / 127 aligned instructions | `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00` |
| resident compositor `+08D4..+0D71` | `START.EXE [0x4F4D,0x53EA)` | 1,181 bytes / 429 aligned instructions | `9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe` |
| reached masked store `+0C6A..+0C6E` | `START.EXE [0x52E3,0x52E7)` | 4 bytes / 1 aligned instruction | `f1798d82c73cbb1d10f18e8d3d5f89265a778c5e2ea9cdfaca48e0dd4d68ec90` |

One fresh hash-pinned DOSBox-X replay loaded visible `(15,4,N; 10:50)` and
sent exactly one Up. The sole `2D14:02FB` breakpoint hit three times. The
third pre-call stop had `DS=14A0`, `SS:SP=1D16:3EA2`, stack words
`(1,1,39,7,6)` at `SP+0..+8`, and `FLAGS=0x7246`. They select one-based
item 39 at `(x,y)=(6,7)`; the live group-zero remap word `DS:[2722]=1`
produces item index 38. The call returned directly to `2D14:0300`.

At `DS:[4A20]`, the live destination far pointer was `3DE4:0008`
(physical `0x3DE48`), yielding a 14,135-byte offscreen object. Its pre-call
SHA-256 was
`bf038de10ab4a98444bde1b3234bd9b14ee6b24dff311c2280e95a3eb421bf04`,
the same value as the approved second call's return object. The third return
object SHA-256 was
`bfa58e2aaa9d5089a49e0b367655746d5720fd9372b500e866ab8fe6a82cee50`.
Fourteen object byte **values** changed. The 1,176-byte dirty-metadata span
`DS:4A28..4EBF` had no value changes. The full 640 KiB pre and return RAM
SHA-256 values were respectively
`a2b2bbfacc914e3880191c24cd6e237c61b42df470dc618dab7674a8d778e0bd`
and
`3fa852b63050d01086fc94ac17787888a7b3177e82955a82efd0fab7aeab7728`;
24 RAM byte values differed. Equal object hashes at two stops do not rule
out intervening writes that restored the same values.

## Original-byte write watch

In another fresh one-Up replay, the third `02FB` pre-call stop was used to
arm a native memory-change watch at `3DE4:1299` (object offset `0x1291`,
row 56, column 6, plane 2). It observed `0→254` and stopped at
`0C88:0C6E`, just after original resident `0464:+0C6A` executed
`MOV byte ES:[DI+0x17],DL`. At the stop, `ES=3DE4`, `DI=0x1282`,
`DX=0x00FE`, `SS:SP=1D16:3E66`, and `FLAGS=0x7216`; the nested return
at `SS:3E82/3E84` was `223B:024E` and the outer return at
`SS:3E9E/3EA0` was `2D14:0300`. The watched RAM SHA-256 was
`48ba7c6aaa41cfa6df3bfcea344f5e9bd1ec24bec567506f1c033142022c44fc`.
Its object differed from the third pre-call object at exactly one byte.
This pins a reached changed-value store, not the first write anywhere.

## C/native boundary and reproduction

The new synthetic test uses item 39, remap 1, `(6,7)`, a synthetic source
and mask, and a whole-object expected-value check. It exercises 32 modeled
plane-byte writes, 14 changed destination values, item index 38, first data
offset 4728, and first changed data offset 4730. The optional private mode
derives the three live far pointers and call arguments from the ignored
pre-call RAM, invokes existing `pr_u40c_entry2_mode1_tile_plane` once, and
compares **14,135/14,135 object bytes** and **1,176/1,176 metadata bytes**
with the matching direct-return RAM. It does not set expected destination
values or model the following loop/flush.

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
root = Path('resources/DOS/POOLRAD')
for name, lo, hi in (('GAME.OVR', 0x2EBF2, 0x2EBF7),
                     ('GAME.OVR', 0x32392, 0x324E2),
                     ('START.EXE', 0x4F4D, 0x53EA),
                     ('START.EXE', 0x52E3, 0x52E7)):
    span = (root / name).read_bytes()[lo:hi]
    print(name, hex(lo), hex(hi), len(span), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-third-page-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-third-page-p9 -j 8
ctest --test-dir resources/build-post-up-third-page-p9 --output-on-failure
resources/build-post-up-third-page-p9/poolrad_slot_a_post_up_02fb_third_page_writer_tests \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/page-next-call3-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/page-next-call3-return-ram-private.bin
```

Original media, save, RAM snapshots, emulator trace, and screenshots remain
ignored under `resources/`. The page result ends at the third direct
`2D14:0300` return.
