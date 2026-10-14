# Saved first-view resident masked-byte loop successor

The saved slot-A first-view frame reaches `RES_0464:+0C6E` immediately
after the bounded [masked-byte store](SLOT_A_RES464_POSTFLUSH_MASKED_BYTE.md).
The next reached stop is `0C88:+0C3D`, the following masked-byte
iteration. A same-run native capture and bounded portable C step match
all 14 captured CPU fields and all 640 KiB of guest RAM at that stop.
Four RAM byte addresses change; no callback is reached in this interval.
This is a loop slice, not a full resident draw translation or a visible
EGA-frame claim.

Before decoding, `deja "Poolrad RES_0464 +0C6E postflush masked-byte loop
successor saved slot A"` was attempted but did not return within a bounded
wait. The prior [merged first-writer note](SLOT_A_RES464_POSTFLUSH_MASKED_BYTE.md)
was reused for the entry frame and independently checked against the fresh
ignored native run. The [cold-start key timing](NATIVE_COLD_START_CODEWORD_KEY_TIMING.md)
guided the private Return input at the translation-wheel prompt.

## Original code and reached path

Ignored `START.EXE` SHA-256:
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The unpacked image intervals map byte-for-byte to live segment `0C88`:

| Resident offsets | Unpacked image | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `+0C6E..+0C8C` | `[0x52AE,0x52CC)` | 30 | `f4cfc827c666bf9247381d9502d84832cbbb78b09db52dfd10e53f9b48213b8c` |
| `+0C31..+0C3D` | `[0x5271,0x527D)` | 12 | `24af6a6df972ee2c136ee6c4ff0621ad734cc715d4962da735ca68d951a46077` |

The two intervals contain 14 aligned instructions; **12 execute** on
the captured route. At `+0C6E`, the source index word at `[BP-8]`
increments. `+0C71` decrements the plane-count byte at `[BP-0x11]`;
`+0C78 JNE` reaches `+0C81`, which increments the destination data
index at `[BP-0x0A]`. `+0C84` reads `DS:[0x70C7]`;
`+0C87` compares it with `[BP-0x19]`, and `+0C8A JNE` reaches
`+0C31`. That arm increments `DS:[0x70C7]`, loads the updated
source index and source far pointer from `[BP+0x0A]`, then `+0C3B`
adds the index to `DI`. The next instruction is the already modeled
masked-byte read at `+0C3D`.

## Native and C boundary

The private run `slot-a-res464-loop-c6e-p5-030` reached the first
flush return at `2D14:05FB`, then the first and second tile-call
entries at `2D14:02FB`. It stopped on the second call's third
`+0C3D` plane iteration, followed by `+0C6E` and the next
`+0C3D`. The same-run 640-KiB RAM difference between the latter
two stops contains exactly four physical addresses:
`0x1BAC7`, `0x20FCF`, `0x20FD6`, and `0x20FD8`.
These are `DS:[0x70C7]` and the three changed stack-local bytes;
the offscreen object has no new changed byte in this loop interval.
The final CPU `AX`, `DI`, `ES`, `IP`, and arithmetic flags match
the original `MOV/LES/ADD` backedge.

`pr_r464_loop_c6e` supports the measured nonzero plane-count and
unequal selector branches, checks every reached RAM address before
mutation, and stops at `PR_R464_LOOP_NEXT_MASKED_BYTE`. The zero-count
and equal-selector branches are typed `OTHER_BRANCH` boundaries;
inaccessible or aliased guest memory has separate typed boundaries.
Synthetic CTest covers the reached writes and CPU state plus those
atomic boundary cases. The hash-locked
`tools/replay/res464_loop_c6e_audit.py` checks the exact original
intervals, 12 reached instruction addresses, 14 CPU fields, all 640 KiB
RAM, and the four changed addresses without publishing source or art
bytes.

The later loop exit, full offscreen object composition, and actual
offscreen-to-EGA transfer remain separate gates. No strict/full or
game-identity count is claimed.
