# Unit 6C +3FB3 complete record/count caller

## Exact source and game-role evidence

The complete body is U6C:+3FB3..408B inclusive, GAME.OVR file
[121138,121355): 217 bytes, 88 contiguous instructions, SHA-256
bf1e24f3fbf0660c08259b756b918b0685b1488c3c530599e55e80778ebd397f.
Its sole call is PUSH CS / near CALL at +4017/+4018 to the merged
[U6C +3F31 leaf](U6C_RECORD_TABLE_SUM_3F31.md). All branch targets remain
inside the body; RETF 4 is at +4089. There are no fixups or INT instructions.
The callee is 130 bytes/44 instructions; composed coverage is 132 sites.
Opcode 98 is CBW and 99 is CWD in 16-bit mode, despite decoder labels.

Pinned inputs are START.EXE (65231 bytes), SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd,
and GAME.OVR (206105 bytes), SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
The unit file base is 104831, original stub segment 0078.

Four aligned incoming calls are +2531/+2546/+2786/+279B in +209F.
Each explicitly pushes DS:[5D94]/[5D92], the active-character pointer
independently established by [U6F insertion](U6F_PARTY_INSERT_1446.md).
The surrounding caller compares AL with, or stores it to, record byte +32.
This establishes a character-state use without naming that field, the
eight bytes +96..9D, selector +14, qualifier +2F or the calculated value.
The canonical caller is game_evidenced; this function remains unresolved
pending independent identity review. No identity or accounting file changes.

## Complete C composition and ordered behavior

pr_u6c_record_tier_3fb3 shares the merged callee's 14-word raw CPU layout.
After CALL it accepts a far record at SS:SP+4, saves BP and reserves ten
bytes. It copies the record to [BP-9]/[BP-7], initializes count [BP-2],
sum [BP-4] and index [BP-5], then loops over indices 0..7. Signed-positive
record[96+index] bytes are added to the byte sum with wrapping, and count
increments. Enabled indices 4 and 7 each add one extra to that sum.
Each pointer/index/record-byte reload occurs at its original source site.

The caller pushes the live far argument, CS and exact near-return IP 401B,
then invokes the complete merged +3F31 C model. This writes a real shared-RAM
callee frame and locals. No callback or invented result substitutes for it.
The returned byte is stored at [BP-3] and interpreted as signed:

- Nonnegative adjustment: signed divide (sum+adjustment) by count.
- Negative adjustment: compare sum to abs(adjustment)+count. If sum is
  no greater, store 1; otherwise signed divide (sum+adjustment) by count.

The quotient's AL is copied through the local sum/result; AX, CX and DX
retain the actual original quotient, divisor and remainder or clamp-state
values. BX/SI/DS/SS remain preserved; ES:DI is the callee's returned pointer.
The actual saved BP and far return come from RAM, with RETF 4 advancing
entry SP by eight bytes overall. The API needs no original instruction
bytes, has no host loop cap and rejects invalid host pointers/short RAM
before mutation. Locals, arguments, record/table data and both call frames
share A20-off 1 MiB RAM; offsets/SP wrap at 16 bits, wide operands use
contiguous physical bytes, and code remains immutable.

Zero count can fault at IDIV +4078; the API reports DIVIDE_ERROR before
executing the CPU vector transition, preserving IP, registers, flags and
all RAM at that boundary. A quotient overflow would use the same status.
TRANSFER reports exact state if the internal far return leaves CS:401B,
including real stack-offset-wrap cases. It does not fabricate continuation.
These reuse the repository's architectural-boundary conventions from
the [U9B caller](U9B_ENTRY_ZERO_COMPOSITION.md) and existing divide-error APIs.

Clamp exits end with a defined word CMP; fault boundaries end with the
defined pre-IDIV ADD. Every flag is compared on those paths. IDIV leaves
all six arithmetic flags (08D5) undefined; only those bits are masked on
successful division exits. The C chooses the preceding ADD's arithmetic
flags. Non-arithmetic FLAGS remain exact on every path.

## Measured gates and reproduction

The original-byte/C audit passed 3088 cases, comparing the entire 1 MiB
RAM and all 14 CPU words. It covered all 88 caller plus 44 callee sites,
and all 24 conditional edges across both bodies. Outcomes: 2946 returns
(2656 divisions, 290 clamps), 136 divide errors and six altered internal
returns. Cases cover byte/selector/gate boundaries, enable patterns, record/
table/local/frame aliases, offset/A20 wrapping and 128 seeded whole-RAM
backgrounds. Faults stop before IDIV vectors; transfers stop before the
first instruction at the unexpected target. All defined flags match.

The DOSBox-X normal-core/386 oracle passed 1538 cases: 1536 returns
(1400 divisions, 136 clamps) and two real divide faults. It executes the
exact original +2528..2533 caller suffix at its original offset, then
both pinned original bodies and their original internal CALL. Synthetic
setup supplies the active-character pointer, table and record. It does
not execute earlier game conditions or prove natural game reachability.

Each case compares all 14 CPU words, 64 frame bytes, 256 DS table bytes
and 288 record bytes, covering all function writes for these fixtures.
The INT 0 handler captures fault IP/CS/FLAGS from the hardware frame and
reconstructs the pre-vector SP. CPU exception entry overwrites three
already-popped callee-frame words; the fixture restores their known actual
values (caller CS, record offset, record segment) before RAM comparison.
Thus fault RAM is normalized to the documented C pre-vector boundary;
the game's own exception handler is not translated or claimed. Normal
capture uses a separate scratch stack so PUSHF cannot alter compared RAM.
The fixture restores INT 0 before exit. IRQs remain disabled through
execution/capture and enabled for DOS I/O. Only successful IDIV arithmetic
flags are masked; clamp and fault flags compare every bit.

Native SHA-256:
b3fb207324dbc8755d0ed6cbc5207e8bca511e9c21504173839daf305d3b5cad.
The header includes the DOS allocation segment, so another environment
can change the hash while preserving parity. The native run has a bounded
90-second timeout. Original media, bodies/caller bytes, assembly, COM,
synthetic case files, disassembly, captures and logs stay ignored in resources/.

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6c_record_tier_3fb3_audit.py --output resources/u6c-3fb3-parity/differential.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6c_record_tier_3fb3_native.py

The focused strict C test checks guards, quotient/remainder, positive and
negative adjustments, the clamp's exact flags, zero division and an actual
wrapped internal return. The strict warnings build and initial 244 CTests
pass. Capstone 5.0.7 and Unicorn 2.1.4 are used by the source audit.

## Review and reuse

target_function: OVL_006C_3FB3. source_candidates: the new caller composed
with merged pr_u6c_record_table_sum_3f31. signals: pinned complete extent/
dependency/CFG; four active-character callers and record+32 use; complete
site/edge full-state parity including architectural boundaries; native
original-caller composition with real faults. score: 4/4 listed checks
pass, a checklist rather than a probability. verdict: match under the
immutable-code, A20-off, entry-frame and explicit boundary contracts.
review_notes: gameplay field meanings and natural game reach remain
unmeasured; divide flags and fault-frame normalization are explicit;
PM decides game identity and strict credit after exact-head review.

A bounded 15-second deja lookup yielded no completed finding. Reused
findings are the merged +3F31 model/oracle, U6F active-pointer mapping,
U9B altered-return convention, existing divide-error APIs and IRQ-isolated
raw-state method. Source, incoming callers and all counts were remeasured.
The separate reserved follow-on is +408C, after this PR's review. No
follow-on source, identity or credit is added here; +4454 remains uncredited.
