# U3CE +014E: entry and conditional external-effect gate

This separate PM fallback from i3576 classifies the original body and first
external-effect boundaries needed by U6F's later +065F call. It adds no C
translation, heap behavior, native reach or strict/game-count credit.

Pinned DOS v1.3 media are START.EXE SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
and GAME.OVR SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The bounded prefix recall expired without a finding. Reused leads are
[the entry-4 map](U3CE_ENTRY4_01C8.md),
[the resident ledger](resident_callable_ledger.csv) and
[the CFG method](../../tools/overlay/u6f_vector_merge_032f_audit.py).
Exact entry and operands are independently checked by the new audit.

## Exact extent and ABI

Unit 3CE's original base is 0x31A6C. Public entry 3 at source stub
03E9:002F resolves to +014E. Executable extent is **[+014E,+01B0),
98 bytes / 40 starts**, file [0x31BBA,0x31C1C), SHA-256
244d2bbd214ceb86d9d6daa80252da6af77fa0a92e4a287ac555aaca26b3c047.
All executable bytes belong to the bounded CFG. The last instruction is
+01AD RETF 2. The old 122-byte next-entry bound includes 24 trailing literal
bytes, which are excluded here.

Entry SS:SP holds return IP/CS and one WORD argument at SP+4. After PUSH BP,
the low byte at [BP+6] selects one of 256 eight-byte records in the DS table
starting at 5E5A; the argument's high byte is ignored. The body reserves two
stack bytes and ORs each pair's two words to test for a nonzero far pointer.

| Live guard | First external stop | Far argument left on stack |
| --- | --- | --- |
| First pointer pair nonzero | +017A CALL 0464:2595 | address DS:(5E5A + 8*slot) |
| First pair zero, second nonzero | +01A5 CALL 0464:2595 | address DS:(5E5E + 8*slot) |
| Both pairs zero | no external call; +01AD RETF 2 | none |

Both calls have segment fixups. The argument is the **far address of the
pointer pair**, not its stored object pointer. Before a call, all store
instructions address the SS stack; none
explicitly stores through the DS table or an object pointer. A stack/table
alias can still make those stores affect table bytes. The unresolved
resident effect may mutate table/heap state. After the first call returns,
both the slot argument byte and second pointer pair are reloaded from live
RAM and cannot be assumed unchanged. Table/pointer names do not establish
allocation ownership.

## Measured probes and blocker

The audit executes original bytes for **12 cases**: slots 0, 1 and 255,
crossed with both-zero, first-only, second-only and both-nonzero pointer
states, with a nonzero ignored high argument byte. Each case stops before
its first external call, or executes the no-call RETF separately. Checks
require the expected stop, actual pointer-to-pointer argument, stack-only
writes, and returned CS:IP/SP/BP for the zero path. These are original
boundary probes, not C parity.

The existing [bitmap-release model](RESIDENT_464_BITMAP_RELEASE.md),
pr464_bitmap_release, already translates 0464:2595 for host memory/free
callbacks. Its free_block callback and raw CPU/heap composition remain
unresolved here. Heap release and any returned continuation remain pending.
The separate U6F prefix's first unresolved
call is entry 4 +01C8 at +04D8. This entry-3 gate covers the later cleanup
dependency.

Run the entry/source gate without the C differential:

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6f_prefix_0482_audit.py --source-only

Original bytes, literal tails and RAM remain ignored under resources/.
Public evidence is limited to hashes, counts, addresses and observed ABI.
