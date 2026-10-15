# Fourth saved-view RES_0464:+275D JG/CMP/JAE gate

## Scope and prior work

This bounded C model evaluates resident +275D JG, +275F CMP AX,CX,
and +2761 JAE. It stops before selected successor +277D or +2763.
It does not execute either successor or complete the fourth resident
call.

Before implementation, deja was run with query: Poolrad fourth
RES_0464 2632 275D JG 275F CMP JAE 277D run014. The bounded
35-second query timed out without results. Reused the reviewed +275B
JL decision (RESIDENT_464_2632_FOURTH_JL.md) and its hash-locked
saved +275D CPU/RAM prestate.

## Instruction and path evidence

The ignored START.EXE SHA-256 is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
File 0x6DD6..0x6DDC maps to loaded 0C88:+275D..+2763.
Both six-byte spans have SHA-256
515dae830436fbd579a67a53f28d36e7ff28debe0e37660a7c4a13cb5237961a.

JG takes +277D when ZF is clear and SF equals OF. Otherwise CMP
subtracts CX from AX, setting the full 16-bit arithmetic flags;
JAE then takes +277D when CF is clear or falls through +2763.
All three instructions are read-only for RAM. Saved flags at +275D
are 0x7246, so ZF makes JG fall through to +275F. Saved AX=168 and
CX=104, so CMP leaves flags 0x7202 and JAE takes +277D.

Original-byte replay reaches +275F/+2761/+277D at steps 2/3/4.
All stops retain the full 640 KiB RAM SHA-256
95c91ee38f22f5d3808a851ddfdb055aa2733b70bfd7bf3ba897ed8bed7f621c
with zero changed bytes. The ordered +277D CPU tuple
CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS is:

    0C88 277D 00A8 0000 0068 0000 2612 0008 3E80 3E5C 14A0 3DE4 1D16 7202

The public tools/replay/resident_464_fourth_jg_unicorn.py locks
START.EXE size/hash, file and loaded spans, input RAM hash, all
three saved stops, and CPU tuples. C matches original bytes at each
stop with zero differences across all 640 KiB RAM and exact all-14
CPU equality. Synthetic tests cover direct JG taken, JG fallthrough
with JAE taken, JG fallthrough with JAE not taken, equality, invalid
host arguments, and unsupported caller CPU without RAM mutation.

pD was asked for direct same-run native +275D/+277D captures. No
native intermediate parity is claimed until checked. The separate
fourth-return 18-byte IRQ-like residue still prevents a full-RAM
return or complete-function/count claim.
