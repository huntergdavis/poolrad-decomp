# Unit 6C +3F31 record/table byte sum

## Pinned extent and callers

The complete call-free leaf is U6C:+3F31..3FB2 inclusive: 130 bytes,
44 contiguous instructions, GAME.OVR file [121008,121138), SHA-256
9f192cb8fe704324facbbec368d1e24602558b878aa3f7e0031f644862e09767.
All six conditional branches and the unconditional jump remain inside
the span; RETF 4 is at +3FB0. There are no calls, interrupts or fixups.
Opcode 98 at +3F50 is CBW in 16-bit mode, despite the decoder's CWDE label.

Pinned owner inputs are START.EXE (65231 bytes), SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd,
and GAME.OVR (206105 bytes), SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
Unit 6C has file base 104831 and original stub segment 0078.

The source audit verifies four aligned PUSH CS / near CALL pairs from
their caller starts: +056C:+1D5B/+1D5C, +29AE:+2FE7/+2FE8,
+3FB3:+4017/+4018 and +408C:+41C0/+41C1. The +29AE call explicitly
pushes DS:[5D94] and DS:[5D92]; +408C uses the same active-record pointer.
Its independently verified native mapping is documented in
[U6F party insertion](U6F_PARTY_INSERT_1446.md), while the surrounding
record/list use is documented in [U6A +025C](U6A_RECORD_REMAINING_025C.md).
Three callers are canonically game_evidenced; +3FB3 remains unresolved.
The byte fields +14/+2F/+96..9D and DS table values are not assigned
gameplay names. This leaf remains helper_candidate pending PM review.

## Complete raw C contract

pr_u6c_record_table_sum_3f31 enters after CALL with a far record argument
at SS:SP+4. It saves BP, reserves eight bytes, initializes total [BP-2],
copies the far record pointer to words [BP-8]/[BP-6], and sets index
[BP-3] to zero. [BP-4] is unused. For indices 0..7:

1. Read the local index, CBW, reload the saved far pointer and test signed
   byte record[96+index]. Only 01..7F enter the body; 00 and 80..FF skip.
2. Reload the pointer, read unsigned selector byte record[14], zero AH,
   and read DS:[408E+selector]. Add that byte to the live local total,
   wrapping at eight bits.
3. Reload the pointer and selector. If selector >10 unsigned, reload
   again and increment total when record[2F] equals 2.
4. Repeat the independent selector >11 and record[2F]==2 gate, reloading
   the pointer at both original sites. Increment the index between iterations.

After the last index CMP against 7, copy total through [BP-1] to AL.
AH is zero, so AX returns the byte total. BX/CX/DX/SI/DS/SS are preserved.
ES:DI retains the last loaded record plus 7 if the final signed gate
skips, or the last record base if it enters. The last CMP defines every
arithmetic flag: PF/ZF set, CF/AF/SF/OF clear. All other FLAGS bits are
preserved, and no later instruction changes flags. No flag mask is used.

Locals, record fields, DS table and frames share 1 MiB RAM. The model
retains separate pointer-word stores and every LES reload; it does not
cache the record pointer or selector across total updates. Aliases can
change later reads, for example when selector or table data is the local
total. Effective offsets/SP wrap at 16 bits. Multi-byte operands and the
four-byte far return use contiguous physical bytes with A20 off; separate
word stores still compute their own wrapped offsets. Boundary cases include
the saved local far pointer crossing offset FFFF. Code remains immutable.
Actual RAM supplies restored BP and CS:IP. MOV SP,BP / RETF 4 advance
entry SP by eight bytes overall. There are no callbacks, original-byte
inputs or host loop caps. Null host pointers or short RAM are rejected
before mutation.

## Measured gates and reproduction

The original-byte/C audit passed **2522 cases**, comparing all 1048576
RAM bytes and all 14 CPU words. All 44 instruction starts and both
outcomes of all six conditional branches (12 edges) are covered.
Cases include every selector/table/signed-gate byte, all 256 eight-position
enable patterns, threshold and overflow paths, record/table/frame aliases,
offset/A20 wrapping, and 128 seeded whole-RAM backgrounds with varied flags.

The DOSBox-X normal-core/386 oracle passed **1282 cases**. It copies
the exact +2FDF..2FEA argument pushes, PUSH CS and near CALL at their
original offsets; the original relative operand and +3F31 leaf are unchanged.
Synthetic setup supplies the active-record pointer. Earlier caller
conditions and a naturally reached game route remain unmeasured.

Each native case compares all 14 CPU words, 64 frame bytes, 256 DS table
bytes and 288 record-region bytes, covering all writes in these fixtures.
Cases include selector/qualifier, table/gate bytes, all enable patterns,
and two dynamic aliases: table/total returns AX=254 and selector/total
returns AX=30. The table alias's frame lies inside the compared DS table.
Capture records SP before moving it to a scratch frame for PUSHF, so flag
capture cannot alter the table/return frame being compared. IRQs remain
disabled through setup/execution/capture, then enabled for DOS I/O.
Every flag including AF is checked. Native output SHA-256:
fc3eb5b002faabd2e84914ae724e626940d232395f226056603438c9369b9cbb.
The header includes the DOS allocation segment, so another environment
may change this hash while preserving parity.

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6c_record_table_sum_3f31_audit.py --output resources/u6c-3f31-parity/differential.json
    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u6c_record_table_sum_3f31_native.py

The focused strict C test checks guards, signed gates, both increment
thresholds, qualifier mismatch, overflow, final registers/FLAGS and locals,
unused-byte preservation and a table/total alias. The initial strict build
and 240 CTests pass. Research hashes, oracle shell syntax, tracked-media
exclusion and whitespace checks pass. All copied bodies/caller bytes,
assembly, COM, disassembly, native output and logs stay under ignored
resources/. Audit dependencies are Capstone 5.0.7 and Unicorn 2.1.4.

## Review and follow-on audit

target_function: OVL_006C_3F31. source_candidates: the new raw C model.
signals: pinned extent/ABI/CFG; four aligned callers and established
active-character pointer; complete site/edge full-state differential;
original-caller native parity including dynamic selector/table aliases.
score: 4/4 listed checks pass, a checklist rather than a probability.
verdict: match under the immutable-code, A20-off, entry-frame contract.
review_notes: field meanings and natural game reach remain unmeasured;
PM's independent exact-head review decides game identity and strict credit.
No identity, classifier or accounting file changes here.

A bounded 15-second deja lookup yielded no completed finding. Reused
findings are the cited active-record mapping, U6A raw alias/CBW conventions,
U86 CPU/RAM audit utilities and IRQ-isolated native caller method. Exact
source, all four callers, constants and every parity count were remeasured.
Initial branch base: 739d394973f802c3828fd7faf28d56b0ec665218.

The fresh address-only queue has no further substantive call-free U6C
row after this target: +4454 is a five-byte empty entry marker with no
incoming sites and remains uncredited. PM selected +3FB3 after this PR's
review: 217 bytes/88 instructions, sole call +4018 -> +3F31, SHA-256
bf1e24f3fbf0660c08259b756b918b0685b1488c3c530599e55e80778ebd397f.
It is a caller with one dependency, not a call-free leaf; its game role
still needs review. +408C is the second disjoint follow-on if unclaimed.
Neither follow-on is implemented or credited in this PR.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_006C_3F31 --require-current-main
```
