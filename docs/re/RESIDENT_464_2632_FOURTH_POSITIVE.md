# Fourth saved-view RES_0464:+2632 positive-index gate

## Scope and prior work

The bounded C model begins at loaded resident `0C88:+2656`, evaluates
the `+2656` CMP and `+265A` JGE, follows the saved nonnegative-index
arm at `+266E`, and stops at `+2699` **before** the next JL. It does
not model the negative-index arm or any successor after +2699. The
input is original-byte replay from pD's directly captured same-run
fourth-call `2D14:+020A` prestate. Direct intermediate +265A/+266E/+2699
RAM/CPU captures have been requested from pD and are not claimed here.

Before implementation, `deja "Poolrad fourth RES_0464 2632 2656 CMP
JGE successor 020F run014"` timed out after 35 seconds without
results. Reused the reviewed
[fourth Move/JNE gate](RESIDENT_464_2632_FOURTH_MOVE.md) and the
[fourth call prefix](RESIDENT_464_2632_FOURTH_PREFIX.md); no existing
source or native trace file was modified for this new model.

## Original-byte and C evidence

The ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Its file `0x6CCF..0x6D12` maps to loaded
`0C88:+2656..+2699`. Both 67-byte spans have SHA-256
`0a0bee66e226ed987b134eb5e457a3310a2a190c1f009673cd2823fc2771c00d`.

Original-byte execution from +2656 reaches pre-JGE +265A at step 2,
takes the JGE to +266E at step 3, reaches shared +2679 at step 7,
and reaches pre-JL +2699 at step 21. The saved x argument at
`SS:BP+0x16` is 2, so JGE is taken. Count at `BP+0x12` is 11.
The saved object pointer is `3DE4:0008`; its width word is 21.
The shared arithmetic leaves low index plus count at 13, reads that
width, and leaves DX and BX zero immediately before JL.

The ordered pre-JL CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`
is

`0C88 2699 0015 0000 000D 0000 2612 0008 3E80 3E5C 14A0 3DE4 1D16 7246`.

At +265A/+266E the full 640 KiB RAM SHA-256 remains
`8d65f2916cda57e7e5c6a4d71a70404e716b2e275c5489c132b48f3ed1371200`.
At +2679/+2699 it is
`9cf87e06c28a8293f625239b02ebb82244ad9cf9f38453394fbf91510d3d5438`.
Only one byte at stack local `SS:3E72` changes relative to +2656.
No object or EGA plane byte changes in this slice.

The C result matches original-byte +2699 across the full 640 KiB RAM
with zero differences and all 14 CPU fields exactly. Synthetic tests
cover the positive branch, local writes, object-width read, changed
x/count/width values, and unsupported negative or oversized inputs
without mutation. The public hash-locked
`tools/replay/resident_464_fourth_positive_unicorn.py` accepts direct
native intermediate images when available. This remains a bounded
path model, not complete-function or count credit.

## Fourth-return reconciliation

pD has directly captured the fourth resident return at overlay
`2D14:+020F` with RAM SHA-256
`f461d6c4cc6972c5fdf38c12cdde62e0624fedc6cb97bd8f1e7e690e0ea48b15`.
Independent original-byte replay from the direct +020A prestate reaches
the same +020F CPU tuple after 16,758 instructions, but its RAM SHA-256
is
`36c542ecf3352e92116d14d286f79b05ac90fe24d094210c1d8b2e05f2092571`.
Eighteen RAM bytes differ: physical `0x0046C` and 17 stack-local
bytes at `SS:3E3C..3E44` and `SS:3E46..3E4D`. There are no
object or EGA plane mismatches. The cause is unresolved, so no full
fourth-call RAM parity or parent continuation is claimed here.
