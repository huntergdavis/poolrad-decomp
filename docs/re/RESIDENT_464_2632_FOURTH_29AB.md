# Fourth saved-view resident +29AB increment arm

## Boundary and prior work

This C gate covers `0C88:+29AB..+29C6` after `+29D5 JNE`
loops back from the first four-byte runtime Move. It increments
`DS:70C7`, prepares the next four-byte Move, and stops at the
`+29C6` call entry. It does not execute that second call, cover
the `+29D7` exit arm, or claim a complete resident routine.

Before decoding, `deja "Poolrad fourth RES_0464 2632 29AB 70C7
increment alternate arm 29D7"` ran with a 35-second bound and timed
out. Reused the reviewed +29A6 call-entry preimage from
[RESIDENT_464_2632_FOURTH_29A6.md](RESIDENT_464_2632_FOURTH_29A6.md)
and the independently reviewed relocated Move contract described in
[RESIDENT_464_2632_FOURTH_MOVE.md](RESIDENT_464_2632_FOURTH_MOVE.md).

## Original-byte path and effects

The ignored START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
File `0x703F..0x7050` maps to loaded
`0C88:+29C6..+29D7`; the file SHA-256 is
`cc1a43a44a97b085fe511ea515d4465c9590d65643add6a2555cde89fb5f78bc`
and the relocated loaded SHA-256 is
`6146e8864328699c80452dda85e44aa7a62c81f7dd40d0ed51e811e96832af76`.
Starting from the hash-pinned +29C6 preimage, original-byte replay
executes 20 Move/caller instructions to `+29AB`: `+29CB`
advances local offset `[BP-0x16]` by four, `+29CF` reads
`DS:70C7`, `+29D2` compares it to local `[BP-0x24]`, and
`+29D5 JNE` is taken.

The 27-byte file `0x7024..0x703F` and loaded
`0C88:+29AB..+29C6` spans both hash to
`8c40857679a81d2a497eacbe20ca4b59227a57617bf307fa7827876941efa4c7`.
The arm executes 12 instructions with 13 checkpoints. Its `INC`
preserves carry and increments `DS:70C7`; the call setup then
pushes the stack-local source pointer, computed far destination,
and four-byte count. On this derived preimage, only physical
addresses `0x1BAC7` and `0x20FB4` differ at the boundary.

Ordered CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`:

    input    0C88 29AB 0001 3E4E 0000 14A0 3E68 1483 3E80 3E5C 14A0 3DE4 1D16 7297
    boundary 0C88 29C6 0004 3E4E 0000 14A0 3E68 1483 3E80 3E52 14A0 3DE4 1D16 7212

Input RAM SHA-256 is
`4c0f78a0462057168f8b7b4edd86d66af4da0f8dcf7d51c5a8079ecb4a0adde2`;
boundary RAM SHA-256 is
`ee20ffd4610472c91661b7948fc6e3cbff2c876674f3556d2f2cf13040ff2600`.
The bounded C model matches all 640 KiB RAM and all 14 CPU fields
at all 13 original-byte checkpoints. Synthetic tests cover ordinary
and wrapping increment flags, five pushed words, and guarded entry.

The public replay tool
`tools/replay/resident_464_fourth_29ab_unicorn.py` accepts the
ignored `fourth-29a6-stops/ram-13.bin` parent image and derives
the `+29AB` preimage before recording this arm. Optional snapshots
stay beneath ignored `resources/`; run with
`PYTHONPATH=/tmp/poolrad-unicorn`.

This is original-byte replay parity from a provisional fourth-call
preimage. Direct same-run native fourth-call capture and the
18-byte IRQ-like return residue remain unresolved. The separate
run015 first-call native state is not cross-paired with this route.
