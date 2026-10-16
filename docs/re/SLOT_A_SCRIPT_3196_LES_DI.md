# Slot-A U1C +3196 script far-pointer load

PM inbox i7164 assigned the next bounded first-visit successor after
merged +3193. A targeted `deja` query was run before implementation;
no prior-session result is used. The merged +3193 CPU gate and the
checked-in U7F +05E0 LES model supply the explicit state and contiguous
far-pointer read patterns. The broad high-level script runner in
`ecl_helpers.c` at +3193..+319C does not own this instruction-level gate.

DOS v1.3 GAME.OVR file 0x3EDC..0x3EDF and saved loaded RAM physical
0x389B6..0x389B9 both contain `C4 3E DE 49`, SHA-256
`759845587fe686131425f382f9450fa52081a01a49a025d4561d0402247c93cc`.
The audit pins complete GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
and saved +33BD RAM SHA-256
`38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3`.
`objdump -m i8086` decodes the four bytes at +3196 as
`LES DI,DWORD PTR DS:[49DE]`. Saved RAM pins loaded code, not a native
+3196 stop.

`pr_rec_u1c_3196_les_script_pointer` accepts explicit first-visit,
ZF-set CPU at 3582:3196 and complete 640 KiB or 1 MiB RAM. It checks
supplied A20-masked opcode bytes, preflights all four bytes of the
DS:[49DE] far pointer, loads the offset into DI and segment into ES,
and stops at +319A. All other CPU words and complete RAM are preserved.
Rejected inputs leave CPU and RAM unchanged. Focused tests compare all
14 CPU words and complete RAM for both sizes, A20 wrap, pointer values,
an operand crossing the 640 KiB boundary, each wrong opcode byte,
bad flag/address/size/null inputs, and synthetic composition with the
merged +318D, +3190, and +3193 gates. This composition does not prove
natural +3171 reach. Strict C11, ASan/UBSan, source audit, and CTest
registration are local checks; no native or full build is used.

## Parity review

- **target_function:** U1C +3196..+3199, stopping at +319A.
- **source_candidates:** `pr_rec_u1c_3196_les_script_pointer`.
- **signals:** exact original/loaded bytes, i8086 decode, merged
  adjacent-gate CPU contract, complete-state synthetic direct/composed tests.
- **score and verdict:** 2/5 direct signal groups; **source-only
  conditional match** pending a native +3196 stop.
- **review_notes:** no natural +3171 reach, +319A successor effects,
  full-function, or gameplay credit is claimed.
