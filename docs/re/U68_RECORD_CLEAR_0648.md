# Unit 0x68 internal +0x0648: clear first matching list record byte

`pr_u68_record_clear` translates the DOS v1.3 `GAME.OVR` routine at
unit `0x68:+0x0648..+0x068C`. It calls the merged far-list search
`+0x05EE` for byte `0x1F`, scans the returned node again through the
merged `+0x05AB` helper, and clears that node's byte at `+0x17+index`.
It returns AX=1 when a node was found and AX=0 for a null list result.
The translated valid-list paths are complete; explicit host mapping,
cycle, and scratch guards bound malformed input. **No strict full-C or
game-identity count is claimed before PM parity review.** The saved
one-Up route through this routine is not established.

**Postmerge ownership gate (PR #152).** PM independently decoded the full
69-byte span and its two exact near calls. The first callee traverses the
documented party list; the second locates byte `0x1F` in that party member's
record, and this routine clears the matching record byte. This is a complete
game-state mutation for valid mapped lists. Award one strict full-C game
translation and one verified overlay identity. The live one-Up watch in
[the later address-only note](SLOT_A_ONE_UP_NATIVE_WRITE_WATCH.md) proves the
coordinate/clock writer, not execution of this record-clear branch.

Before implementation, `deja "Poolrad unit 0x68 +0x0648 record clear +0x066D list search ABI"`
found no prior exact trace. Reused and independently byte-checked the
[merged record scan](U68_RECORD_SCAN_05AB.md) (PR #145) and
[merged list search](U68_LIST_SEARCH_05EE.md) (PR #149) for the two
callee contracts. The [slot-A route boundary](SLOT_A_MINUTE_WRITER_BOUNDARY.md)
provides context for entry 1 but does not establish execution here.

## Hash-locked span and ABI

The ignored local `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x68` starts at file `0x166CB`. The target occupies file
`0x16D13..0x16D57` inclusive: **69 bytes / 29 contiguous decoded
instructions**, SHA-256
`16c166e8281ce17e48fb68a5cc95f64c098ede2f6a1f66d85de1f78597c91146`.
The final `retf` ends at `+0x068D`, exactly the next prologue.

The routine has no stack arguments. `push cs; call +0x0648` supplies its
far return address; its final instruction is plain `retf`. The initial
list head comes from the `+0x05EE` callee's DS:[0x5D96] far pointer.
The C API receives that head, mutable 64 KiB segment views, read-only
adapters for the merged list search, visited scratch, and AX output.

| Offsets | Instruction-backed behavior |
| --- | --- |
| `+0x064E..+0x0652` | Pass low byte `0x1F` to list search `+0x05EE`. The high argument byte is immaterial because the callee uses only AL. |
| `+0x0655..+0x0661` | Store returned AX:DX far node and OR both words. Only null 0:0 takes the no-write path at `+0x0682`. |
| `+0x0663..+0x0670` | Pass the same low byte and returned far node to record scan `+0x05AB`, then clear AH. |
| `+0x0672..+0x0677` | Load ES:DI from the node, add scan index, and write zero to ES:[DI+0x17], using 16-bit offset arithmetic. No other guest byte is written. |
| `+0x067C..+0x068C` | Return AX=1 after a found node, or AX=0 for null, through `retf`. |

The second scan result is not tested for miss. If the node changes
between calls and that scan returns `0x00FF`, the DOS arithmetic clears
node byte `+0x116`; the C path retains the same index arithmetic. In a
stable synchronous view, the earlier successful list search ensures the
second scan finds the same first index. The C wrapper introduces no
mutation between the calls. It returns a host guard error without an AX
write for missing/short segment views, cycles, or exhausted scratch;
the original DOS loop has no such error exits.

## Callers and route limit

A prologue-bounded decode of unit `0x68` found two direct near calls to
`+0x0648`, both in entry 1 (`+0x0D6D..+0x10A6`): `+0x0EE4` and
`+0x1021`. Both are preceded by comparisons of AL with `0x4B` at
`+0x0EDF..+0x0EE1` and `+0x101C..+0x101E`, respectively; both store
the returned AL in local `[bp-1]`. The later branch checks that local
at `+0x1027` and conditionally calls coordinate writer `+0x073B` at
`+0x102E`. These are static instruction links, not a measured guest-PC
trace of the saved one-Up replay.

## Synthetic verification

`test_overlay_u68_record_clear.c` uses fabricated 64 KiB segment
views. It checks empty head without a view, first-hit clear of exactly
one byte, first-node miss followed by second-node index-20 hit, full
list exhaustion without a write, nonnull far pointer with offset zero,
16-bit wrapped write offset, and cycle, capacity, missing-view, and
missing-output guards. No original game bytes are embedded.
