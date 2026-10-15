# Fourth saved-view RES_0464:+2699 width gate

## Scope and reuse

This C model starts at loaded resident 0C88:+2699, follows the saved
JL and JG fallthroughs and the JAE branch to +26CD, calculates the
width remainder, and stops at +26FB before the next JGE. It handles
only that branch shape. The parent overlay continuation and negative
row/index paths remain outside this model.

Before implementation, deja was run with query: Poolrad fourth RES_0464
2632 2699 JL successor 269B 020F run014. It was stopped after a bounded
wait without a result. Reused the reviewed +2656 positive-index model
(RESIDENT_464_2632_FOURTH_POSITIVE.md), its hash-locked +2699 starting
image, and the preceding fourth-call CPU contract. Direct same-run native
+2699/+269B captures were requested from pD; none are claimed here.

## Original-byte path and effects

The ignored START.EXE SHA-256 is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
File 0x6D12..0x6D74 maps to loaded 0C88:+2699..+26FB.
Both 98-byte spans hash to
90090ebf66c6cf2c96c48cdf82c96223ea73c3c8eebf07600cea03f8dd83ecd8.

From the saved +2699 prestate, DX=BX=0 and flags=0x7246.
The +2699 JL falls through to +269B; JG falls through to +269D;
CMP AX=21 with CX=13 sets flags=0x7212; JAE at +269F takes
+26CD. That arm writes zero at SS:BP-0x0C, recomputes x+count=13,
subtracts it from object width 21, and writes remainder 8 to
SS:BP-0x10. CMP of row 62 with zero at +26F7 leaves flags=0x7202
at pre-JGE +26FB. The full 640 KiB RAM changes by one byte from
+2699 through +26FB, with no object or EGA plane changes.

Original-byte replay reaches +269B/+269D/+269F/+26CD at steps
2/3/4/5 with input RAM SHA-256
9cf87e06c28a8293f625239b02ebb82244ad9cf9f38453394fbf91510d3d5438.
It reaches +26F7/+26FB at steps 23/24 with RAM SHA-256
a96589cea30f44509fe4d9f26c7739ce56c6a637f273a1e75b44b93fc9705509.
The ordered +26FB CPU tuple CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS is:

    0C88 26FB 0008 0000 000D 0000 2612 0008 3E80 3E5C 14A0 3DE4 1D16 7202

The public tools/replay/resident_464_fourth_width_unicorn.py locks source
size/hash, file span, loaded span, input image hash, all six stops,
and their CPU tuples. The private six-stage C comparison matches
original bytes at +269B, +269D, +269F, +26CD, +26F7, and +26FB:
zero differences across all 640 KiB RAM and all 14 CPU fields at
each stop. Synthetic tests cover the saved branch, exact-fit width, unsupported
nonzero flags, width below sum, and negative row with no mutation.
The private full-state comparison uses the focused test's two image
arguments. No complete-function or count credit is claimed.

The separate fourth-return comparison still has 18 native-only
RAM changes against original-byte replay. They are physical 0x0046C
(BDA 0040:006C tick low byte C9 to CA) and 17 stack-local bytes in
SS:3E3C..3E4D. The native stack words at SS:3E48..3E4D are
1689 131C 7202 (return into runtime Move), and at SS:3E3C..3E41
are FEAF F000 7202 (inner BIOS handler frame). IVT INT08 points to
F000:FEA5. This is strong IRQ0/INT1C residue evidence, not an
instruction-proven interrupt trace. No mask is applied: full-RAM
return parity and parent +020F continuation remain open.
