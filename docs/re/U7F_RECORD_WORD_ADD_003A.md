# U7F +003A record-word addition

## Publication refresh (2026-10-17)

This additive publication reuses archived branch codex/u7f-record-word-add-003a
at 460dfd516d83a8090063a857fafa074ff1fbe8de. The C implementation is unchanged.
It is rebased onto current main; the CMake integration preserves every
existing registration and adds only this source and focused test.
The original branch was already pushed but had no PR. No superseded
implementation was found in current main.

The historical evidence below is retained for provenance. Fresh original-byte,
native, strict C11/ASan/UBSan and CMake/CTest results are recorded in the PR
and ignored resources/publication/ logs. Earlier whole-suite counts below
describe the archived run, not a new whole-suite claim.

The pre-publication deja query timed out without a reusable result. Reuse
is attributed to the Git head above. Independent review and credit remain
with the reviewer/VP; this publication does not change acceptance,
classification, or accounting. Native cases are synthetic selected-memory
captures, distinct from the original-byte whole-RAM comparisons. Native
coverage applies only to the listed frames and is not a general proof of
every segmented-memory boundary. CPU storage must be separate from RAM;
the code is immutable, execution synchronous, and hardware faults/devices
are outside the flat A20-disabled RAM profile.

Return-profile caveat: this archival API reads the RETF CS word at the
contiguous physical address IP-word+2. That matches its Unicorn profile.
The 544 native cases use ordinary return placements; they do not establish
native FFFE/FFFF behavior. Other measured normal/386 RETF bodies use
separately wrapped SS word offsets there. Callers requiring those native
boundary placements must not treat this API as a proved wrapped-SS model.

`pr_u7f_record_word_add_003a` translates the complete 26-byte, nine-instruction
leaf at U7F:+003A..0053. It adds a word argument to a far record's word +0102h,
preserving the original RAM order, exact ADD flags, and POP BP/RETF 6 effects.
The field's semantic name is unverified. Natural game reach and strict-credit
eligibility remain separate PM gates; no accounting file changes.

## Source and callable evidence

Owner inputs: START.EXE, 65,231 bytes, SHA256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
GAME.OVR, 206,105 bytes, SHA256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U7F's file base is 150665, original stub segment 008Fh. The exact leaf
occupies [150723,150749), SHA256
`ef4d49e3fef030f2acc13a9d5f78563d091bb60ce167bb986143671b6e14059f`.
The next callable starts at +0054. All 26 bytes decode into nine instructions,
ending in +0051 RETF 6. There are no branches, calls, interrupts or relocations.

The private current-main ranked overlay ledger places this address at rank
442, grade B, helper_candidate/unknown: zero outgoing calls, 26-byte bound,
nine validated incoming near-call sites. Ranking is selection evidence, not
identity or a whole-game completion measure. U7F +05E0 was excluded because
its merged translation and reservation already exist. PM assigned this
distinct leaf in inbox i3614; local source/claim and public open-PR checks
found no competing implementation.

All nine raw E8 target sites are independently instruction-aligned, each
preceded by PUSH CS to supply the far-return ABI:

| Caller start | Near sites | Measured record provenance |
| --- | --- | --- |
| +0203 | +024D, +028B | Active far pointer DS:5D92/5D94; preceding writes to record +0090 |
| +044C | +04E9 | Far pointer loaded indirectly through caller argument +000E; ownership beyond that argument is unmeasured |
| +063A | +07A8, +0805, +086C, +091D, +097E | Saved current party-node pointer at BP-4/BP-2, initialized from DS:5D96/5D98 in two passes |
| +0A88 | +0B51 | Explicit caller far-record argument at BP+000A; preceding record +0088 write |

The +063A root loads are +0640/+0643 and +08A8/+08AB. The next-node reads
at +087B/+0880 and +0991/+0996 use record +0104/+0106, then update the same
saved pointer. Thus the five calls use party nodes, while the two +0203
calls use the active record. The remaining two records stay typed as caller
arguments. These source-backed ownership signals do not establish that any
of these paths was naturally reached in a native game session.

## Complete frame and RAM contract

Entry SS:SP holds a far return, SS:SP+4 the increment, SS:SP+6 the far record.
The leaf pushes BP, sets BP=SP, loads the record pointer through LES,
reads its word +0102h, then reads the increment and performs a 16-bit ADD.
It reloads the far pointer before writing the result, pops BP from live RAM,
and performs RETF 6. AX returns the word sum; ES:DI is the reloaded record.
BX/CX/DX/SI/DS/SS and defined non-arithmetic flags retain their source effects.

Saved BP, return IP/CS, arguments and record storage may alias. A store that
changes the outer return frame remains visible through returned CS:IP; the
model does not follow that destination. The pushed BP can become the read
record word and be changed before POP BP. LES and multi-byte operands use
contiguous physical bytes; effective offsets/SP wrap at 16 bits, physical
A20-off RAM at 20 bits. Code must remain immutable and disjoint from writes.
Null pointers or RAM smaller than 1 MiB are rejected before mutation.

## Original-byte and native parity

```sh
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u7f_record_word_add_003a_audit.py \
  --output resources/u7f-003a-parity/audit.json
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tools/overlay/u7f_record_word_add_003a_native.py
```

The full original-byte oracle passes 2,212 cases, comparing all 1 MiB RAM
and fourteen CPU words with no flag mask. All nine instruction sites are
covered. Tests include carry, overflow, auxiliary carry/parity boundaries,
record/frame aliases, changed return IP/CS, 16-bit frame/operand wrap,
A20-off physical wrap and deterministic random initial memory/flags.
The quick matrix passes 376 cases. No loop budget/exclusion is needed for
this straight-line leaf; the original execution is bounded by its nine starts.

The DOSBox-X normal-core 386 fixture passes 544 cases executing the original
+0241..024F incoming prefix and complete leaf in a separately allocated code
segment. A transparent synthetic far jump captures the original +0250 return
before further caller instructions. It does not substitute the helper result.
All fourteen CPU words and exact flags, 64 frame bytes and 288 record bytes
match C. Four cases alias saved BP, the increment, record offset and record
segment while retaining the synthetic capture destination. Other changed
return destinations are tested by the full-RAM oracle, not followed natively.
Native RESULT.BIN SHA256:
`6c870390586b73d9d5894f7a40602faee3415e5d92b1f38ae914d4162a15c864`.
Native captures are synthetic frame evidence, not naturally reached game
calls or whole-RAM native capture. Loader dispatch is unmeasured.

All copied code, generated COM files, disassembly, captures and logs stay
under ignored resources/u7f-003a-parity/. Synthetic C tests check exact flags,
unmodified registers, argument guards and saved-BP/return-frame aliases.
All 246 CTests pass with -Wall/-Wextra/-Wpedantic/-Werror, including the
focused synthetic test. Shell syntax, research checksums, staged whitespace
and public-media hygiene checks pass.

- `target_function`: U7F:+003A..0053.
- `source_candidates`: new uniquely named raw record-word addition source.
- `signals`: exact extent/hash/return, nine aligned callers, active/party
  record provenance, source-ordered RAM/frame effects, complete original-byte
  CPU/RAM comparison and independent synthetic native execution.
- `score`: nine of nine instruction sites and all measured parity cases;
  no game-function denominator or percentage claim.
- `verdict`: match within the immutable-code/raw-frame contract.
- `review_notes`: the field name, natural game reach, identity/count eligibility,
  and merge remain separate gates.

Bounded deja target/ownership queries returned no completed matching finding.
Reused findings: [merged U7F party-head/next-link evidence](U7F_PARTY_COUNT_05E0.md),
[merged raw CPU/offset conventions](U6C_RECORD_TABLE_SUM_3F31.md), and reviewed
+4235/+408C separate-code/native-frame audit design. No unmerged symbol is
required. CMake registration overlaps the four pending U6C registrations;
integration must retain all entries.
