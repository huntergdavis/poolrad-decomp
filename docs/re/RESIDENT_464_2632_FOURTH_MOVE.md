# Fourth saved-view RES_0464:+2632 Move and JNE gate

## Boundary and provenance

This model starts at loaded resident `0C88:+2646`, executes the far
runtime `Move` call, then follows `+264B` through the taken `+2651`
JNE to `+2656`. It **stops before** the `+2656` CMP and `+265A` JGE.
The zero-object path through `+2653` and later fourth-call return are
outside this model. The starting +2646 image is original-byte replay
from pD's directly captured fourth-call prestate at +020A. Its full
640 KiB RAM and all 14 CPU fields match the earlier +020A replay.
pD has not supplied a direct +2646 or later fourth-call capture. This
is a bounded provisional path, not a complete resident routine or count
claim.

Before decoding, `deja "Poolrad U3BD fourth RES_0464 2632 2646 runtime
Move return 264B first conditional"` timed out after 35 seconds without
results. Reused the already reviewed forward `Move` contract from
[the run014 resident composer](RESIDENT_464_2632_FIRST_CALL.md) and the
hash-locked [fourth prefix](RESIDENT_464_2632_FOURTH_PREFIX.md). The
new C source keeps that runtime behavior local to this disjoint gate;
it does not change the existing composer.

## Byte and control evidence

The ignored `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Original resident file `0x6CBF..0x6CCF` maps to loaded
`0C88:+2646..+2656`. The 16-byte original and loaded SHA-256 values
are, respectively,
`439a66c1f0c5069102fa75d200a37e0e164a48d7cb5fe78eb3a76e0830663b0d`
and
`ba5fdcc57f25ecaf397c60a01727dd956431dc6306dc5a367584fd65c0164ba1`.
The first five bytes are the relocated runtime far call to
`131C:0A3F`; the loaded 24-byte runtime body `+0A3F..+0A57` has
SHA-256
`5bf8af52bdcd4af1b7e73b160b3ee02e6de7812b7e686aae02f73d9ccbc061aa`.
It loads source/destination far pointers and count from the stack,
copies forward, restores DS, and returns with the argument words removed.

Original-byte replay from +2646 reaches runtime entry at step 2,
resident return `+264B` at step 19, `+2651` JNE at step 21, and the
taken `+2656` successor at step 22. The object far pointer is
`3DE4:0008`; OR of its words is `0x3DEC`, so zero flag clears and
JNE is taken. The next `+2656` CMP/JGE has not been modeled here.

The ordered +2656 CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`
is

`0C88 2656 3DEC 3E4E 0000 14A0 2612 3E80 3E80 3E5C 14A0 1D16 1D16 7202`.

The 640 KiB original-byte RAM SHA-256 at all four observed stops is
`8d65f2916cda57e7e5c6a4d71a70404e716b2e275c5489c132b48f3ed1371200`.
Relative to +2646, only two bytes at stack `SS:3E4E..3E4F` change.
The eight-byte `Move` executes, but this saved source/destination
pair already matches. No object or EGA plane byte changes in this gate.

The bounded C result matches original-byte +2656 across all 640 KiB
with **zero differing bytes** and all 14 CPU fields exactly. Synthetic
tests also change the copied source bytes and verify destination
effects, return-frame residue, observer stages, and rejection of
unsupported CPU or zero-object paths without mutation. The public
hash-locked
`tools/replay/resident_464_fourth_move_unicorn.py` accepts optional
direct native +2646/+2656 images. Until pD's direct same-run postcall
capture is available, this remains original-byte postcall parity. Parent caller
work after the fourth resident return is deferred to a separate branch.
