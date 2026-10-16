# Slot-A U1C +33EA second key caller jump

PM inbox i5922 assigned this caller JMP after the source's +33E7
near CALL; the C gate does not require a +33E7 model. Targeted deja
recall was attempted but the indexed
lookup stalled; no prior session evidence is reused. The supplied
CPU interface follows reviewed `overlay_u1c_key_jne_33e4.c`.

DOS v1.3 GAME.OVR file 0x4130..0x4131 and saved loaded RAM physical
0x38C0A..0x38C0B both contain `EB 40`, SHA-256
`081b3ef897938b3abd1047243d7466dffdd346bc2f81b28e925f18898f66ab8c`.
This short JMP uses next IP 33ECh plus 40h to target +342C.
The audit pins complete GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
and earlier +33BD RAM SHA-256
`38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3`.
That RAM pins loaded code, not a native +33EA return stop.

`pr_rec_u1c_33ea_jmp_342c` accepts independently supplied CPU at
3582:33EA after the +2DD3 callee return and complete 640 KiB or
1 MiB RAM. It requires the supplied A20-masked code bytes at CS:IP
to equal `EB 40`, checking both addresses before any CPU write. It
sets IP to +342C, preserving all other CPU and RAM state. It does not
execute or infer +2DD3 effects. Tests compare all 14 CPU words and
complete RAM for both sizes, varied FLAGS/SP, each wrong opcode byte,
short code RAM, and atomic rejection. Strict C11, ASan/UBSan, source audit, and
CTest registration are local checks.

## Parity review

- **target_function:** U1C +33EA..+33EB, stopping at +342C.
- **source_candidates:** `pr_rec_u1c_33ea_jmp_342c`.
- **signals:** exact original and loaded bytes, reviewed CPU state
  interface, complete-state synthetic caller-return tests.
- **score and verdict:** 2/5 direct signal groups; **source-only
  conditional match** pending a native +33EA return stop.
- **review_notes:** no +2DD3 callee effects, native return trace,
  full-function or gameplay credit is claimed.
