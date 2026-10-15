# Fourth saved-view RES_0464:+277D selector gate

## Scope and prior work

This bounded model executes +277D..+278F and stops before +2792
when selector byte is 0 or 2, or before +28D9 otherwise. It does not
execute either successor or complete the fourth resident call.

Before this implementation, the bounded deja query "Poolrad fourth
RES_0464 2632 277D 278B 278F saved run014 JAE successor" timed out
after 35 seconds. Reused the reviewed +275D JG/JAE stop and its
hash-locked +277D CPU/RAM prestate from
[RESIDENT_464_2632_FOURTH_JG.md](RESIDENT_464_2632_FOURTH_JG.md).

## Instruction and saved path evidence

The ignored START.EXE SHA-256 is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
File 0x6DF6..0x6E0B maps to loaded 0C88:+277D..+2792.
Both 21-byte spans have SHA-256
aa092082caad21f29e64249a32a4a188062c054f06ff4b5a39fa54b422bf0a40.

+277D loads the stack word at BP+0x10 into AX; +2780 decrements AX,
preserving CF and updating the other arithmetic flags; +2781 writes
the result at BP-0x14. +2784 loads DS:4A1E into AL. Comparisons
at +2787 and +278B select +2792 for byte 0 or 2. Other values jump
from +278F to +28D9. The C model preserves full arithmetic flags
and all non-arithmetic flags at both comparisons.

The saved source word is 42, the already stored destination word is
41, and the selector byte is 1. Consequently the saved path takes
neither equal branch and reaches +28D9. Original-byte replay reaches
+2780/+2781/+2784/+2787/+2789/+278B/+278D/+278F/+28D9 at steps
2..10. Each stop retains the full 640 KiB RAM SHA-256
95c91ee38f22f5d3808a851ddfdb055aa2733b70bfd7bf3ba897ed8bed7f621c
with zero changed bytes. The ordered +28D9 CPU tuple
CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS is:

    0C88 28D9 0001 0000 0068 0000 2612 0008 3E80 3E5C 14A0 3DE4 1D16 7297

The public tools/replay/resident_464_fourth_select_unicorn.py locks
START.EXE size/hash, file and loaded spans, input RAM hash, all nine
saved stops, and their CPU tuples. The focused C replay checks full
640 KiB RAM and all 14 CPU fields at every stop. Synthetic tests
exercise selector 0, 1, 2, and 3, destination-word changes, decrement
wraparound, invalid host arguments, and unsupported CPU with no effects.
The replay's `--branch-matrix` option independently verifies the
four selector outcomes against the original instructions and full RAM:
0/2 stop at +2792 with FLAGS 0x7246; 1/3 stop at +28D9 with FLAGS
0x7297/0x7202 respectively.

pD's direct same-run native intermediate capture remains pending.
The separate fourth-return 18-byte IRQ-like residue still prevents
a full-RAM return or complete-function/count claim.
