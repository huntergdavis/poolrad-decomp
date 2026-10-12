# Unit `0x6F:+0x1446` SS-local memory and helper-stack boundary

`pr_u6f_party_insert_1446_ss` extends the merged [20-bit RAM model](U6F_PARTY_INSERT_1446_DOS.md)
through the source `+0x1574` far-call transfer. It reads the incoming far
argument from SS stack words, writes all 16 callee local bytes to the same
1 MiB RAM as DS globals and ES records, and models the transient stack
stores made by both SYSTEM helper call sites. The existing C-local API is
preserved. This entry stops before the overlay thunk and u9B callee; it
claims no strict full-C or identity count.

Before debugging, a long `deja` CLI query stalled; MCP recall for the
shorter `u6f 1446` query recovered prior PM party-build research. Reused
the [PR #257 source audit](U6F_PARTY_INSERT_1446.md), [PR #264 RAM model](U6F_PARTY_INSERT_1446_DOS.md),
[PR #265 SS alias counterexample](U6F_INSERT_SS_FRAME_ALIAS.md), and the
[native first-insertion capture](SLOT_A_NATIVE_19E3_INSERTION_RETURN.md).
The original spans and loaded saved set were independently checked again.

| Span | Size / result | SHA-256 |
| --- | --- | --- |
| GAME.OVR | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit `+0x1446..+0x157E`, file `0x1F42C..0x1F564` | 313 bytes; 99 instructions | `370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245` |
| SYSTEM fill, loaded `0xC628..0xC63B` | 20 bytes | `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7` |
| SYSTEM membership, loaded `0xBD40..0xBD60` | 33 bytes | `753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c` |
| CS set at `+0x1426..+0x1445`, GAME file `0x1F40C..0x1F42B` | 32 bytes; same in saved loaded RAM | `d22f8e3450655784853879c7ecfee4ff36192352f6ffe0d40593dee56f583350` |
| u9B body `+0x01AF..+0x0254` | 166 bytes; 49 instructions | `5dd90fb8796e7aa15a770e4868aaceeee0a0886e0251a9fc7c10f3f32935652c` |

The saved `+0x19E3` caller has `SS:SP=1D16:37B2`, `BP=3DCA`, and
`CS=2A8E`. The caller's near call pushes its return IP at `SS:37B0`;
callee `+0x1446 PUSH BP` writes `SS:37AE`, then sets `BP=37AE` and
`SP=379E`. The far argument is at `SS:37B4/37B6`. The new API receives
the **entry SP after that near call** and the caller BP, then performs the
callee prologue itself. Tests put the near-call return IP into synthetic
RAM; the native test inserts that single word into a copy of its pre-call
capture.

## Reached transient SS writes

| Site | Pushes / helper reads | Saved-run physical write range |
| --- | --- | --- |
| `+0x14C1..+0x14CA` fill call | SS, local offset, length 8, value 0, return CS, return IP; SYSTEM `+0x16A8` reads the arguments and writes eight bytes through REP STOSB, then RETF 8 | `0x208F2..0x208FD` |
| `+0x14EC..+0x14F2` membership call | Slot AX, CS, set offset, return CS, return IP; SYSTEM `+0x0DC0` reads low index and set pointer, tests one loaded CS bit, then RETF 6 | `0x208F4..0x208FD` |
| `+0x156E..+0x1574` u9B call site | Incoming segment, incoming offset, return CS, return IP | `0x208F6..0x208FD` |

These writes remain in RAM after stack cleanup. In the synthetic
`SS:36E3` counterexample they overlap the incoming record's counter
bytes: at the transfer boundary its `+0xB2..+0xB4` bytes have values
`0x14,0x79,0x15` from helper/final-call words, while the prior C-local
model leaves them zero. The same synthetic case still selects slot 0
through the RAM-backed `used[0]` alias, matching the PR #265 bounded
probe; the C-local model selects slot 1. The loaded set is read from
CS RAM, so prior memory writes can affect the membership test.

The new traversal has no host node limit for finite lists. Synthetic
tests include the two-node alias, a disjoint control, and a 300-node
finite list. The hash-pinned native first-insertion gate copies the
655,360-byte pre-call RAM into the model, then compares the returned
capture's head, active pointer, window count, and incoming slot. These
fields agree. The comparison ends at the model's **pre-callee transfer**
and does not claim whole-RAM, u9B counter, or final register parity. The
optional [second native insertion capture](SLOT_A_NATIVE_SECOND_SLOT_INSERTION.md)
adds a nonempty-head gate: one append node, two mark nodes, a new first
record link, selected slot 1, and window count 2 agree with its returned
snapshot. It too is a field comparison across the unresolved callee.

The first unresolved effect is execution of the overlay thunk at
`0x00AC:0x0025` after the source far call at `+0x1574`. The u9B body is
known to begin at `+0x01AF` with `PUSH BP`, then write a four-byte local
far pointer at `+0x01B8/+0x01BB`; the intervening thunk/dispatcher stack
and memory effects are not modeled here. Strict full-routine credit
requires that transfer and its callee stack effects, or a proved
non-overlap contract for them.

Reproduce with ignored inputs:


```sh
bash tests/recovered/check_overlay_u6f_party_insert_1446_ss.sh
bash tests/recovered/check_overlay_u6f_party_insert_1446_ss.sh \
  resources/DOS/POOLRAD/GAME.OVR resources/DOS/POOLRAD/START.EXE \
  resources/path/to/ignored-pre-ram.bin resources/path/to/ignored-post-ram.bin
# Optionally append the second insertion's ignored pre/post RAM captures.
```

No original code bytes, save data, RAM captures, or media are committed.
