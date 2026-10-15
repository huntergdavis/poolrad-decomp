# Fourth saved-view RES_0464 +29F3 stack and JE gate

## Scope and prior evidence

This bounded C gate starts at loaded `0C88:+29F3`, after the
separately reviewed indexed word writer, and executes through the
first following control instruction, `+2A02 JE`. It stops at the
successor `+2A04` or `+2A07` without executing that instruction.
This is neither the complete resident routine nor a direct native
fourth-call result.

Before implementation, `deja "Poolrad fourth RES_0464 2632 29F3
after 29E6 writer next control boundary return saved view"` ran
with a 35-second bound and timed out. The pinned +29E6 output and
CPU were reused from
[RESIDENT_464_2632_FOURTH_29E6.md](RESIDENT_464_2632_FOURTH_29E6.md).

## Original-byte route and address evidence

The ignored START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The 17-byte file `0x706C..0x707D` maps to loaded
`0C88:+29F3..+2A04`; file and loaded hashes are
`95935cd5fdb612c9643c381238bba60ca271dd025fc7f0d051dde9a75f16b109`.
Starting 640 KiB RAM SHA-256 is
`f0923e1fc6d1b28ea0da06c152b7b2a1cacf7e13da0bcd873be18ab862e8600f`.

Six instructions produce seven checkpoints. `+29F3` loads the
stack word at `[BP-0x10]`; `+29F6` adds it into `[BP-0x16]`;
`+29F9` increments `[BP-0x1E]`; `+29FC` loads `[BP-0x18]`;
`+29FF` compares that AX against the unaligned word at `[BP-0x23]`;
and `+2A02 JE` selects the successor. The pinned route falls
through to `+2A04`. Only physical addresses `0x20FC2` and
`0x20FCA` differ in the 640 KiB RAM boundary image, both within
the stack words. Boundary RAM SHA-256 is
`fad5029ace030b75902a05c478247f3462e24a5d1968abc062ff8a5eb4af5aff`.

Ordered CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`:

    entry    0C88 29F3 148B 3E4E 0000 14A0 3E68 007C 3E80 3E5C 14A0 3DE4 1D16 7202
    boundary 0C88 2A04 0000 3E4E 0000 14A0 3E68 007C 3E80 3E5C 14A0 3DE4 1D16 7297

The C gate matches original-byte replay at all seven checkpoints
across all 640 KiB RAM and all 14 CPU fields. Original-byte
synthetic inputs separately reach the equal `+2A07` successor,
the unequal `+2A04` successor, wrapping addition/index, and
index sign overflow. Focused C tests assert their stack words,
CPU flags, and successor addresses; invalid and unsupported inputs
leave state unchanged. The public
`tools/replay/resident_464_fourth_29f3_unicorn.py` checks the ignored
image and preimage hashes and optionally emits private checkpoints
under ignored `resources/`.

This parity uses the same pinned predecessor chain through +29E6.
pD's direct run015 fourth-call native captures are a different
starting RAM state from the earlier run014-derived chain; their
CPU/RAM hashes cannot be paired across runs as parity evidence.
Direct native capture of this fourth-call suffix, later control
flow, and the IRQ-like return residue remain unresolved.
