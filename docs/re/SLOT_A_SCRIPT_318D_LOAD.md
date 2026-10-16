# Slot-A U1C +318D opcode-latch byte load

PM inbox i5922 assigned this load after the source's +318B
fallthrough. The gate starts from an explicit supplied ZF-set CPU;
it does not execute +3186 or +318B. Targeted deja recall was attempted
but its index lock was unavailable; no prior session evidence is reused. The byte-load
and A20 preflight reuse reviewed `overlay_u1c_key_load_33d9.c`.
The high-level ECL runner in `ecl_helpers.c` records the broader
`+318D..+3190` assignment but does not own this CPU instruction gate.

DOS v1.3 GAME.OVR file 0x3ED3..0x3ED5 and saved loaded RAM physical
0x389AD..0x389AF both contain `A0 7F 6F`, SHA-256
`0ac68aa8f8e31bed0b7f260b68fa14399650912de2c06a68c496056f28908bac`.
The audit pins complete GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
and earlier +33BD RAM SHA-256
`38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3`.
That RAM pins loaded code, not a native +318D stop.

`pr_rec_u1c_318d_load_latch_al` accepts supplied first-visit,
ZF-set CPU at 3582:318D and complete 640 KiB or 1 MiB RAM. It
requires supplied A20-masked code bytes at CS:IP to equal `A0 7F 6F`,
checking all three addresses before any CPU write. It preflights
DS:[6F7F] under A20-off 20-bit masking, loads AL while
preserving AH/FLAGS/all other registers and RAM, then stops at +3190.
Tests start from explicit synthetic ZF-set CPU at +318D and
compare all 14 CPU words and complete RAM for both sizes, zero/nonzero
latch values, DS A20 wrap, each wrong opcode byte, short code RAM,
and atomic rejection. Strict C11,
ASan/UBSan, source audit, and CTest registration are local checks.

## Parity review

- **target_function:** U1C +318D..+318F, stopping at +3190.
- **source_candidates:** `pr_rec_u1c_318d_load_latch_al`.
- **signals:** exact original and loaded bytes, reviewed byte-load
  model, complete-state synthetic supplied-CPU tests.
- **score and verdict:** 2/5 direct signal groups; **source-only
  conditional match** pending a native +318D stop.
- **review_notes:** no natural +3171 reach, successor effects,
  full-function or gameplay credit is claimed.
