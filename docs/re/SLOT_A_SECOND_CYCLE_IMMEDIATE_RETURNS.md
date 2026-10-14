# Saved slot-A first view: second printer-cycle immediate returns and first row clear

This local audit separates each C printer call from the page work between
calls. Given its **own measured native entry page**, each of printer calls
25..42 produces exactly the native immediate-return page: **18/18 calls,
32,000/32,000 plane bytes per call**. This is per-call parity. Persistent
printer-only composition across the second cycle is not established because
five page changes occur between printer calls. The terminal first-view page
still equals the first-cycle final page, as observed in the preserved
two-cycle PR #375 evidence.

Before debugging, `deja "Poolrad slot A first view second-cycle printer calls
25 through 42 immediate EGA entry return page parity reset 410 bytes"` was
run with a 20-second limit and timed out, so no indexed session result was
reused. The audit reuses the merged
[printer-page C model](SLOT_A_FIRST_VIEW_PRINTER_PAGE.md), the
[row-seven `RES_03F1:0137` caller proof](SLOT_A_ROW7_TWELVE_CELL_PRINTER_PAGE.md),
and the [native BPM protocol](SLOT_A_FIRST_CLEAN_PAGE_GLYPH_WRITE.md).

## Inputs and independent immediate returns

Private replay `slot-a-ui-second-cycle-p9-023` contains RAM at each call
entry and four full 8,000-byte EGA planes at each entry and immediate
return. A separate private replay `slot-a-ui-second-cycle-watch-p9-024`
supplies the row-clear write watch. Both use the same pinned slot-A save.
No RAM, page, font, text, save, screenshot, or executable payload is tracked.

| Input / capture set | SHA-256 or count |
| --- | --- |
| DOS v1.3 `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| slot-A save, each run | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| heavy DOSBox-X debugger | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |
| 18 entry RAM captures, concatenated in call order | 11,796,480 bytes; `893b7f88ce2f9983264e99c0008842b7095bd97e6538a6789c3e43164f09c0c5` |
| 18 entry/return four-plane page pairs, concatenated | 1,152,000 bytes; `650fde0e71c40848f90d50f48fe2738d1049a333a3b244f93fc174c1794a9748` |
| call-24 return, call-25 entry, and terminal pages, concatenated | 96,000 bytes; `dd99ce02cf7024ecc30a514cbd197c410f4dc441515b8cfee44c784abe08e873` |
| first row-clear watch RAM | `be3ece5652d1746b146b42bb69d9bb882330be4ad69c4d8f3741d026ca4821f2` |

At each call, the audit verifies the loaded printer at `0F2D:0626`, the
loaded far call and `08DE` return frame, the 1,416-byte live font hash,
EGA page mode, source length, color, row, and column. It supplies that
call's *captured native entry page* to the existing portable C full-page
probe. Its output equals that call's *captured native immediate return*.
Across 18 calls, C reports **69 cells, 2,208 plane-byte stores, and 1,404
per-call value changes**. The last number sums changes within independent
calls; it is not a net whole-frame delta.

Call 24's return page hashes to
`3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738`.
The 410-byte inter-cycle reset produces call 25's entry page hash
`2aa65739f6bc280936e6203d6bb6cbaad26663475ee86e51468d48a4b7b4bfdf`.
Call 42's immediate return and the actual `3582:3387` first-view return
both hash to the call-24 page. The two cycles' intermediate page routes
remain distinct.

## Five native inter-call effects

All inter-call page changes below set the destination byte to zero. The
other 12 adjacent return/entry pairs are equal. Each row interval spans
eight scanlines; the listed value-change count excludes same-value stores.

| Native boundary | Changed plane bytes | Screen scanlines | Byte columns |
| --- | ---: | --- | --- |
| call 27 return → call 28 entry | 154 | 40..46 | 17..38 |
| call 30 return → call 31 entry | 196 | 48..54 | 17..38 |
| call 33 return → call 34 entry | 273 | 56..62 | 17..38 |
| call 36 return → call 37 entry | 196 | 64..70 | 17..38 |
| call 39 return → call 40 entry | 175 | 72..78 | 17..38 |

Only the **first** of these five effects has a live writer/caller watch
below. The other four have measured entry/return deltas and geometry,
without a separately watched guest writer. No expected native page is
copied into a persistent C composition state.

## First row-boundary writer and caller

In the fresh write-watch replay, the seventh `08DE:09C3` return was the
call-27 return. `A000:0651` still read 124 there, matching plane 0 of the
independent call-27 return capture. After arming `BPM A000:0651`, DOSBox-X
first reported its watch baseline `00 → 7C` at the unchanged return PC.
The next event was the actual `7C → 00` write at `131C:16B7`, the loaded
SYSTEM `rep stosb` instruction. At that stop, `ES=A000`, `DI=0655`,
`AX=0`, `CX=0012`, `DS=14A0`, `SS=1D16`, and `BP=3E92`. The repeated
store had advanced four byte positions when the debugger reported the
selected change; the sampled post-watch page byte was zero.

The retained stack returns from SYSTEM fill to `0F2D:0145`, immediately
after the far call at `RES_0709:0140`. Its enclosing
`RES_0709:00B7` rectangle-filler frame returns to `0C15:01F6` and has
low-byte arguments `(color=0, page=0, bottom=5, right=38, top=5, left=17)`.
That loaded `0C15:01F1` call lies in `RES_03F1:0137`; its own frame
returns to `08DE:07EF` from `RES_00BA:07EA` with `(5,38,5,17)`.
This is the reached row-five clear request, not a printer call.

| Bounded original/loaded span | Bytes | SHA-256 |
| --- | ---: | --- |
| `RES_0709:00B7..0171`, expanded `[0x7147,0x7201)` | 186 | `f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5` |
| loaded `0F2D:00B7..0171` | 186 | `02ba954ed5bc0baa194fa7e74eeaceba27b09386107ac51df7df27a09c6aaa5f` |
| `RES_03F1:0137..02E8`, expanded `[0x4047,0x41F8)` | 433 | `1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc` |
| loaded `0C15:0137..02E8` | 433 | `f50a7fde3dcc5886f55beb81c5fcdc22efe4348b52d8df35c70a646d5a309d5a` |
| loaded SYSTEM `131C:16A8..16BC` fill | 20 | `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7` |
| original `RES_03F1:01F1` far call, expanded `[0x4101,0x4106)` | 5 | `2286767d9eccf2e3fe83669ef5367f5ef97da8c2145e12d54b0291510e19093c` |
| loaded `0C15:01F1` far call | 5 | `8d1b1dec8fbef9dea92ee3e851c55572850da6b639cd9fb3890c998e918cc8e2` |
| original `RES_00BA:07EA` far call, expanded `[0x138A,0x138F)` | 5 | `395783ec4ad75a0552ed076373c516213f522e5aacc77c12d8fbf9ead76dd23f` |
| loaded `08DE:07EA` far call | 5 | `2f89ade8cd970b19f81e674324278b7e2f7845bd119dbf92374051d645fc1850` |

The reached `RES_0709:00B7` path computes the EGA byte offset from row 5
and column 17, an inclusive 22-byte width, and eight scanlines. Clearing
that rectangle on all four planes of the measured call-27 return page
exactly produces the measured call-28 entry page: **32,000/32,000 bytes**,
SHA-256 `570ad46e976182f1227b76bdb367fa0303bd789c42bfa095770a74cb5f7aa52f`.
This checks the bounded row-five effect. It does not translate the full
186-byte rectangle routine or prove same-value store counts.

## Local reproduction

Use the existing full-page C probe built with
`PR_PAGE_BASE=0` and `PR_PAGE_BYTES=8000` in a strict CMake build, then:

```sh
python3 tools/replay/slot_a_second_cycle_immediate_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-ui-second-cycle-p9-023 \
  --watch resources/oracle-runs/slot-a-ui-second-cycle-watch-p9-024 \
  --probe resources/build-slot-a-first-view-two-cycle/poolrad_slot_a_full_ega_printer_page_probe
```

The first replay's 144 full-plane entry/return files and 18 RAM files,
and the watch replay's RAM and selected pre/post byte files, remain ignored
under `resources/`. This note and the audit contain only addresses, hashes,
counts, numeric register/argument values, and instruction evidence.
