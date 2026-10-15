# Saved slot-A post-Up view flush into persistent EGA pages

The next observed page-value change after the second post-Up
`RES_0709:0626` printer return occurs across the reached
`RES_0464:1B52` display flush. A bounded C bridge carries the four
16,192-byte EGA planes through that flush. From the captured flush-entry
RAM, offscreen object and planes, its output matches the immediate native
return at **64,768/64,768 plane bytes** and **168/168 dirty metadata rows**.
This is one reached transfer boundary, not a complete C view call or a
joined one-Up replay. It adds no game-routine count.

Before debugging, `deja "Poolrad slot A post-Up second 0F2D:0626 printer
return next EGA page writer A000 A200 native stop"` found no indexed
sessions. I reused and rechecked the [post-Up caller return](SLOT_A_POST_EXIT_NATIVE_RETURN.md),
[view-four placement](SLOT_A_U3BD_VIEW4_PLACEMENT.md),
[all-dirty flush model](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md), and
[prior second-flush register measurement](SLOT_A_RES464_SECOND_FLUSH.md).
The preceding two-printer page join is local branch `eeac65a`; this
branch starts separately from `main` and does not edit that bridge.

## Original code and live path

Ignored DOS v1.3 `START.EXE`, `GAME.OVR`, and slot-A `SAVGAMA.DAT`
have SHA-256 hashes, respectively:

```text
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614
abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1
```

The unit `0x1C:+3492..+3496` original call is `GAME.OVR[0x41D8,0x41DD)`:
5 bytes, one aligned `lcall 0x3CA:0x25`, SHA-256
`e0055a6b9ab9d331162e677d95400c8459a2dd025aa4ffa8f4f2ba92f7968411`.
The loaded `3582:3492..3496` call is one `lcall 0xBEE:0x25`, SHA-256
`1f137b76909275734e41077c1dc7471b9effc1a81029c67294f96cdf8496b477`.
That stub enters the unit `0x3B4` view callback. The resident flush
original span is `START.EXE[0x61CB,0x64D5)`,
`RES_0464:1B52..1E5B`: 778 bytes / 305 aligned instructions,
SHA-256
`7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`.
The [original flush trace](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md)
identifies four offscreen-to-A200 staging stores and the latch-backed
A200-to-A000 Move.

The ignored same-run replay is
`resources/oracle-runs/slot-a-post-up-next-ui-p9-117/`. After the
hash-pinned slot-A load and exactly one Up, the caller reached
`3582:3492`, then the view call reached the resident flush
`0C88:1B52`, `DS=14A0`, `SS:SP=1D16:3EBE`. Its stack return words
are `2D14:05FB`; the enclosing view returns to `3582:3497`.
At view entry, flush entry and immediate return, party bytes are
`(14,4,6)`, the actual window-0 pointer is `3996:0000`, and its
`+18E/+190/+192` words are `(0,0,11)`.
The previous private run `p9-116` separately followed the second
printer return through its resident caller to this view call.

| Same-run stop | RAM SHA-256 | Four-plane page state |
| --- | --- | --- |
| `3582:3492` view entry | `04c001fa62397b12e6944bb737fec9beba9bc64c636d11a02de7bb92db61b5b3` | pre-flush |
| `2D14:02FB` first tile entry | `ce46d9cb61cbd8bb58187e113bcd20369ee55d5eea99544102c4edeb612e5d35` | same pre-flush values |
| `0C88:1B52` flush entry | `0a52ffd104d9277534e57940de97560b50ceeafe46a37583894fb5184b215061` | same pre-flush values |
| `2D14:05FB` immediate flush return | `9d3ff617ab35da40cd5b03f0e38c53b4aaa051feaa56e392370ccb2279c15038` | 1,394 values changed |
| `3582:3497` enclosing view return | plane capture only | same post-flush values |

The audit pins the full hashes of all 20 private plane captures. Each
plane contains 8,000 A000 bytes, a 192-byte gap, and 8,000 A200
bytes. All four planes are equal between view entry, first tile entry,
and flush entry; they are equal again between immediate flush return
and enclosing view return. This is a comparison of sampled values at
stops: intervening same-value or transient writes are not excluded.
Across the flush, 697 A200 plane-byte values and 697 A000 values
change. The pre/post full-plane SHA-256 hashes are:

| Plane | Before | After |
| --- | --- | --- |
| 0 | `625deb6ace7bb0c9386515828ec4f5cabbfceb88ad25940278f8ac0c6c7efb7e` | `1cc993dcf9fa7f47ed1bc27bcb32b1376382f21aba89033f6cbf187172bac70f` |
| 1 | `9c386eb2e055b9b1030e59ed45dc8042a37fe15db3977e3ff84a421222b0e05d` | `38f7ba9fc9c900545a5c59157826bf33a7ff1278484084a5d82d776874593a19` |
| 2 | `41e9fd59f57279235fa656d70e114d23d2bba4309e23b6c0093216e9d65424df` | `350012034c279d2bbf0131aa9843a1d9faf433e5b95b8b3277e56c0ab0847b99` |
| 3 | `558cb49ebe7333b97b7bdde0e9a3cb6355777e7968f1aa3dc1263bce488f9435` | `7101f831f127d638507ce3f0367683ef306b57883f746baa1735533f1252bf0a` |

## Reached C boundary

At view entry no dirty row is set. At flush entry, exactly rows
`16..103` are dirty. Each has 11 byte positions per plane, with
`first(r)=1352+84(r-16)`, `last(r)=1395+84(r-16)`, `low(r)=2`,
and matching A200/A000 start offset `0x03C3+40(r-16)`. The actual
far pointer `DS:[4A20]` resolves to `3DE4:0008`. In the bounded
8,727-byte source extent needed by these rows, 697 values differ
between view and flush entry. Constructing that object and the dirty
extents is the next unsupported C effect upstream. The bridge returns
`PR_SLOT_A_POST_UP_FLUSH_BLOCKED_DIRTY_FRAME` or
`PR_SLOT_A_POST_UP_FLUSH_BLOCKED_OFFSCREEN` if either input is absent;
it never inserts the native output as a prediction.

The bridge maps the existing bounded `pr_r464_dirty_transfer` onto
caller-owned full planes, preserving both A000 and A200, their gap,
and all bytes outside reached dirty spans. The probe uses the captured
flush-entry RAM, offscreen object, and pre-flush four planes. It
predicts **64,768/64,768** bytes at the native immediate return and
**168/168** row reset records; the entire private C output SHA-256 is
`86b0c234c0958a9b841529e5df8a61630c2d0dc40eca4ce7a5fc9f1c7a17fd22`.
The synthetic test checks blocked missing dirty-frame and object
inputs, a short source boundary without effects, and one successful
four-plane copy into a persistent page. The full strict C11 build and
CTest suite passed
**246/246** on this branch.

The C probe supplies Graphics Controller registers 0/1/3/5/8 as
`00/00/00/00/FF`. A live read at this run's enclosing view return
gave those values, and the earlier independently captured same
`0C88:1B52` return-frame path measured them at flush entry. This run
did not preserve a separate register readback at its flush entry;
the C parity claim is conditioned on the stated register input.
Page-value parity does not prove whole-screen color or one-Up C parity.

## Reproduce

```sh
cmake -S . -B resources/build-p9-next-ui \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-p9-next-ui -j4
ctest --test-dir resources/build-p9-next-ui \
  -R '^poolrad_slot_a_post_up_view_flush_page_tests$' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/replay/slot_a_post_up_view_flush_page_audit.py \
  --start resources/DOS/POOLRAD/START.EXE \
  --game resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/slot-a-post-up-next-ui-p9-117 \
  --probe resources/build-p9-next-ui/poolrad_slot_a_post_up_view_flush_page_probe
```

The ignored replay was captured by starting the private heavy DOSBox-X
with `DEBUGBOX START`, loading slot A, selecting Search, arming the
view caller at `3582:3492`, and sending one Up. At the view caller,
first tile entry, flush entry, immediate flush return, and enclosing
view return, all four EGA planes were dumped with graphics-controller
read-map selection `0..3` and `MEMDUMPBIN A000 0000 3F40`; read-map
zero and register selector 5 were restored before each `RUN`. RAM
snapshots used `MEMDUMPBIN 0000 0000 A0000`. Original media, save,
debugger output, and full captures remain ignored under `resources/`.

The next two disjoint UI gates are to compose the view's offscreen
object and 88 dirty extents from the second-printer page state, then
to stop at the next page-value writer after `3582:3497`. The intercall
party/window writer watch is owned separately by pC; this page model
does not infer that state.
