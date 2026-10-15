# Saved slot A: row-15 clear between two post-Up printer calls

After exactly one Up from the pinned saved-slot-A prompt, the native
replay reaches two `RES_0709:0626` printer calls at loaded
`0F2D:0626`, both returning to `08DE:2B5A`. Between them, the first
printer's 210 changed EGA page values are reset by a reached
`RES_0709:00B7` rectangle fill. The first reached value-changing store
for the selected row-15 byte is loaded SYSTEM `131C:16B7` (`rep
stosb`), with `ES:DI=A000:12D1`, `AL=0`, and `CX=0x16` (22 bytes).

The existing bounded C models, composed as printer → rectangle clear
→ printer, match each of the three same-run native page boundaries
over **32,000/32,000 bytes**. This is page parity for this measured
sequence. The C helpers do not model the intervening key route,
movement, clock, CPU state, or all RAM side effects.

Before this new trace, `deja "Poolrad slot A one Up next native UI
writer after 08DE:2B5A printer return 0C88:1B52 2116:01DA two call
page parity"` timed out without a match. The merged
[full-page first-view printer evidence](SLOT_A_AFTER_ROW9_FULL_PAGE_SEQUENCE.md)
and [resident rectangle-fill recovery](RES_0709_RECT_FILL_00B7.md)
provided the two existing C models and original-byte leads; the
post-Up caller order and frame were checked from these replays.

## Pinned private inputs and stops

The printer sequence is in ignored
`resources/oracle-runs/slot-a-post-up-next-ui-p9-112/`; the targeted
writer replay is in ignored
`resources/oracle-runs/slot-a-post-up-reset-watch-p9-113/`. Both use
DOS v1.3 `START.EXE` SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
`GAME.OVR`
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and slot-A save
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private patched DOSBox-X binary hashes to
`c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457`.
All RAM dumps are 640 KiB; each EGA page is four 8,000-byte planes.

| Native stop | Full RAM SHA-256 | Full EGA page SHA-256 |
| --- | --- | --- |
| First printer entry, `0F2D:0626` | `7b3ef058c858962c92e42bcf4dbbae341dab58b93d1821f71532777ef07bdad8` | `b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44` |
| First return, `08DE:2B5A` | `8c11527ea071fa4aea4972f5d9b8d2dae915058d7f0e75c627e41485d2d17a70` | `a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac` |
| Rectangle entry, `0F2D:00B7` | `cc4e20fd5a91cb158aa09239924ab1d3cfd83cc53e3209f4a29917f0c3a89475` | `a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac` |
| First zero-store stop, `131C:16B7` | `05e5cad6daff48a72a57c507a464258b483f3b1209eb6d95d00b9bd7822d36b0` | Not dumped |
| Rectangle return, `0C15:01F6` | `438cf267bba6666971cac6dba404e8d1cbbaee18e1496702423d623c20427714` | `b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44` |
| Second printer entry, `0F2D:0626` | `f6015203f697b7c8007029d4b629a6c3154a9fae85938b958896b5af0c093719` | `b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44` |
| Second return, `08DE:2B5A` | `1c805a0f0c35e2c6f6b4d7c70318f296a95cc2f1289f05c1123df23a78972d7f` | `1019a688b324afa165f731013c6491c0cb54232262403f7c70cbb7fb97543773` |

The two printer calls are from the **same** one-Up replay. Their
Pascal sources each have length 19. First source length-plus-content
SHA-256 is
`e767d5c8ef552436c5e411af72d8eca9581f3feacbb988c67c16bf2b701bc2fe`;
second is
`f13e024bc34f688552e96d6cfb8be870b68d2ada58f4ae6fdea7fd7e583e9860`.
Both frames use color 10, row 15, column 17, but their private source
far pointers differ: `1D16:3E7D` and `1D16:3D8D`. The loaded printer
body is the same hash-locked 129-byte `0F2D:0626..06A7` body, and
the live font buffer at `DS:5E20` hashes to
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.

## Caller and first writer proof

The original `RES_00BA:2B42` five-byte far call at expanded image
`0x36E2` hashes to
`395783ec4ad75a0552ed076373c516213f522e5aacc77c12d8fbf9ead76dd23f`
and targets `03F1:0137`. Its loaded `08DE:2B42` call hashes to
`2f89ade8cd970b19f81e674324278b7e2f7845bd119dbf92374051d645fc1850`
and targets `0C15:0137`. The original `RES_03F1:01F1` far call at
expanded image `0x4101` hashes to
`2286767d9eccf2e3fe83669ef5367f5ef97da8c2145e12d54b0291510e19093c`
and targets `0709:00B7`; its loaded `0C15:01F1` call hashes to
`8d1b1dec8fbef9dea92ee3e851c55572850da6b639cd9fb3890c998e918cc8e2`
and targets `0F2D:00B7`. The original 186-byte rectangle body at
expanded `[0x7147,0x7201)` hashes to
`f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5`;
the loaded `0F2D:00B7..0171` body hashes to
`02ba954ed5bc0baa194fa7e74eeaceba27b09386107ac51df7df27a09c6aaa5f`.

At `0F2D:00B7`, `SS:SP=1D16:3B62` contains far return
`0C15:01F6` and six low-byte arguments in order: color 0, page 0,
bottom 15, right 38, top 15, left 17. The parent far return is
`08DE:2B47`, and `DS:4A1A=0` selects active page zero. At the first
SYSTEM store stop, `SS:SP=1D16:3B4A` returns to `0F2D:0145`; the
stack supplies a 22-byte count, offset `0x12D1`, and segment `A000`.
Debugger registers give `CS:IP=131C:16B7`, `ES:DI=A000:12D1`,
`AL=0`, `CX=0x16`; the two-byte loaded instruction span hashes to
`50b2d3be54fb620b49268d3b4bd0c759531794c4c735fc2916071807cf9ee7a6`.
The first store address is global scanline 120, column 17. The
rectangle covers scanlines 120–127, columns 17–38, on four planes.

An earlier `BPM A000:12D1` stop at `0F2D:003F` reported 12→0, but a
direct read-map-1 sample at that stop still read 12. That event was
caused by EGA read-map selection and is excluded as a guest write.
The code-entry stop at `131C:16B7`, its return chain, and the full
pre/post page captures supply the writer proof.

## Bounded C page bridge and native state limit

The first printer changes 210 values: 105 in plane 1, 105 in plane 3,
within row-15 columns 17–35. The clear changes those 210 values back
to zero. The second printer changes 210 values in the same region to
its distinct private source glyphs. No net values change in planes 0
or 2. The C reports are:

| Stage | Modeled stores | Net changed values | C output versus native page |
| --- | ---: | ---: | ---: |
| First printer | 608 | 210 | 32,000/32,000 |
| Row-15 rectangle clear | 704 | 210 | 32,000/32,000 |
| Second printer | 608 | 210 | 32,000/32,000 |

The rectangle C model also matches its *own* immediate native return
in the separate writer replay, 32,000/32,000 bytes. Native page
hashes bridge the two runs: first printer return equals rectangle
entry, and rectangle return equals second printer entry. The same-run
printer sequence uses those exact two boundary pages. Snapshot
equality does not count same-value or transient native stores.

At the first printer entry and return, the native `DS:6AAD..6AAF`
party tuple is `(15,4,6)`, while at the second entry and return it is
`(14,4,6)`. The actual far pointer at `DS:49D2` stays
`3996:0000`; words at that object's `+0x18E/+0x190/+0x192` change
from `(0,5,10)` to `(0,0,11)` between the calls. Both page segments
at `DS:4A16/4A18` remain `A000`, and the active page remains zero.
These are **native** boundary observations. The C printer and
rectangle helpers take a supplied page and do not produce those
party/window RAM or CPU changes. No joined one-Up route parity or
new full-C routine count is claimed.

## Reproduce locally

Use the pinned private input under `resources/` and the patched
DOSBox-X. Load slot A, wait for `(15,4,N)` at 10:50, perform Search
and wait for the command prompt, then arm `BP 0F2D:0626` and
`BP 08DE:2B5A`. Resume and send exactly one Up. Capture both first
printer stops and the second printer entry/return. In a separate
same-save replay, stop at the first printer return, then arm
`BP 0F2D:00B7`, `BP 131C:16B7`, and `BP 0C15:01F6` to capture the
row-15 rectangle frame, first store, and immediate return. Select
EGA read map with `OUTP 3CE 04`, `OUTP 3CF <plane>`, then use
`MEMDUMPBIN A000 0000 1F40` per plane; restore read map 0 and
Graphics Controller selector 5 before `RUN`. Dump RAM with
`MEMDUMPBIN 0000 0000 A0000`. Preserve each dump privately before
the next debugger command overwrites it.

```sh
cmake -S . -B resources/build-slot-a-post-up-bridge \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-post-up-bridge --target \
  poolrad_slot_a_full_ega_printer_page_probe \
  poolrad_resident_0709_rect_fill_probe
python3 -m unittest tests/replay/test_slot_a_post_up_row15_clear_bridge_audit.py
python3 tools/replay/slot_a_post_up_row15_clear_bridge_audit.py \
  --exe resources/oracle-runs/slot-a-post-up-next-ui-p9-112/POOLRAD/START.EXE \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --printer-run resources/oracle-runs/slot-a-post-up-next-ui-p9-112 \
  --watch-run resources/oracle-runs/slot-a-post-up-reset-watch-p9-113 \
  --printer-probe resources/build-slot-a-post-up-bridge/poolrad_slot_a_full_ega_printer_page_probe \
  --rect-probe resources/build-slot-a-post-up-bridge/poolrad_resident_0709_rect_fill_probe
```

The public audit locks media, emulator, seven RAM dumps, every page
plane, original and loaded call/body spans, first-store instruction
hash, native route fields, private source/font hashes, and exact C
outputs. Its four synthetic tests reject altered rectangle frames,
the parent return, off-region page deltas, wrong plane distribution,
and a changed window far pointer. No executable, save, source text,
font, screenshot, RAM, or EGA payload is tracked.
