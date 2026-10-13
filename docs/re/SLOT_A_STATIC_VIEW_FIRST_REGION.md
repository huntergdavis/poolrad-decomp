# Slot-A static view: first original-art region

This is a bounded first step from the owner slot-A save to a portable C
indexed frame. The merged world-save adapter supplies position, facing,
clock, and map ID; the GEO and EGA readers supply raw map fields and original
art indices. The new frame records which pixels were drawn. Its first
private comparison covers **128 of 64,000 EGA pixels**, with **zero
mismatches** against a normalized DOSBox saved-screen capture. The remaining
63,872 pixels are explicitly uncovered. This is a partial frame and adds no
full-C game routine or identity count.

Before analysis, `deja "Poolrad slot A first static save to view frame original
DOS art screen normalization"` found no exact match. The shorter
`deja "Poolrad first view save render"` surfaced the earlier
`WALL_RENDER_DECODED.md` session. That older note was used only to look for
8X8D/WALLDEF leads: its 70/31/45 claim for blocks 1/3/5 does not agree
with the present DOS corpus audit's 70/70/70. The merged
[save adapter](SLOT_A_NATIVE_SAVE_ADAPTER.md),
[EGA reader](../formats/EGA.md), [GEO parser](../formats/README.md), and
[native first-view route](SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md) are the
reused, independently checked contracts.

## Private inputs and route boundary

| Ignored input | SHA-256 |
| --- | --- |
| `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| `GEO2.DAX` | `2035246014efdb26de3114f4737d52a935ad8834f79b4a8e4880da2922b6d595` |
| `8X8D1.DAX` | `cce9313f23a600200fec60b5d46cd08aa9535a6143eee5d5d768a1d4e0415021` |
| ignored DOSBox baseline PNG | `2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

The save adapter restores `(x,y,facing)=(15,4,6)`, clock words `(0,5,10)`,
map ID 20, and ECL file byte 2. Map ID 20 is present in `GEO2.DAX`; its
unpacked 1,026-byte block is accepted by `pr_geo_parse`. The C context
reports all four raw wall and door values plus the event byte at the saved
cell, without assigning any visual face to them. It rejects a mismatched GEO
block ID and out-of-range coordinates. A valid-size GEO block is not
authenticated by the parser, so the local audit pins the source hash.

The native first-view trace proves unit `0x1C:+3382` reaches unit `0x3B4`
entry 1 at `+0x016D` and returns to `+0x3387`. Independently decoding the
hash-pinned unit `0x3B4:+016D..+02A4` shows its saved-position path calls
unit `0x3BD` entry 5 through stub `0x3D5:0x39`. That callee begins at
`+0x05FD` and returns with `retf 6` at `+0x0DB1`. Its code span
`+0x05FD..+0x0DB3` is 1,975 bytes, SHA-256
`f618ac7c5ada7bda627b378fc0d6072ac9a882ab0e3f81ae29cea289cb5055a0`
(827 linear decoded instructions, 39 calls, 62 branches). The 84 bytes
from `+0x0DB4` up to the next entry at `+0x0E08` are not counted as code;
they include inline data. Its complete draw
selection, blitter effects, and font use remain outside this C slice.

## Exact bounded pixel comparison

The private 1280×1024 screenshot contains a centered 640×400 DOSBox screen
at `(320,312)..(960,712)`. Every 2×2 pixel square in that crop has one RGB
color from the standard 16-color EGA palette. The checker converts it to a
320×200 indexed frame. It invokes a C probe that reads the ignored save,
GEO2 and 8X8D1 archives through the merged parsers, then blits two 8×8
items from original 8X8D block ID 1. The measured item/placement pairs are
`(40,40,64)` and `(41,48,64)` in `(item,x,y)` order. Those two placements
are **empirical screen matches**, not a recovered DOS instruction path for
tile selection. No pixel values or screenshot crop are embedded in source.

The C frame has an index plane and a separate coverage plane; all unknown
pixels remain uncovered. The hash-locked private comparison reports:

```text
normalized=320x200 covered=128 mismatches=0 unknown=63872
```

The generic `pr_slot_a_view_blit_tile` checks image layout, item and frame
bounds before changing output, and supports index-zero transparency. The
synthetic test checks save-to-GEO binding, all four directional raw values,
frame-edge placement, overlapping transparency, invalid input, and failure
immutability. The private checker reads the original art and capture without
writing any derived pixels into Git.

## Reproduction and next boundary

```sh
cmake -S . -B resources/build-slot-a-static-view \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-static-view \
  --target poolrad_slot_a_static_view_tests poolrad_slot_a_static_view_probe
ctest --test-dir resources/build-slot-a-static-view \
  -R '^poolrad_slot_a_static_view_tests$' --output-on-failure
python3 tests/replay/check_slot_a_static_view.py \
  resources/build-slot-a-static-view/poolrad_slot_a_static_view_probe \
  resources/DOS/POOLRAD/SAVGAMA.DAT \
  resources/DOS/POOLRAD/GEO2.DAX \
  resources/DOS/POOLRAD/8X8D1.DAX \
  resources/oracle-runs/slot-a-walk-001/slot-a-before.png
```

The next parity gate is the saved path inside unit `0x3BD` entry 5 and its
draw callees: derive tile IDs and screen placement from instructions rather
than the empirical match. Completing the frame also needs the remaining
wall/door composition, text/font block 201 (outside the current EGA reader),
and palette/state changes. The native `+3382..+3387` screen delta in the
merged first-view note is a distinct replay and is not equated with this
saved-screen baseline.
