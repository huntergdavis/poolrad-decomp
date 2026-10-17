# Unit 6C +408C complete weighted record caller

## Source and game-role evidence

The complete body is U6C:+408C..4234 inclusive: 425 bytes, 176
contiguous instructions, GAME.OVR file [121355,121780), SHA-256
a6f888761ea4f984f7562342143d2a366885d4248d4c047b5f0b42cdf26ff97e.
Its sole call is +41C1 -> merged [U6C +3F31](U6C_RECORD_TABLE_SUM_3F31.md),
through PUSH CS at +41C0 and exact near-return IP 41C4. The callee adds
44 instruction sites. RETF 4 is at +4232; all branches stay in this body.
There are no fixups or INT instructions. Opcode 98 is CBW and 99 is CWD
in 16-bit mode, despite decoder labels.

Pinned owner media are START.EXE (65231 bytes), SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd,
and GAME.OVR (206105 bytes), SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Unit 6C's file base is 104831 and original stub segment is 0078.

Four aligned incoming calls in +209F are +2472/+2487/+281E/+2833.
All push DS:[5D94]/[5D92], the independently verified active-character
pointer from [U6F insertion](U6F_PARTY_INSERT_1446.md), and compare or
store AL against record +32. The containing caller and this function
are canonically game_evidenced. Specific meanings of +32, +96..9D,
selector +14, qualifier +2F and the result remain unassigned. No canonical
classification, identity or strict-accounting file changes here.

## Complete ordered C behavior

pr_u6c_record_weighted_408c shares the merged callee's raw 14-word CPU.
It saves BP, reserves ten bytes, copies its far argument from SS:SP+4
to local words [BP-9]/[BP-7], and initializes count [BP-2], weighted
byte sum [BP-3] and index [BP-5]. For indices 0..7, signed-positive
argument[96+index] bytes enable that slot and increment count. The source
reloads the index, local far pointer and record byte before weighting:

| Index | Contribution before the byte addition |
| --- | --- |
| 0, 1 | 8*value |
| 2, 3 | 10*value, unsigned word MUL |
| 4 | 8*(value+1), word INC then shift |
| 5 | 4*value |
| 6 | 6*value, with SI receiving 2*value |
| 7 | 4*(value+1), word INC then shifts |

Every addition to the local sum wraps at eight bits. Shift-by-CL sites
0/1/4 set CL=3 while retaining CH. MUL retains the actual DX:AX product;
index 6 retains its SI side effect. These registers remain observable
at an altered internal return, and SI remains observable on normal exits.

The callee argument is reloaded separately from DS:[5D94]/[5D92], rather
than reusing the outer argument. The API therefore preserves two independent
records. It pushes that live global far argument, CS and near-return 41C4,
then executes the complete merged +3F31 C model with shared RAM/CPU.

The callee byte is stored at [BP-4] and interpreted as signed. For a
nonnegative adjustment, signed-divide (weighted sum+adjustment) by count.
For a negative adjustment, clamp to 1 when weighted sum <= abs(adjustment)
+count, otherwise perform that signed division. The full word quotient
remains in AX while AL is copied through the local result: a tested case
returns AX=016E (366), not just AL=6E. CX/DX retain the divisor/remainder
or clamp-state values. BX/DS/SS are preserved; SI may have changed at slot 6.
ES:DI is the callee's returned record pointer. Actual RAM supplies restored
BP and far CS:IP, with RETF 4 advancing entry SP eight bytes overall.

DIVIDE_ERROR preserves exact state before faulting IDIV at +41FF/+4221;
no CPU vector frame is synthesized. TRANSFER exposes an altered internal
callee return instead of inventing continuation. Clamp and pre-fault flags
are defined and compared exactly. Successful IDIV leaves arithmetic FLAGS
08D5 undefined; only those six bits are masked there. The C chooses the
preceding ADD's arithmetic flags. All other FLAGS remain exact.

Locals, both arguments, DS globals/table and frames share A20-off 1 MiB
RAM. Every original reload/store remains ordered. Offsets/SP wrap at 16 bits,
wide operands use contiguous physical bytes and code stays immutable.
Null host pointers/short RAM are rejected before mutation. There are no
callbacks, original-byte inputs or host loop caps.

The index is initialized to zero and only INC changes it, so it is always
0..7. The selector default edge +4196 -> +41AF is therefore infeasible;
the fallthrough at +4196 is reached by enabled slot 7. The audit requires
all 35 feasible conditional edges across caller/callee and records this
one excluded static edge explicitly, rather than claiming all 36.

## Measured gates and native fixture

The original-byte/C audit passes 3265 full-1MiB/all-14-word cases,
covering all 220 caller/callee instruction sites and 35 feasible edges.
Outcomes: 3071 returns (2893 divisions, 178 clamps), 188 divide faults
and six altered internal returns. Cases include all selector/gate/table
boundaries, enable patterns, separate/shared records, global-pointer/frame
aliases, offset/A20 wrapping, full AX quotients and 128 seeded full-RAM
backgrounds. Faults stop before IDIV vectors; transfers stop before the
first instruction at the unexpected target. Every defined flag matches.

The native run passes 1029 cases: 1026 returns (953 divisions, 73 clamps)
and three real INT 0 faults. There are 1025 original incoming-prefix cases
and four distinct-record direct calls. Native output SHA-256:
4749ec32cf66990a0589ccddda1927e227f1ee8380d4ee8cd3a15ae44504c1eb.

The DOSBox-X normal-core/386 fixture shrinks its synthetic program block
to 64 KiB and allocates a separate 20 KiB code block. It loads the pinned
caller, callee and incoming +2469..2474 prefix at their original offsets
there. Keeping code separate is necessary to supply arbitrary DS:408E
table values without overwriting the +408C instructions. Relative calls
are unchanged. The original prefix returns to +2475, where a transparent
synthetic far jump reaches capture. That unique landing records the
allocated code CS and original IP while preserving registers/flags.

Four additional synthetic direct far calls pass distinct argument and
active records; their separate capture landing records the actual program
return CS:IP. One verifies AX=016E after signed division, preserving AH.
These are explicit stress inputs, not claims about the earlier game caller.
Earlier caller conditions and natural saved-game reach remain unmeasured.

Native cases compare all 14 CPU words, 64 frame bytes, 256 DS table bytes
and 1536 record-region bytes, covering all function writes in these fixtures.
Real INT 0 faults capture hardware IP/CS/FLAGS and reconstruct pre-vector
SP. Three already-popped callee-frame words overwritten by exception entry
are restored to their known values: code CS, active-record offset and
data segment. Fault RAM is thus normalized to the C pre-vector boundary;
the game's own exception handler is outside this model. Separate scratch
stack capture prevents PUSHF changing compared RAM. IRQs stay disabled
through setup/execution/capture and are enabled for DOS I/O. INT 0 is
restored before exit. Only successful IDIV arithmetic flags are masked.

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6c_record_weighted_408c_audit.py --output resources/u6c-408c-parity/differential.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6c_record_weighted_408c_native.py

All media, code images, original caller/body bytes, assembly, COM, synthetic
case files, disassembly, captures and logs stay ignored under resources/.
The native run has a 90-second bound. Header allocation segments make its
checksum environment-dependent even when parity passes. Audit dependencies
are Capstone 5.0.7 and Unicorn 2.1.4.

The focused strict C test checks guards, weighted overflow, independent
records, positive/negative division, clamp flags, zero divisor, full AX
quotient and CL/DX/SI effects at an altered return. The initial strict
warnings build and 246 CTests pass; the expanded focused test also passes.
Research hashes, oracle shell syntax, tracked-media exclusion and diff
whitespace checks pass.

## Review, reuse and queued work

target_function: OVL_006C_408C. source_candidates: the new caller composed
with merged pr_u6c_record_table_sum_3f31. signals: pinned complete source/
dependency/CFG; four aligned active-character callers; full-state site/
feasible-edge parity with separate records and boundaries; original-caller
native parity and distinct-record/fault captures. score: 4/4 listed checks,
a checklist rather than a probability. verdict: match under immutable-code,
A20-off, entry-frame and explicit boundary contracts. review_notes:
undefined IDIV flags, infeasible default edge, native return landing and
fault-frame normalization are explicit; field meanings/natural reach remain
unmeasured; PM independently reviews identity and strict credit.

A bounded 15-second deja lookup yielded no completed finding. Reused
findings are the merged +3F31 model/oracle, +3FB3 arithmetic and architectural
boundary helpers, active-pointer mapping and IRQ-isolated native method.
All copied helpers are from the reviewed +3FB3 head; this branch has no
dependency on its still-unpublished caller files. The only source dependency
is the already merged +3F31. The helper source is reviewed head
010074dc27f6331acc0fd0b4b326d3e23800a618. Extent, four incoming calls
and every parity count were remeasured.

PM prioritized disjoint +4235 next, isolating the raw dice/RNG interface:
184 bytes/73 instructions, SHA-256
fdb467838a6d2067178154c2b0bf3d60f0acff37060f337977e3d66d24391d60,
with sole +4280 -> B0:0048 (merged U9E entry 8) dependency. Two incoming
sites currently rechecked are +056C:+1D3B and +29AE:+2FB1; the next caller
inventory must be remeasured before further claims. +4454 is the
spare five-byte empty entry marker, with no implementation/strict/open-PR
collision found and no incoming caller; it remains uncredited. Neither
follow-on is implemented or credited here.
