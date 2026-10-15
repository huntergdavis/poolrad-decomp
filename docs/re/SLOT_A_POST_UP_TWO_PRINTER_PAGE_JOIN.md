# Saved slot-A post-Up two-printer page join

This is a bounded **page-value** route across the first post-Up
`RES_0709:0626` printer, the reached row-15 `RES_0709:00B7` rectangle,
and the next `RES_0709:0626` printer. The same-run C route carries four
16,192-byte EGA plane windows through all three calls. Its outputs match
all **64,768/64,768 bytes** at each immediate native return. It accepts
captured call frames and initial EGA state; it does not dispatch Up or
calculate party and window state. No full-C routine count or joined one-Up
CPU/RAM parity is claimed.

Before this work, `deja "Poolrad slot A post-Up first printer row15 clear
second printer persistent C page route A000 A200 native metadata missing
dependency"` timed out after 20 seconds without a reusable result. The
merged [first-view printer page evidence](SLOT_A_AFTER_ROW9_FULL_PAGE_SEQUENCE.md)
and [rectangle model](RES_0709_RECT_FILL_00B7.md) supplied prior address
and page-layout leads. The immediately preceding local row-15 clear bridge
at commit `442c7441d0aeff31fa46a2225373f4ecc843b9b8` supplied the
clear-frame lead. This run independently rechecks each original and loaded
call, the live frames, six RAM snapshots, and six four-plane windows.

## Input and call proof

The ignored DOS v1.3 `START.EXE` hashes to
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` to
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
the owner slot-A save to
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`;
and this private heavy DOSBox-X binary to
`c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457`.
The live 1,416-byte font block hashes to
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
All original media and captures remain ignored under `resources/`.

| Proven span | Bytes | SHA-256 |
| --- | ---: | --- |
| Expanded START `[0x36F5,0x36FA)` far call to `0709:0626` | 5 | `04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23` |
| Loaded `08DE:2B55..2B5A` far call to `0F2D:0626` | 5 | `cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585` |
| Expanded START `[0x76B6,0x7737)` printer body | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` |
| Loaded `0F2D:0626..06A7` printer body | 129 | `8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164` |
| Expanded START `[0x36E2,0x36E7)` parent call to `03F1:0137` | 5 | `395783ec4ad75a0552ed076373c516213f522e5aacc77c12d8fbf9ead76dd23f` |
| Loaded `08DE:2B42..2B47` parent call to `0C15:0137` | 5 | `2f89ade8cd970b19f81e674324278b7e2f7845bd119dbf92374051d645fc1850` |
| Expanded START `[0x4101,0x4106)` rectangle call to `0709:00B7` | 5 | `2286767d9eccf2e3fe83669ef5367f5ef97da8c2145e12d54b0291510e19093c` |
| Loaded `0C15:01F1..01F6` rectangle call to `0F2D:00B7` | 5 | `8d1b1dec8fbef9dea92ee3e851c55572850da6b639cd9fb3890c998e918cc8e2` |
| Expanded START `[0x7147,0x7201)` rectangle body | 186 | `f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5` |
| Loaded `0F2D:00B7..0171` rectangle body | 186 | `02ba954ed5bc0baa194fa7e74eeaceba27b09386107ac51df7df27a09c6aaa5f` |

The first printer enters at `0F2D:0626`, `SS:SP=1D16:3C6E`, returning
to `08DE:2B5A`. Its private Pascal source starts at `1D16:3E7D`, has
length 19, and its length-plus-content SHA-256 is
`e767d5c8ef552436c5e411af72d8eca9581f3feacbb988c67c16bf2b701bc2fe`.
The reached rectangle enters at `0F2D:00B7`, `SS:SP=1D16:3B62`, with
return `0C15:01F6` and low-byte frame `(color 0,page 0,bottom 15,right
38,top 15,left 17)`; its parent returns to `08DE:2B47`. The second
printer enters at `0F2D:0626`, `SS:SP=1D16:3B7E`, again returning to
`08DE:2B5A`. Its private source starts at `1D16:3D8D`, has length 19,
and hashes to
`f13e024bc34f688552e96d6cfb8be870b68d2ada58f4ae6fdea7fd7e583e9860`.
Both printer frames have low-byte `(color 10,row 15,column 17)`.

## Same-run native and C page state

Each of the four plane captures spans 16,192 bytes: A000 page 0 at
offsets `[0,8000)`, a 192-byte gap, then A200 page 1 at
`[8192,16192)`. At all six stops, `DS:4A16=DS:4A18=A000`, the active
page byte `DS:4A1A=0`, EGA mode byte `DS:4A1E=1`, and the actual window-0
far pointer at `DS:49D2` is `3996:0000`.

| Native stop | A000 32,000-byte SHA-256 | Party `DS:6AAD..6AAF` | Window-0 words `+18E,+190,+192` |
| --- | --- | --- | --- |
| First printer entry | `b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44` | 15,4,6 | 0,5,10 |
| First printer return | `a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac` | 15,4,6 | 0,5,10 |
| Rectangle entry | `a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac` | 14,4,6 | 0,0,11 |
| Rectangle return | `b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44` | 14,4,6 | 0,0,11 |
| Second printer entry | `b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44` | 14,4,6 | 0,0,11 |
| Second printer return | `1019a688b324afa165f731013c6491c0cb54232262403f7c70cbb7fb97543773` | 14,4,6 | 0,0,11 |

The A200 page is unchanged at all six stops, SHA-256
`f1d00351b9c1e79264fcb2b6804a32b26570c811aa50498185409205f07d416b`.
Each direct call changes 210 A000 plane-byte values and zero A200 values.
The first printer return equals the rectangle entry over all 64,768
bytes, and the rectangle return equals the second printer entry. The
page audit hash-locks all six 640 KiB RAM captures, all 24 plane
captures, and each concatenated 64,768-byte window; the hash table lives
in `slot_a_post_up_page_join_audit.py`. These equality checks describe
values at stops and do not count same-value or transient guest writes.

The C route feeds the same four initial plane windows to the existing
printer page model, then the decoded rectangle fill, then the printer
again. It receives each measured frame as input, preserves the 192-byte
gaps, and keeps the page state persistent. C reports one distinct page,
19 cells, 608 modeled plane stores and 210 changed values for each
printer; the rectangle reports 704 modeled plane stores and 210 changed
values, restoring draw segment A000. The three C output windows match
their respective native return windows **64,768/64,768 bytes each**.
Modeled store counts are C results, not native bus-write counts.
The focused synthetic test passed **1/1** and the full strict
CMake/CTest suite passed **246/246** on this branch.

The first missing C state dependency in this joined route lies between
the first printer return and the rectangle entry: the party tuple changes
`(15,4,6) -> (14,4,6)` and window-0 words change
`(0,5,10) -> (0,0,11)` while the EGA window is unchanged. This page
model does not assign those results or identify their writer.

## Reproduce and next gates

Use a fresh ignored run from `tools/oracle/prepare_run.sh`, copy the
hash-pinned slot-A input into that run, and start the private heavy
DOSBox-X with `DEBUGBOX START`. Advance title and codeword prompts, load
slot A, press Search, and wait until the BIOS keyboard polling path at
`1264:0302`. Arm `BP 0F2D:0626`, resume, and send exactly one Up.
Capture RAM and all four EGA plane windows at the first printer entry
and `08DE:2B5A` return. The first generic rectangle after that return is
unrelated; arm `BP 0C15:01F1` to reach the measured parent call, then
`BP 0F2D:00B7` and capture its entry and `0C15:01F6` return. Capture
the next `0F2D:0626` entry and `08DE:2B5A` return. Use
`MEMDUMPBIN 0000 0000 A0000` for RAM; for plane 0..3 select graphics
controller register 4 with `OUTP 3CE 04`, select the read map with
`OUTP 3CF 00` through `03`, and dump `MEMDUMPBIN A000 0000 3F40`.
Restore read map 0 and selector 5 before every `RUN`. Keep captures under
ignored `resources/`.

```sh
cmake -S . -B resources/build-slot-a-post-up-page-join \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-post-up-page-join -j4
ctest --test-dir resources/build-slot-a-post-up-page-join \
  -R '^poolrad_slot_a_post_up_page_join_tests$' --output-on-failure
ctest --test-dir resources/build-slot-a-post-up-page-join \
  --output-on-failure -j4
python3 tools/replay/slot_a_post_up_page_join_audit.py \
  --run resources/oracle-runs/slot-a-post-up-two-page-join-p9-115 \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --probe resources/build-slot-a-post-up-page-join/poolrad_slot_a_post_up_page_join_probe
```

Two disjoint follow-ons remain. The gameplay lane can locate the first
intercall party/window writer and connect its C state before the rectangle
frame. The UI lane can follow the next page writer after the second
printer return using this exact persistent page as its starting state.
Neither follow-on is supplied by this bounded page route.
