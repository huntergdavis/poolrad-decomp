# Unit 0x68 internal +0x05EE: far-list record search

`pr_u68_list_search` translates the 90-byte DOS v1.3 `GAME.OVR`
routine at unit `0x68:+0x05EE..+0x0647`. It walks a far-pointer list,
uses the merged [record-byte scan](U68_RECORD_SCAN_05AB.md) at each
node, and returns the first matching node in AX:DX or null 0:0 when
the list ends. For finite lists with mapped far segments and readable
next words, all decoded paths are represented. Host-only missing-view,
cycle, scratch-capacity, and segment-view boundary guards keep malformed
inputs bounded. PM byte-parity review of PR #149 found a complete match
for valid finite lists. The postmerge game-state audit below gives this
routine one strict full-C game-function credit and one verified identity.
The saved one-Up route through this helper is not established.

Before implementation, `deja "Poolrad unit 0x68 +0x05EE list search +0x0614 pointer ABI no match"`
and `deja "Poolrad 0x05EE list search"` found no prior exact trace.
Reused and byte-checked [the merged +0x05AB translation](U68_RECORD_SCAN_05AB.md)
(PR #145) for the callee's 21-byte search/AL result and
[the slot-A route note](SLOT_A_MINUTE_WRITER_BOUNDARY.md) (PR #143)
for the static entry-1 connection. Neither source proves live use of
this search during the captured Up key.

## Hash-locked boundary and ABI

The ignored local `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x68` starts at file `0x166CB`; this routine occupies file
`0x16CB9..0x16D12` inclusive, **90 bytes / 33 contiguous decoded
instructions**, SHA-256
`a7c44a16897e088df3dd64221b58be8efb68344b21ab3083b0748de5b21dc10f`.
Decode ends exactly at the next prologue `+0x0648`; its final
`+0x0645` instruction is `retf 2`.

The sole argument is a word at `[bp+6]`; only its low byte is passed to
the callee. The initial far node is read from DS words `[0x5D96]`
(offset) and `[0x5D98]` (segment). Null means **both words zero**:
`+0x0601` ORs the words, and `+0x0607` takes the empty-list branch
only when the result is zero. The return is AX=node offset,
DX=node segment; exhausted or initially empty lists return AX=DX=0.
Both direct callers use `push cs; call near +0x05EE`, providing the
far return address for `retf 2`.

| Offsets | Instruction-backed effect |
| --- | --- |
| `+0x05F4..+0x05FE` | Copy the DS far list head to the local current pointer. |
| `+0x0601..+0x0607` | If the OR of offset and segment is zero, branch to return current 0:0 without reading a node. |
| `+0x0609..+0x0619` | Push the argument low byte and current far node, `push cs; call +0x05AB`; `AL != 0xFF` returns the **current pointer**, including a hit at scan index 20. |
| `+0x061B..+0x062E` | On `AL=0xFF`, read the next offset from `ES:[node+0x104]` and next segment from `ES:[node+0x106]`, then repeat the null test. The two effective word addresses use 16-bit offset arithmetic. |
| `+0x0630..+0x0645` | Move current pointer to AX:DX and `retf 2`. After a final miss followed by a null next pointer, this is AX:DX=0:0. |

The C API receives the DS head as a far value and maps each ES selector
to a synthetic 64 KiB byte view. It invokes the merged
`pr_u68_record_scan` at exactly the reached node; the scan's AX result
controls the same hit/miss branch. Next-pointer words are read only
after a miss. A word beginning at effective offset `0xFFFF` would
straddle the supplied 64 KiB host view; this API returns ARGUMENT
before that unsupported read. The original has no host-view, cycle,
or capacity guard, so those guard statuses are outside DOS parity and
leave the result unchanged.

## Static callers and live-route limit

A prologue-bounded instruction walk of unit `0x68` found exactly two
direct near calls into `+0x05EE`: `+0x0652` in helper
`+0x0648..+0x068C`, and `+0x0E18` in entry 1
`+0x0D6D..+0x10A6`. At `+0x0652`, the caller pushes value `0x1F`,
then tests the returned AX:DX for null before calling scan
`+0x05AB` again and clearing a matched record byte. At `+0x0E18`,
entry 1 pushes the same value after a nonzero `[0x6DE0]` gate and
branches on the returned far pointer. These are static links;
the saved one-Up observation provides no guest PC trace showing either
call executed. The sought byte's meaning remains unclassified.

**Postmerge ownership gate.** The PM independently decoded all 90 bytes and
both near call sites against the pinned DOS build. The routine reads the
documented party-list far pointer `[0x5D96]`, walks member links at `+0x104`,
passes a byte to the separately verified record scan, and returns a matching
party member. Its direct callers are unit `0x68`'s record-clear helper and
game entry 1. This is game-state traversal. The C translation covers every
finite valid-input path; cycle, segment-view, and scratch-capacity guards
apply only to host error cases. Promote only `0x68:+0x05EE` to verified
identity and award one strict full-C credit. The static callers and the
observed one-Up store do not prove this search ran in that saved-key execution.

## Synthetic verification

`test_overlay_u68_list_search.c` fabricates two 64 KiB ES views and
their far next pointers. It checks empty head without a view, first
node hit without reading next, first miss then index-20 hit in the
second node, full exhaustion to 0:0, nonzero segment with offset zero,
16-bit effective-offset wrap for the next pointer, self and two-node
cycles, scratch capacity, missing/short views, and both straddling-word
guard positions. No original record or media bytes enter the test.
