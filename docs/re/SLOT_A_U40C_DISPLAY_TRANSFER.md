# Saved first-wall object reaches the EGA display-page transfer

The two saved calls to unit `0x40C` entry 2 mutate an offscreen object.
Their [bounded plane-byte model](SLOT_A_U40C_FIRST_TILE_PLANE.md) found the
first changed data byte at offset 4,734. This follow-up traces that byte
through the **post-wall flush**: the original copies dirty object rows to
EGA staging segment `0xA200`, then calls runtime `Move` from `0xA200` to
active page `0xA000`. In one private native replay, the byte at offset
`0x0A08` on EGA read plane 2 changes from 0 to 255 at **both** segments.
This is framebuffer-byte evidence; it does not assert a particular displayed
RGB pixel, palette colour, or screenshot match.

Before debugging, `deja "Poolrad saved slot A first view DS 4A20
destination object 4734 display transfer framebuffer write"` and the
shorter `deja "Poolrad bitmap display transfer"` found no exact session.
`deja "4A20"` recalled the resident bitmap-constructor lead. Reused and
independently checked: the [first tile note](SLOT_A_U40C_FIRST_TILE_PLANE.md),
the [first-wall caller](SLOT_A_U3BD_FIRST_WALL_TILE.md), the
[bitmap constructor](RESIDENT_464_BITMAP_CONSTRUCTOR.md), and the
[mode/page setup](RESIDENT_VIDEO_INIT_4426.md). The prior whole-view
screen-difference observation was **not** used as proof for this byte.

## Hash-locked original code

Ignored `START.EXE` has SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
ignored `GAME.OVR` has SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Capstone 5.0.7 decodes each span below to its exclusive end:

| Original span | File range | Bytes / instructions | SHA-256 |
| --- | --- | ---: | --- |
| Unit `0x3BD:+0x05F3..+0x05FC`, local flush wrapper | `[0x2EEEA,0x2EEF4)` | 10 / 5 | `1caa4f4b724b9f109077bbaa3afce253092779ecc08504cc3062c82d2da20675` |
| Unit `0x3BD:+0x0DAA..+0x0DB3`, entry-5 exit | `[0x2F6A1,0x2F6AB)` | 10 / 5 | `7dec4d4d92bb5fffbafdc4de698caef9e181d36f059162d329066df4f06867a3` |
| `RES_0464:1B52..1E5B`, ending at next prologue | `[0x061CB,0x064D5)` | 778 / 305 | `7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c` |

`RES_0464:1B52` maps to loaded image `0x06192..0x0649B` and has six
calls and 24 branches across its three video-mode paths. The saved route
uses **only** video-state byte `DS:[0x4A1E]=1` at `+0x1B58..+0x1BD1`.
Unit `0x3BD:+0x0DAB` calls local `+0x05F3`; that helper calls
`RES_0464:1B52` at `+0x05F6`. The native stop is `0C88:1B52` with return
frame `SS:3E94 = 2D14:05FB` and outer return `SS:3E9A = 2D14:0DAE`.
Thus this flush occurs at the **end** of the reached entry-5 wall scan,
after the item-1 and item-13 calls. The native view then returns at
`3582:3387`.

## Reached row-56 byte path

At flush entry, live `DS:[0x4A20]` points to object physical `0x3DE48`.
The byte at object data offset 4,734 is 255 and stays 255 through return.
This was the byte changed from 0 to 255 by the second unit-`0x40C` call.
Row 56 is dirty (`DS:[0x4A28+56]=1`), with first and last interleaved
data indices 4,712 and 4,755 (`DS:[0x4C20+2*56]` and
`DS:[0x4D70+2*56]`), and low display offset 2
(`DS:[0x4AD0+2*56]`). The byte is index `(4734-4712)/4=5` in plane 2.

In video mode 1, `+0x1BD1..+0x1BD6` selects Sequencer map-mask
register 2. The four loops at `+0x1C22..+0x1D25` set its values from
`DS:[0x25F6..+0x25F9]`. The reached plane-2 value
`DS:[0x25F8]=4` selects EGA plane 2. The loop copies the object byte
through its `+0x1CC6` store to `0xA200`:

```text
DS:[0x5050 + 2*(56+8)] + low + 1 + byte_index
= 2560 + 2 + 1 + 5 = 2568 = 0x0A08
```

The first reached `0xA200` store after the two tile calls was observed
at loaded `0C88:1C37`, on dirty row 16 (source index 1,352,
destination offset 963). A native memory watch at `A200:0A08` later
stopped at `0C88:1C3A` just after the row-56 plane-0 store; the saved
locals then held row 56, source index 4,732 and destination offset
2,568. The two ignored 640 KiB RAM images at those stops have SHA-256
`e4b0546cb746135fc2299d3bfa80b20e7172b5b94d93df02cb799022bbc2da0d`
and `fd9cb5b9418ea293bc3944d52212dc75c48c3784b450e49915ed1d4ae2643c92`,
respectively. The plane-2 `+0x1CC6` store itself was not separately trapped;
the decoded loop, its row-56 inputs, and the same-run plane-2 RAM/VRAM
comparison establish its reached effect.

After the four plane loops, `+0x1D31..+0x1D49` enables all Sequencer
planes and EGA graphics-controller write mode 1. For each dirty row,
`+0x1D5E..+0x1DA0` calls runtime `0xAF8:1685` (`Move`) with staging
source `0xA200` and page destination `0xA000`. For row 56 its source and
destination start at `DS:[0x5060]+1+56*40+low = 0x0A03`; the length is
`((4755-4712) >> 2)+1 = 11` bytes, so offset `0x0A08` is inside the
call. The individual runtime store instruction inside `Move` was not
traced. The flush clears the dirty flags at `+0x1E25`; native row 56
changes from dirty 1 to 0, with the extent words reset to `0xFFFF` and 0.

## Same-run private numeric comparison

The second ignored replay stopped at `0C88:1B52` and `3582:3387`.
Its full conventional-RAM SHA-256 values are
`552f260119eeb651a33455a6ab512560c6a5c323d301e00900b31bc00b43b958`
at flush entry and
`da6687e8d259188daf97ac9f9d5753ca173f9bfd737abadf1a531ae844350944`
at return. Both `DS:[0x4A1A]` and `DS:[0x4A1B]` select page 0;
active/draw segments `DS:[0x4A16]` and `DS:[0x4A18]` are `0xA000`.
The debugger temporarily selected EGA read-map plane 2 for each 32-byte
VRAM sample; before resuming, it restored read map 0 and the original
graphics-controller selector 5.

| Segment / plane-2 sample | Entry SHA-256 | Return SHA-256 | Byte `+0x0A08` |
| --- | --- | --- | --- |
| `A200:[0x0A00,0x0A20)` | `66687aadf862bd776c8fc18b8e9f8e20089714856ee233b3902a591d0d5f2925` | `3c21f304946ef5c6b483a7e38bd4fe52a3f29ecca3e0ce191a6a05d69188400c` | 0 → 255 |
| `A000:[0x0A00,0x0A20)` | `1bcc1bbf3316abe5c8b57be7dba5c7e7b1c7f9dc078913f6a1d367dd876b5849` | `74c7e9e1450eb255e2b86a1289042fc6492b5db841d1fea6ab37c2494841344e` | 0 → 255 |

The first replay separately pinned `0C88:1C37` and the row-56 watchpoint.
The second replay supplies the same-run plane-2 before/after comparison.
The whole 32-byte sample changes at 11 offsets in each segment; those
other bytes are not assigned to the item-13 tile here. No screenshot or
RGB pixel is used as evidence. This note does not translate the 778-byte
flush routine, the EGA register model, or runtime `Move`, and adds no
strict full-C or game-identity count.

## Reproduction and private boundary

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/replay/slot_a_u40c_display_transfer_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/slot-a-u40c-display-p5-002
```

The read-only audit checks hashes, instruction alignment and transfer
sites, the post-wall native return frame, row metadata, page state,
source/target arithmetic, dirty reset, and both numeric VRAM transitions.
It prints no game bytes or pixel arrays. All DOS binaries, saves, RAM/VRAM
snapshots and screen captures remain under ignored `resources/`. A future
bounded C model of this flush requires an explicit EGA register/page hook
and a defined `Move` boundary; this evidence note intentionally stops
before claiming full-routine parity or visible RGB output.
