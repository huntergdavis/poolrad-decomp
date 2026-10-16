# Cold-start credits source-to-frame contract

This note identifies the original source of the DOS v1.3 cold-start credits
frame. It is an asset and static-call contract with a read-only comparison to
an existing native frame. It does not establish the first live display writer,
instruction-level startup reach, or C/native route parity. Original text,
tiles, fonts, and frames remain in ignored `resources/` directories.

Before analysis, a bounded `deja "Poolrad cold start credits frame slots 443
595 text font border palette first writer source"` attempt failed because its
local `index.db.lock` was read-only. The investigation instead reused the
checked-in [screen-group evidence](NATIVE_COLD_START_SCREEN_GAPS.md),
[unit 0x17 picture-route note](NATIVE_COLD_START_U17_PICTURE_ROUTE.md),
[rectangle-fill contract](RES_0709_RECT_FILL_00B7.md), and
[EGA/DAX format contract](../formats/EGA.md), then independently checked the
original bytes and saved native captures.

## Pinned private inputs

The reference `stable-frames/0443-0595.png` is in the native replay worker's
ignored `resources/oracle-runs/native-cold-start-gap-004/` directory. The PNG
file SHA-256 is
`5d434e35fdfcbdc62970ed61d365763497be64234ed98c491b3dcb72cd33f295`.
Its decoded 640×400 row-major RGB24 bytes (768,000 bytes) have SHA-256
`624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698`.
Every 2×2 output block has one color, yielding 320×200 source pixels.

The source images are `GAME.OVR`, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
`START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
and `8X8D1.DAX`, SHA-256
`cce9313f23a600200fec60b5d46cd08aa9535a6143eee5d5d768a1d4e0415021`.
The read-only native RAM snapshots are `ram-main-011d.bin`,
`ram-u17-entry.bin`, and `ram-main-0122.bin` in the ignored
`resources/oracle-runs/native-cold-start-picture-002/` capture. Their
SHA-256 values, respectively, are
`3de44fb40c3490f5e540d7ec1ffff7e764326eb6badcfc1638144ac08bfb45d6`,
`11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6`,
and `6e0ae646dc59e13992f760e0a899c7482d62b62b375f7d1f2934e514f4a4a3fe`.

## Original code and asset route

Original unit 0x17 entry 1 occupies `GAME.OVR` file `[0x06FB,0x08DD)`,
482 bytes, SHA-256
`02adf3ddfc09a9cf558fe9735f53e78858bb4ec57471672d33be6082677f137d`.
At file `0x08D5` (unit runtime `+0x0801`) its static path near-calls the
unit-local credits procedure at runtime `+0x0206`, file `0x02DA`. The latter
occupies file `[0x02DA,0x06F5)`, 1,051 bytes, SHA-256
`de98811361eae3a03facc8107f2ebee7fd48d4f181425351b5e5644d453a22da`.
The entry has an earlier `DS:49D1` gate; this note does not claim that the
saved native run was stopped at the credits call.

The local procedure supplies two empty-caption rectangles to resident
`RES_03F1:02E8` at file `0x0301` and `0x0327` (unit runtime `+0x022D` and
`+0x0253`). Their inclusive cell arguments are `(1,1,38,3)` and
`(1,5,38,23)`. That helper clears via `RES_03F1:0137`, then draws the border
via `RES_03F1:0000`. In mode 1, the clear uses `RES_0709:00B7` on both EGA
pages with color zero. The border expands each rectangle by one cell to
`(0,0)..(39,4)` and `(0,4)..(39,24)`. It calls overlay `0x428:002A`
(unit 0x40C entry 2, runtime `+0x015D`) with the direct-copy flag pair
`(0,0)`; that route copies selected group-4 items and calls resident
`0x464:1951` for display. The three selected item IDs are `0x114` corner,
`0x115` vertical, and `0x116` horizontal, corresponding to group-4 item
indices 20–22 after the live `DS:272A=0x0100` remap. This direct-copy route
is distinct from the saved-view offscreen tile route.

The local procedure then makes 33 `RES_0709:0626` printer calls from 29
distinct Pascal string addresses in its original code. Their source lengths
sum to 459 characters. The calls place 459 non-overlapping cells at rows
1–23 and columns 1–26, with EGA color indices 10, 11, and 14. No original
string bytes are reproduced here. After printing, file `0x06E9..0x06F1`
passes 10 to the local `+0x0000` wait routine. That routine polls
`RES_0709:0C3F`, checks `0xA40:02FD`, and flushes through
`RES_0899:0104` before returning. This is a code path description, not a
timing claim about the saved run.

The live RAM group-4 pointer is `DS:5E4A=41E1:0008` in all three saved
snapshots; its 1,015-byte object SHA-256 is
`2cf4db22f038f8daca0d515c4587d37be9c26065821253b0d8bb51ceaacfc311`.
The 31-item DAX block 202 decodes to 1,009 bytes, SHA-256
`38bcdc776057d1d917030c22d5f02fb2cfaf1bdf28a694a7e42f9e12c85e51fb`.
Converting its packed nibbles to four planar bytes per tile row reproduces
all **992/992** live group-4 payload bytes. The live font pointer resolves
to `3CB6:0000`; its first 1,416 bytes have SHA-256
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`,
matching decoded DAX block 201. The sprite and font payloads are not tracked.

## Read-only source-to-frame comparison

The comparison used original call operands, source string lengths, the
decoded DAX font and border tiles, the standard 16-color EGA RGB palette,
and 2× display scaling. The printer comparison uses the recovered
uppercase-modulo-64 glyph lookup. It compared each predicted source pixel
with the top-left pixel of its corresponding uniform 2×2 native block.

| Region | Cells | Source pixels | RGB mismatches |
| --- | ---: | ---: | ---: |
| Final border placements | 164 | 10,496 | 0 |
| Printer character placements | 459 | 29,376 | 0 |
| Remaining cleared cells | 377 | 24,128 | 0 |
| **Complete 40×25 screen** | **1,000** | **64,000** | **0** |

The second rectangle overwrites the first rectangle's shared border row;
the counts describe final cell ownership. All 377 cells outside the border
and printer footprints are black in the saved native frame. This is exact
source-to-frame RGB agreement for the pinned credits state, independently
from the title-frame comparison. A synthetic C renderer and its native-frame
RGB comparison are separate follow-on work.

The first *possible* display writer on this static credits path is the
`RES_0709:00B7` clear reached through the first box at file `0x0301`.
Neither an instruction stop at that call nor the exact first live pixel
write is available in the saved capture. The source contract therefore
does not earn a strict full-C game-function credit or assert complete
cold-start reach.
