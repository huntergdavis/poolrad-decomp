# Unit `0x6F:+0x1446` SS-frame alias audit

The merged [20-bit party-insertion model](U6F_PARTY_INSERT_1446_DOS.md)
keeps the Pascal frame and eight membership bytes in C locals. A synthetic
two-node path shows a reachable memory-alias behavior that this API cannot
represent. This note and its bounded probe claim no strict full-C or identity
count. The counterexample is derived from original instruction order and
tested against the merged model; it is **not** a native measurement of this
synthetic case.

Before debugging, `deja "Poolrad u6F 1446 SS frame alias BP 3DCA DS 14A0
saved insertion"` found no exact match. The shorter `deja "u6f 1446"`
returned the prior PM party-build sessions. Reused the merged
[source audit](U6F_PARTY_INSERT_1446.md), the [DOS-memory model](U6F_PARTY_INSERT_1446_DOS.md),
and the [saved native call/return capture](SLOT_A_NATIVE_19E3_INSERTION_RETURN.md).
The GAME body was independently re-hashed and decoded with Capstone 5 before
this probe.

| Original span | Size / decode | SHA-256 |
| --- | --- | --- |
| GAME.OVR | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit `+0x1446..+0x157E`, file `0x1F42C..0x1F564` | 313 bytes; 99 instructions | `370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245` |
| Call-free selector `+0x151E..+0x1564`, file `0x1F504..0x1F54A` | 71 bytes; 21 instructions | `a864d95f14e675a55dae74f80e6db5721bdfa865ed5c03d601aeb7d10542750f` |

## Saved frame and pointer ranges

At the native stop **before** caller `+0x19E3`, `SS=0x1D16`, `SP=0x37B2`,
and `BP=0x3DCA`. That BP belongs to the caller. The near call pushes its
return word and callee `+0x1446 PUSH BP` pushes the caller BP, so callee
`+0x1447 MOV BP,SP` sets **BP=0x37AE**. `+0x1449 SUB SP,0x10` then makes
`SP=0x379E`. The saved stack top at caller `SS:37B2` contains the pushed CS
word, followed by the incoming far argument at callee `[BP+6/+8]`.

| Reached field | Segment:offset | A20-off physical range |
| --- | --- | --- |
| Callee selected pointer `[BP-0x10/-0xE]` | `1D16:379E..37A1` | `0x208FE..0x20901` |
| Callee used bytes `[BP-0xC..-5]` | `1D16:37A2..37A9` | `0x20902..0x20909` |
| Callee cursor `[BP-4/-2]` | `1D16:37AA..37AD` | `0x2090A..0x2090D` |
| Incoming argument `[BP+6/+8]` | `1D16:37B4..37B7` | `0x20914..0x20917` |
| DS active far pointer | `14A0:5D92..5D95` | `0x1A792..0x1A795` |
| DS head far pointer | `14A0:5D96..5D99` | `0x1A796..0x1A799` |
| DS window far pointer | `14A0:49D6..49D9` | `0x193D6..0x193D9` |
| Saved window-1 counter | `3A16:067C..067D` | `0x3A7DC..0x3A7DD` |
| Saved incoming record | `449A:0008..0124` | `0x449A8..0x44AC4` |
| Saved incoming slot `+0xBF` | `449A:00C7` | `0x44A67` |

The captured saved call has a null DS head and disjoint local, record-slot,
head, active, and window fields. The caller's `ES:DI=1D16:3626` before
the call is physical `0x20786`; `ES:DI=449A:0008` after return points to
the record. Segment windows themselves overlap: `DS=14A0` maps
`0x14A00..0x249FF`, while `SS=1D16` maps `0x1D160..0x2D15F`.
The capture proves disjointness only for the listed **accessed fields** in
that one run. No allocator invariant, stack high-water bound, or complete
corpus trace proves all incoming, list-record, or window pointers avoid SS
locals.

## Synthetic two-node counterexample

Use `SS=1D16`, callee `BP=37AE`, `DS=14A0`, one existing head
`3000:0100` with slot 0 and a null `+0x104/+0x106` link, and incoming
`1D16:36E3` with a null link. Set DS head to the existing node, DS active
to null, and SS `[BP+6/+8]` to the incoming far pointer. Set both local
membership bytes `SS:37A2/37A3` to zero. The incoming slot at
`1D16:37A2` is **the same physical byte** as `used[0]` (`0x20902`);
`used[1]` is `0x20903`. These are synthetic addresses and values, not
game bytes.

| Source order | Physical effect on `0x20902` / `0x20903` |
| --- | --- |
| `+0x144F` | First incoming slot store makes `used[0]=0xFF`. |
| `+0x14B4/+0x14B9` | Existing node's null link becomes the incoming far pointer. |
| `+0x14CA` | FillChar clears the eight SS used bytes, also changing incoming slot to 0. |
| `+0x14FC/+0x1505` on existing slot 0 | Mark `used[0]=1`, also changing incoming slot to 1. |
| `+0x14FC/+0x1505` on incoming slot 1 | Mark `used[1]=1`; the next link is null. |
| `+0x1538` | Reset incoming slot to 0, also clearing `used[0]`. |
| `+0x1541/+0x1555` | Read slot 0 and now-zero `used[0]`; exit at `+0x1565`. |

The instruction-order outcome at `+0x1565` is incoming slot **0**, SS
`used[0]=0`, SS `used[1]=1`. Running the same synthetic input through the
merged C-local model leaves its C `used[0]=1` after marking the existing
node. Its incoming slot stays `0xFF` until the reset, so it then advances
to slot **1**; physical `0x20902=1`, `0x20903=0`. The focused test asserts
these final bytes and a disjoint control where both models select slot 1.

`pr_u6f_insert_ss_two_node_probe` models this two-node path from
`+0x144C` through the selector branch at `+0x1563` over one unified 20-bit
RAM image. It re-reads the stack argument and SS cursor/selected pointer
at their source sites. It uses the hash-audited FillChar eight-byte result
and membership's low-byte decision. It does not model the helpers' transient
stack writes, arbitrary list lengths, the `+0x1565` window increment, or
the final u9B call, and is intentionally a bounded probe.

To seek strict full-routine credit, the smallest next API change is to add
`SS` and callee `BP` (or entry `SP`) to the existing 1 MiB RAM view, read
the incoming pointer from `SS:[BP+6/+8]`, and keep all 16 local bytes in
that RAM. The full path then needs helper call/stack effects modeled or a
proof that their stack locations cannot alias game-visible pointers on the
supported inputs. The saved case alone does not supply that proof.
