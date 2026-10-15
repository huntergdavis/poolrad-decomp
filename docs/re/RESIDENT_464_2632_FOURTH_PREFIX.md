# Fourth saved-view RES_0464:+2632 prefix

## Boundary and prior evidence

This bounded model starts at overlay `2D14:+020A`, follows its far call to
resident `0C88:+2632`, and stops at `0C88:+2646` **before** the resident's
first runtime `Move` call. It does not model that callee or the fourth
resident return. The +020A image is generated from original instructions
starting at pD's directly captured third-call return `2D14:+01E9`; a
direct DOS debugger +020A stop remains pending. There is no complete
routine, game-identity, or strict-C count claim.

Before implementation, `deja "Poolrad U3BD entry5 fourth 020A RES_0464
2632 020F same-run native prefix"` timed out after 35 seconds without a
result. Reused the locally reviewed third-call
[address note](RESIDENT_464_2632_THIRD_CALL.md), the parent
[setup note](U3BD_AFTER_THIRD_01E9.md), and pD's same-run direct
`+01E9` image. The third C return and original-byte return each compare
against that direct image with **zero differences across 640 KiB**, and
all 14 CPU fields match.

## Original-byte boundary

The ignored `START.EXE` is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The original five-byte `GAME.OVR` far call at file
`0x2EB01..0x2EB06` has SHA-256
`a734c1740b95619bedad6325a3ac8d1413024a9cbb9302bbf3c15831cfb20cfe`
and targets `0464:2632`. Its loaded `2D14:+020A` copy has SHA-256
`0d18bd5c35579c163826a0665cd04cf9a8b7b054c3f930e7633f0c6bdc4352f4`
and targets `0C88:2632`.

Resident `+2632..+2646` is 20 bytes at `START.EXE` file
`0x6CAB..0x6CBF`, with identical original/loaded SHA-256
`0987655ef499dd6c7ba5d5c9c63bf124c9e1715647b0e7949c883e5a22ff9f30`.
The next five bytes at file `0x6CBF..0x6CC4` are a far call to
runtime `0AF8:0A3F`, relocated to `131C:0A3F` in RAM. Their original
and loaded SHA-256 values are
`29a16310ee045858c5b1fecbb30ec4e438ced3f943e627fc7e261ff319c7965c`
and
`5edd05f1f8251f93185453fdb7f6901fa80650c85244e9e985109823bd18cfbd`,
respectively. That runtime call is separately modeled for earlier
`+2632` paths; this fourth-prefix model stops before it.

The +020A stack words from top to bottom are object `3DE4:0008`,
source `14A0:260A`, mask `0x0006`, 42 rows from row 62, 11
four-byte elements per row, and x index 2. The object header reports
height 168 and width 21. The far call pushes overlay CS and return IP
`+020F`. The resident then saves BP, allocates 36 stack bytes, loads
the source far pointer, and pushes five words for runtime `Move`.

Original-byte replay reaches resident entry on step 2 and the
pre-`Move` boundary on step 13. The ordered CPU tuple
`CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS` at `+2646` is

`0C88 2646 0008 3E4E 0000 14A0 3E68 3E78 3E80 3E52 14A0 14A0 1D16 7216`.

The 640 KiB entry and pre-`Move` RAM hashes are
`7cf5907411261888e2bb6c9d1474adea7853de50b3cd4b64be5f98c60b51e8c3`
and
`d6907db0096edac3304be6e836b5984dba4d9889842dd3a26fe0df35c44edda2`.
Only 11 bytes differ between the +020A and +2646 images, all in the
resident stack. No object or EGA plane byte changes in this prefix.

The C prefix matches the provisional original-byte +2646 RAM across
all 640 KiB with zero differences and all 14 CPU fields exactly.
Synthetic tests cover return-frame placement, stack argument order,
source-pointer variation, invalid host input, and unsupported CPU input
with no mutation. The public hash-locked
`tools/replay/resident_464_fourth_prefix_unicorn.py` accepts optional
direct native +2632/+2646 images when pD supplies them. Until a direct
same-run +020A stop is available, fourth-call reachability and the
fourth resident continuation retain an explicit provisional label.
