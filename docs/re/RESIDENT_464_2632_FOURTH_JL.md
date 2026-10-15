# Fourth saved-view RES_0464:+275B JL decision

## Scope and prior work

This isolated C model evaluates the two-byte JL at resident
0C88:+275B, then stops before the selected successor: +275D on
fallthrough or +2763 when taken. It does not execute either successor
or complete the fourth resident call.

Before implementation, deja was run with query: Poolrad fourth
RES_0464 2632 275B JL 275D run014 native C parity. The bounded
35-second query timed out without results. Reused the reviewed
+26FB positive-row model (RESIDENT_464_2632_FOURTH_ROW.md), including
its hash-locked +275B RAM and exact CPU tuple.

## Instruction and path evidence

The ignored START.EXE SHA-256 is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
File 0x6DD4..0x6DD6 maps to loaded 0C88:+275B..+275D.
Both two-byte spans have SHA-256
4cd53691e3aeeb340b82a72a9a6a63018ab03ed3c8bd0a6effc95a6a1d4a7d20.

JL selects +2763 when sign and overflow flags differ; otherwise it
falls through to +275D. It changes neither flags nor RAM. The saved
+275B flags are 0x7246, so sign and overflow are both clear.
Original-byte execution reaches +275D after one JL instruction and
two hooked stops, with zero changed bytes of the 640 KiB RAM image.
The pre/post RAM SHA-256 is
95c91ee38f22f5d3808a851ddfdb055aa2733b70bfd7bf3ba897ed8bed7f621c.
The ordered +275D CPU tuple CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS is:

    0C88 275D 00A8 0000 0068 0000 2612 0008 3E80 3E5C 14A0 3DE4 1D16 7246

The public tools/replay/resident_464_fourth_jl_unicorn.py verifies
source size/hash, file and loaded spans, the input RAM hash, saved
fallthrough and all 14 CPU values. Synthetic C tests cover the saved
fallthrough, a taken JL, both sign and overflow set, invalid host
buffers, and unsupported caller CPU without RAM mutation. The private
C comparison uses the original +275B image and checks zero full-RAM
differences and all 14 CPU fields after fallthrough.

pD was asked for direct same-run native +275B/+275D captures. No
native intermediate parity is claimed until those images are checked.
The separate fourth-return 18-byte IRQ-like residue still prevents a
full-RAM return or complete-function claim; no count credit is taken.
