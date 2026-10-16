# Actual bounded U6F02AE return join to the U6F01B8 caller

The merged01B8 entry gate already executes the real PUSHCS/CALL02AE and
supports normal return to01C3. This separate diagnostic fixture observes
only the source-backed callee epilogue [032B,032F), four bytes / three
instructions. It reuses the unchanged merged stepper, not a duplicate
translation or fabricated successful call. The input must actually be
at loaded2A8E:032B; entry provenance and earlier service completion are
caller obligations. A state already at01C3 cannot certify this episode.

Original GAME.OVR206105B SHA256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Unit base1DFE6; source file[1E311,1E315), SHA256
d938592002da1e8f84eae35df938d5b693d827276f6e045d142340640128831a.
Static disassembly confirms032B MOVSP,BP;032D POPBP;032E RETF.
No instruction in this interval writes RAM or changes flags.

## Live frame and CPU14 map

Let C be the input callee BP, R its current SP, B=word(SS:C),
I=word(SS:C+2), K=word(SS:C+4). Each stack-word offset is16-bit;
an operand crossing the inherited segment/linear-RAM domain blocks.

| Operation | Source effect |
| --- | --- |
| MOVSP,BP | SP=C; all other CPU fields unchanged |
| POPBP | BP=B, SP=C+2 modulo65536; read live saved BP |
| RETF | IP=I, CS=K, SP=C+6 modulo65536; read live far frame |

The report's CPU14 order and mask bits are AX BX CX DX SI DI BP SP CS IP
DS ES SS FLAGS. Only BP,SP,CS,IP may differ across this tail. Normal
01C3 return leaves CS=2A8E; changed-mask bits reflect actual equality,
not a constant expected mask. AX/BX/CX/DX/SI/DI/DS/ES/SS/FLAGS remain
unchanged. FS/GS are reported separately before/after and also remain
unchanged. A14-field report does not drop the two actual16-field values.

For ordinary frames from the merged01B8 entry with SP=S:
callerBP=S-2; callee-entry frameSP=S-030C; calleeBP=C=S-030E.
The live far words atC+2/C+4 are01C3/2A8E, and savedBP atC isS-2.
Return produces BP=S-2 and SP=S-0308. The original outer caller frame
and four-byte argument atS..S+7 remain live; this fixture does not
execute their outer RETF4. The target is always read from RAM. An
altered target commits the actual generic TRANSFER, never a forced01C3.

## Supported and unsupported cases

The fixture stages all three unchanged source steps on a CPU copy and
commits CPU/report only when the complete tail is supported. The tail
is RAM-read-only, so blocked/argument cases preserve the entire fixture
CPU/RAM/report. This aggregate fixture guarantee is distinct from
the merged one-instruction stepper's per-step guarantee.

Supported: audited032B input; normal01C3 target or arbitrary live far
target; savedBP changes;16-bit stack offset wrap between operands that
each individually fit the bounded span domain. For example C=FFFE
pops a valid word then reads a far frame at0000.

Unsupported: wrong source PC/segment (including already-at01C3), missing
or excessive RAM, short saved-BP/far-frame spans, word/far operands that
cross the inherited segment/RAM boundary. C=FFFF and a far frame starting
FFFE block before any commit. Host state/report/RAM must be exclusive
and disjoint; loaded source identity remains host-audited.

Pending actualDOS131C:1821 or unsupportedUI02E9 states cannot be passed
off as a completed return. The fixture supplies no DOS/UI callback,
does not change their results and does not retry. Direct01B8 UI0239
remains separate from the02AE031F continuation.

## Changed RAM, validation and next lead

Return-tail RAM changed bytes=0 is supported by the three store-free
instructions and full-RAM comparison in the focused tests. It says
nothing about RAM changed earlier by entry, copying, DOS, locals or
stack frames. With the explicit synthetic test initial state and
complete synthetic DOS0/5 events, entry-to-join changes43 net bytes
in655360-byte RAM; the final tail changes0 and CPU14 mask is02C0.
These are C-fixture observations, not native state or an ordered journal.
Other supplied events/input aliases can yield different prefix deltas.

Strict and ASan/UBSan tests reach032B through the existing01B8 entry,
cover0/5 joins, live-frame transfer, valid/invalid wrap, full fixture
rejection atomicity, missing DOS and unsupported UI. A private static
audit checks the exact four-byte/three-instruction original span.
The fixed three-step tail is a source interval, not a retry cap.

Reused: [entry gate](U6F_01B8_ENTRY_ABI.md),
[bounded02AE](U6F_POSTINSERT_02AE.md), merged u6f_01b8_entry_abi and
u6f_02ae_abi, and existing synthetic test helpers. Private source
packet PMi7649 supplies the broader dependency graph. The focused
recall attempt timed out; it supplies no negative or parity evidence.

Queued A lead for a new PM gate:01C3 file/path setup through existing
B0B/B98 to the actual04F7 text-record call boundary. No implementation
reservation, DOS/UI success, whole-function/native/strict credit follows
from this return fixture. Parity is source-backed for this bounded
read-only tail; whole/native execution remains unclear. New credit ZERO.
