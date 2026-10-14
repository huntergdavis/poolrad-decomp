# Saved first-view postflush resident byte writer

The saved slot-A first-view route reaches a masked offscreen byte store at
`RES_0464:+0C6A`. A native stop immediately before its 17-instruction
iteration at `0C88:0C3D`, followed by a stop at `0C88:0C6E`, changed
exactly one byte in the 640 KiB DOS RAM image: physical `0x3F0DD`.
This is destination object data offset `0x127E`, plane
2 of four. The portable C slice matches all 14 captured CPU fields and the
entire RAM image at the stop. It ends with the typed
`PR_R464_MASKED_BYTE_AT_0C6E` boundary. This does not translate the whole
resident draw routine or assert final visible EGA parity.

Before debugging, `deja "Poolrad RES_0464 first value-changing postflush art
writer saved first-view route"` returned no matching prior session. This
audit reused the merged [first-wall tile handoff](SLOT_A_U3BD_FIRST_WALL_TILE.md),
[first tile plane](SLOT_A_U40C_FIRST_TILE_PLANE.md), and
[static view](SLOT_A_STATIC_VIEW_FIRST_REGION.md) evidence. The first tile
plane note already models destination data over 32 stores but leaves the
instruction-level CPU and interleaved resident effects open. This slice
independently checks one original masked-store iteration.

## Original-byte and native route evidence

Ignored `START.EXE` SHA-256:
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The unpacked image span `[0x527D,0x52AE)` maps byte-for-byte to live
`0C88:0C3D..+0C6E`: 49 bytes, 17 aligned x86 instructions, SHA-256
`b351be8dfc49d233bbf25a6a75e0fe42f05cbeaac0a0b0478c2ee0cb3c32e16e`.
The last instruction, `+0C6A`, stores the merged byte at
`ES:[DI+0x17]`; `+0C6E` begins the following loop step. The span has
no call. The later resident loop and any callbacks are outside this model.

In a new saved-slot-A run, the first flush return stopped at
`2D14:05FB`. The first postflush unit-`0x3BD` tile call at
`2D14:02FB` had `(x,y,item)=(7,6,1)`; its return at `+0300`
left all 14,135 offscreen object bytes unchanged. The next call had
`(7,7,13)`. Its third reached `RES_0464:+0C3D` plane iteration
read source, auxiliary-mask and destination far pointers from the live
frame; the destination address was `3DE4:129D`. The first two iterations
were plane 0 and plane 1 at adjacent offsets. The same-run
`+0C3D → +0C6E` comparison found one RAM change at physical
`0x3F0DD`, and no other memory effect.

The original sequence reads source byte `ES:[DI+0x17]`; loads a mask
pointer from the source header `+0x13`; reads mask byte at the
`[BP-0x10]` index; loads the destination far pointer from `[BP+6]`;
uses `[BP-0x0A]` as the destination data index; then stores
`(old & mask) | source` at destination `+0x17`. Its final
`ADD DI,AX` supplies the CPU flags at `+0C6E`. The C model uses
16-bit offset arithmetic and a 20-bit real-mode RAM address, checks every
read and the one write before mutation, and reports the boundary if RAM
is inaccessible.

## Reproduction and limits

`tools/replay/res464_postflush_masked_byte_audit.py` hash-locks the
ignored native transcript, full-RAM stops, and original executable. Build
`src/recovered/resident_464_masked_byte_0c3d.c` as a strict shared
library, then pass the ignored `START.EXE`, private run directory, and
library to the audit. It verifies the route's first unchanged tile object,
the exact 49 original code bytes, 17 decoded instructions, 14 CPU fields,
the full 640 KiB RAM image, and the one plane-2 change. The synthetic
CTest covers changed and unchanged stores, 16-bit offset carry/wrap, wrong
entry, and atomic host-boundary failure.

The original game's broader resident draw loop, later offscreen-to-EGA
transfer, and any unmeasured callback remain separate boundaries. No
full-routine or game-identity count is claimed.
