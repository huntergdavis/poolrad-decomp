# Fourth saved-view RES_0464 +2A04 jump and +297F JBE gate

## Scope and prior evidence

This bounded C gate starts at loaded `0C88:+2A04`, follows its
unconditional jump to `+2962`, and executes through `+297F JBE`.
It stops at the selected `+2981` or `+2999` successor before
executing either. It does not complete the resident routine or
prove a direct native fourth-call result.

Before implementation, `deja "Poolrad fourth RES_0464 2632 2A04
2A07 successor JNE return saved art gate"` ran with a 35-second
bound and timed out. The pinned preimage and CPU were reused from
[RESIDENT_464_2632_FOURTH_29F3.md](RESIDENT_464_2632_FOURTH_29F3.md).

## Original-byte route and address evidence

The ignored START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
Two noncontiguous file spans are involved:

| File span | Loaded span | SHA-256 in file and loaded RAM |
| --- | --- | --- |
| `0x707D..0x7080` | `0C88:+2A04..+2A07` | `2206490c93d265f0bda4f2a27f5a35a5ff81799af78d4dc18c59c2d526bc7c96` |
| `0x6FDB..0x6FFA` | `0C88:+2962..+2981` | `d4b5392bea1f7a3d74e5abc1d7ab5765d12c6277292e350ae8c65d5f779b2e7f` |

Starting 640 KiB RAM SHA-256 is
`fad5029ace030b75902a05c478247f3462e24a5d1968abc062ff8a5eb4af5aff`.
Eleven instructions produce twelve checkpoints. The `+2A04 JMP`
lands at `+2962`, which increments stack `[BP-0x18]`.
`+2965` loads index `[BP-0x1E]`; `+2968` stores byte one at
`DS:[DI+0x4A28]`; `+296D` loads stack `[BP-0x0E]`;
`+2970` adds it into `[BP-0x16]`; `+2973` reloads the index;
`+2976` doubles DI; `+2978` loads `DS:[DI+0x4C20]`;
`+297C` compares that word with `[BP-0x16]`; and `+297F JBE`
selects the next path. The pinned route falls through to
`+2981`. The three changed RAM addresses are `0x19467`,
`0x20FC8`, and `0x20FCA`; boundary RAM SHA-256 is
`c78f443d72bc1550ed523b2549b6974e973585edb2d27b117477ee0030b4acf6`.

Ordered CPU tuple `CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS`:

    entry    0C88 2A04 0000 3E4E 0000 14A0 3E68 007C 3E80 3E5C 14A0 3DE4 1D16 7297
    boundary 0C88 2981 FFFF 3E4E 0000 14A0 3E68 007E 3E80 3E5C 14A0 3DE4 1D16 7286

The C gate matches original-byte replay at all twelve checkpoints
across all 640 KiB RAM and all 14 CPU fields. Synthetic original-byte
inputs separately reach `+2981` and `+2999` through unsigned
greater, equal, and below comparisons. They also exercise counter
and accumulator wrap and a high-bit index; C tests assert affected
memory, intermediate flags, and final CPU. Invalid and unsupported
inputs leave state unchanged. The public
`tools/replay/resident_464_fourth_2a04_unicorn.py` checks both
ignored spans and the preimage hash and optionally writes private
snapshots beneath ignored `resources/`.

This route continues the pinned predecessor chain through +29F3.
pD owns direct run015 native fourth-call capture; its starting RAM
differs from the earlier run014-derived chain. Cross-run RAM hashes
and CPU values are not parity evidence. Later control flow and the
IRQ-like return residue remain unresolved.
