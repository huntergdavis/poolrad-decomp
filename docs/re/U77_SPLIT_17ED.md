# Unit 0x77 +0x17ED: provisional sidecar split

`src/recovered/overlay_u77_split_17ed.c` models the complete decoded body,
including RAM, CPU, stack, and `retf 4` state under bounded external-call
effects. This is **provisional**: no native entry frame was reached, and the
heap, string, and resident UI calls have not been replayed jointly with this
caller.

## Source and route

The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The candidate is file range `[141351,141527)`, unit `0x77` offsets
`+0x17ED..+0x189C`: exactly **176 bytes / 63 instructions**, SHA-256
`46c07cbe1120a95c2830f87199a1f5141a272b9dfba7df3f534e4168079e82e9`.
It ends with `retf 4` at `+0x189A`; all branch targets are aligned and inside
the body. The aligned near call at `+0x1423` in public entry `+0x0EF8`
reaches it on the `H` branch after a far pointer and `CS` push. The adjacent
`J` branch calls `+0x189D` at `+0x1433`.

There are four outgoing far calls: `AF8:023F` allocation at `+0x1820`,
`AF8:1685` move at `+0x1839`, `AF8:0B0B` string copy at `+0x188D`, and
resident `BA:1693` UI report at `+0x1892`. The model exposes the original
stack arguments and lets a caller adapter supply their effects. The
allocation and move call shape was cross-checked against
[RESIDENT_SIDECAR_APPEND.md](RESIDENT_SIDECAR_APPEND.md); the move's
overlap-safe behavior was reused from [U9E_0160.md](U9E_0160.md).
A bounded `deja` query found no prior-session result for this candidate.

## Decoded behavior

The far argument points to a 63-byte sidecar node. The routine reads its
count at `+0x39` and takes the unsigned floor of half. When the half is zero
(counts zero or one), it copies the private `CS:17DC` string into a local
stack buffer, invokes the resident UI report, and returns without changing
the node. The bytes at `CS:17DC` remain in ignored original media.

For larger counts it allocates `0x3F` bytes, copies the complete source node,
sets the new node's count to the half and byte `+0x34` to zero, and sets its
next pointer at `+0x2A/+0x2C` to the source's previous next pointer. It
sets the source count to the remainder and its next pointer to the new node.
It preserves source-order writes, segment:offset wrap, scratch stack bytes,
returned registers and flags, and far-return cleanup.

## Verification and open gate

`test_overlay_u77_split_17ed.c` checks argument guards, split/relink, and
count-one report behavior. `check_overlay_u77_split_17ed.py` verifies the
original file and body hashes, instruction and call boundaries, and executes
13 cases against the original bytes under Unicorn and the C model. Cases
cover counts 0, 1, 2, 3, 4, 5, 127, 128, 254, and 255, offset wrap, a
one-word-zero next pointer, and a zero-segment allocation. The audit compares
the **entire 1 MiB RAM image**, all returned registers and flags, and the
external-call argument sequence.

The allocation adapter supplies a fixed far pointer; the move adapter copies
the 63 bytes; string and UI adapters have no effects. Those same effects
are applied to both executions. A bounded scan of route notes and 17 local
RAM captures found no native `+0x17ED` entry frame. This PR requests no
strict full-C credit, verified identity, or count claim.
