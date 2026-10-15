# Unit 2F +122B record-link clearer

## Pinned extent and game-role evidence

The complete call-free leaf is U2F:+122B..1279 inclusive, GAME.OVR
file [43570,43649): 79 bytes, 23 contiguous instructions, SHA-256
ef58c3eedf096f21b9b923c9c303dfc629e5f0935d2673dabf93e1bc9749de9a.
Both conditional branches stay in the body. RETF 4 is at +1277; there
are no calls, interrupts or relocation words. Unit file base is 38919,
resident stub segment 0035. The original media are START.EXE (65231 bytes),
SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd,
and GAME.OVR (206105 bytes), SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

Two instruction-aligned PUSH CS / near CALL pairs occur at +0464/+0465
and +0559/+055A in the game_evidenced +031A caller. The second passes a
record from DS:[5D96/5D98] and then follows its +104/+106 far link.
The audit verifies this dataflow from aligned original instructions.
The [party counter](RESIDENT_PARTY_COUNT.md) independently establishes
that head/link pair as a traversal of character records. The nested
record +108 -> child +0A pointer chain is also documented in
[U38 effect entry 110](OVERLAY_UNIT38_WALK_ENTRIES.md), and character
byte +10E appears in the [resident predicate](RESIDENT_CHARACTER_FIELD_ZERO.md).
These are game-record signals, not names for the field's gameplay meaning.
The leaf remains helper_candidate pending PM review; no classifier,
identity, strict manifest or completion denominator changes here.

## Complete ordered behavior

pr_u2f_record_link_clear_122b takes 1 MiB RAM and 14 CPU words after
CALL, with a far record argument at SS:SP+4. It saves BP without reserving
locals. The first store sets record[10F] to 1. Every later LES reload
occurs at its original site, including reloading the far argument after
this store. The source does not check the record or child pointer for null.

It follows record[108] to a child and ORs the two words at child[0A/0C].
If both are zero it returns. Otherwise it reloads the argument and child,
follows child[0A] to a target, loads AL from target[10E], reloads the
argument, and compares AL with record[10E]. Unequal bytes return without
clearing the child pointer. Equal bytes reload the argument and child,
zero AX, then store the offset word at +0A followed by the segment word
at +0C. This order remains observable through aliases.

Null and equal paths return AX=0 and ES:DI=the last child pointer.
The unequal path returns ES:DI=the reloaded record, with AL=target[10E]
and AH retained from the earlier OR of the target pointer words.
BX/CX/DX/SI/DS/SS are preserved. Actual RAM supplies restored BP and
far return CS:IP; entry SP advances eight bytes overall through RETF 4.
There is no MOV SP,BP epilogue, loop cap, callback or original-byte input
to the C model. Null host pointers or short RAM are rejected before mutation.

The null path's last flag writer is OR; the equal path's is XOR. Both
define CF/PF/ZF/SF/OF and leave AF architecturally undefined. The C
chooses AF=0 there; audits mask only that bit on those paths. The unequal
path's last writer is byte CMP, so every flag including AF is compared.
Other FLAGS bits remain unchanged. Effective offsets/SP wrap at 16 bits;
multi-byte memory and four-byte RETF operands use contiguous physical
bytes with A20 off. All arguments, records and frames share RAM; code
must remain immutable.

## Measured parity

The pinned original-byte/C audit passed **1143 cases**, comparing every
byte of 1 MiB RAM and all 14 CPU words. All 23 instruction sites and all
four conditional edges were covered. Terminal paths were 705 CMP exits,
one null OR exit and 437 equal XOR exits. Cases cover all equal byte
values, unequal byte/overflow/borrow boundaries, null and single-word
target pointers, record/child/frame/chain aliases, offset/A20 wrapping,
and 128 seeded whole-RAM backgrounds with varied initial flags.

The DOSBox-X normal-core/386 oracle passed **518 cases**. It copies the
exact +0553..055C caller suffix at its original offset: argument pushes
from the caller's BP-6/BP-4 locals, PUSH CS and the unchanged relative
near CALL. The pinned original leaf runs at +122B. Synthetic setup
supplies the caller locals; earlier party traversal and its conditions
are not executed. Natural game reachability remains unmeasured.

Native cases include 256 equal and 256 unequal byte pairs, null target,
zero offset with nonzero segment, equivalent far-pointer representations,
first-store saved-BP mutation, argument redirection, and final argument-word
clearing. Each compares all 14 CPU words, 320 stack/alias bytes and 1536
record-region bytes, covering all writes for these fixtures. Only undefined
AF is masked on logic exits; mismatch CMP flags are exact. IRQs remain
disabled through setup/execution/capture, then enabled for DOS I/O.
Native SHA-256:
20ccdce06d01ca349b3a4e91adf12bafb49d3d2606415e6bdfd2f5abb589fb21.
The output includes the DOS allocation segment, so another environment
can change this hash while preserving parity.

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u2f_record_link_clear_122b_audit.py --output resources/u2f-122b-parity/differential.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u2f_record_link_clear_122b_native.py

The focused C test verifies input guards, all three exits, partial AX,
defined flags, preserved registers/neighbors, first-store pointer redirection
and both return words being cleared through aliases. Strict warnings build,
235 CTests, research hashes, oracle shell syntax, tracked-media exclusion
and whitespace checks pass at the initial integration base. All original
bodies, caller bytes, assembly, COM, disassembly, output and logs stay under
ignored resources/. The audit uses Capstone 5.0.7 and Unicorn 2.1.4.

## Review, reuse and follow-on

target_function: OVL_002F_122B. source_candidates: the new raw C model.
signals: pinned exact extent/ABI and CFG; aligned party-record caller and
shared record fields; full-state differential with complete site/edge
coverage; original-caller native parity including pointer/frame aliases.
score: 4/4 listed checks pass, a checklist rather than a probability.
verdict: match under the immutable-code, A20-off, entry-frame contract.
review_notes: undefined AF is explicitly limited to OR/XOR exits; gameplay
field names and natural reach are unmeasured; PM decides identity/credit.

A bounded 15-second deja lookup yielded no completed finding. Reused
repository findings are the cited character/party/pointer-chain notes,
the U86 raw CPU/RAM audit utilities and IRQ-isolated caller oracle method.
Extent, caller alignment/dataflow and parity counts were measured again.
Branch base: 7d319e2b7a2d73b713ade29069693d9de581d848.

The next disjoint candidate sent to PM is OVL_006C_3F31. The freshly
regenerated address-only queue ranks it 457: helper_candidate, grade B,
span upper bound 130, outgoing calls 0, incoming sites 4. Remeasured
extent +3F31..3FB2 is 130 bytes/44 instructions, RETF 4, SHA-256
9f192cb8fe704324facbbec368d1e24602558b878aa3f7e0031f644862e09767.
Four aligned callers are +056C:+1D5C, +29AE:+2FE8, +3FB3:+4018 and
+408C:+41C1; three are game_evidenced. No implementation/strict/open-PR
collision was found. U38/U7D/U6A and the reserved U7F +05E0 are excluded
from selection. U6C is not implemented or credited in this PR.
