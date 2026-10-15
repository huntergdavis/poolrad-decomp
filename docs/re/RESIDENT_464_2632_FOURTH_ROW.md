# Fourth saved-view RES_0464:+26FB positive-row gate

## Scope and reuse

This bounded C model starts at resident 0C88:+26FB, takes the saved JGE
to +2722, performs the positive-row calculations, and stops at +275B
before the next JL. Negative-row and later branch outcomes are outside
this model. It neither models the fourth resident return nor the
parent overlay successor.

Before implementation, deja was run with query: Poolrad fourth
RES_0464 2632 26FB JGE successor 2722 run014. It returned no matches
(searched zero indexed sessions). Reused the reviewed +2699 width gate
(RESIDENT_464_2632_FOURTH_WIDTH.md), including its hash-locked +26FB
input image and CPU tuple. pD was asked for direct same-run fourth-call
+26FB/+2722/+275B captures; they remain pending here.

## Original-byte and C evidence

The ignored START.EXE SHA-256 is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
File 0x6D74..0x6DD4 maps to loaded 0C88:+26FB..+275B.
Both 96-byte spans have SHA-256
12057cc425aea721fbfd8c2c9c31782f4ba8e38d4a6dd42d17535da940ff9148.

The saved row argument at SS:BP+0x14 is 62. The +26FB JGE targets
+2722. That arm stores row 62 at BP-0x1E, multiplies it by object
width 21 and stores product 1302 at BP-0x16, then stores total rows
minus one, 41, at BP-0x14. It adds row 62 and total 42 to form 104,
loads object height 168, and compares zero DX and BX immediately before
+275B JL. Only three bytes of the 640 KiB image change in the span;
there is no object or EGA plane write.

Original-byte replay reaches +2722/+2728/+2732 at steps 2/4/7 with
RAM SHA-256
a96589cea30f44509fe4d9f26c7739ce56c6a637f273a1e75b44b93fc9705509.
At +2735 step 8, RAM SHA-256 is
2db1fccb156fb479998d18ce33b3ad3604e58d0b9a319a69829de5b49dab130e.
At +273C/+2751/+275B, steps 11/21/25, RAM SHA-256 is
95c91ee38f22f5d3808a851ddfdb055aa2733b70bfd7bf3ba897ed8bed7f621c.
The ordered +275B CPU tuple CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS is:

    0C88 275B 00A8 0000 0068 0000 2612 0008 3E80 3E5C 14A0 3DE4 1D16 7246

The public tools/replay/resident_464_fourth_row_unicorn.py locks source
size/hash, file and loaded spans, input image hash, all seven stops,
and their CPU tuples. C matches original bytes at each of those seven
stops with zero differences across all 640 KiB RAM and exact equality
of all 14 CPU fields. Synthetic tests cover the saved path, changed
row/total/height, and unsupported flags, negative row, zero total,
and multiply overflow with no mutation. Private stage images remain
under ignored resources.

The separate fourth-return full-RAM comparison remains open because
18 native-only bytes are consistent with an IRQ frame and BDA tick
change. No mask or full-function/count claim is applied here.
