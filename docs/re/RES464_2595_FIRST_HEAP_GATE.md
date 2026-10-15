# Resident 0464:2595: original first-heap-call gate

This is the second measured follow-on to
[the U3CE zero/first-call gate](U3CE_014E_ZERO_FIRST_GATE.md). It measures
original entry/argument boundaries, without executing SYSTEM:0254 or adding
a C translation, heap semantics, native reach or strict credit. The existing
[bitmap-release C wrapper](RESIDENT_464_BITMAP_RELEASE.md) already implements
its data operations using host memory/free callbacks; that dependency stays open.

Pinned START.EXE SHA-256 is
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
The hash-locked unpacker places the body at image [0x06BD5,0x06C72),
relative [+2595,+2632): **157 bytes / 52 CFG starts**, SHA-256
d3cd5846fe2109fc1eb33b500561830c73601d21bea64575f7a08488301eb700.
All three SYSTEM:0254 call segments have image relocation entries.

Entry has a far argument pointing to the caller's object-pointer slot.
The body pushes BP and reserves SIX local bytes: a saved object far pointer
and the truncated size product. A nil slot jumps directly to the epilogue
and RETF 4. A nonzero slot supplies header byte +8 and word +11; the size
product is truncated to sixteen bits before any shift or addition.

| First unresolved boundary | Condition | Size at SS:[SP] |
| --- | --- | --- |
| +25F0 CALL SYSTEM:0254 | auxiliary pointer nonzero, DS:4A1E = 1 | product shifted right twice |
| +2605 CALL SYSTEM:0254 | auxiliary pointer nonzero, other mode | product |
| +261B CALL SYSTEM:0254 | auxiliary pointer zero | product + 23, modulo 65536 |

The pre-call frame is **size, block offset, block segment** at SP, SP+2,
SP+4. Auxiliary calls use header fields +13/+15. The primary call rereads
the caller slot's segment, then offset, before pushing the size. No return
frame for SYSTEM is pushed by the probe. The null route separately executes
+262F RETF 4 and checks returned CS:IP, SP = entry SP + 8, and restored BP.

**148 original-byte probes pass**: modes 0/1/2/255, header plane counts
0/1/255, strides 0/1/65535, and zero/offset-only/segment-only/full auxiliary
pointers; plus four nil primary-pointer cases. The measured stops are 27 at +25F0, 81 at +2605, 36 at +261B and four
nil returns. Thus every first-call site and the nil return are measured. Each nonzero probe validates exact
stack arguments and restricts memory writes to the procedure's SS stack.
The probes use separate table and stack storage; stack aliases can still
alter table bytes in other inputs. No flags or full-RAM C parity is claimed
for this resident classifier.

Post-free calls, slot clearing, allocator effects and successful callback
returns are outside the measured boundary. The +017F U3CE follow-on uses
independently supplied caller state; it does not stand in for these effects.

    PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u3ce_gate_014e_audit.py --source-only

Original bodies, disassembly and RAM stay ignored under resources/.
This note contains only source identities, addresses, counts and numeric ABI.
