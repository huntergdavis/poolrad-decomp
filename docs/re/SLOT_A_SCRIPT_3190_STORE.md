# Slot-A U1C +3190 opcode-latch byte store

PM inbox i6828 assigned a bounded +3190 store after the merged +318D
byte load. A targeted `deja` query could not open its read-only index
lock, so no prior session evidence is reused. The merged
`overlay_u1c_script_load_al_318d.c` supplies the CPU and A20 preflight
pattern. The broader high-level assignment appears in
`ecl_helpers.c` at +318D..+3190, but that model does not own this
instruction-level CPU gate.

DOS v1.3 GAME.OVR file 0x3ED6..0x3ED8 and saved loaded RAM physical
0x389B0..0x389B2 both contain `A2 7E 6F`, SHA-256
`3f8a2194d247e6485a293ab7baea2dfc9716fc1584ee37e2f50ec5d19a137e87`.
The audit pins complete GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
and saved +33BD RAM SHA-256
`38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3`.
That RAM pins loaded code, not a native +3190 stop.

`pr_rec_u1c_3190_store_latch_al` accepts explicit first-visit,
ZF-set CPU at 3582:3190 and complete 640 KiB or 1 MiB RAM. It checks
the supplied A20-masked code bytes, preflights DS:[6F7E], writes AL
to that byte, and stops at +3193. All other RAM bytes and all CPU
words except IP are preserved. Rejected inputs leave CPU and RAM
unchanged. Focused tests compare all 14 CPU words and complete RAM for
both sizes, zero/nonzero AL, DS A20 wrap, each wrong opcode byte,
short RAM, and bad flag/address/null inputs. One synthetic test composes
the merged +318D load with this store; it does not establish natural
reach from +3171. Strict C11, ASan/UBSan, source audit, and CTest
registration are local checks. No native or full build is used.

## Parity review

- **target_function:** U1C +3190..+3192, stopping at +3193.
- **source_candidates:** `pr_rec_u1c_3190_store_latch_al`.
- **signals:** exact original/loaded bytes, merged +318D interface,
  complete-state synthetic direct and composed tests.
- **score and verdict:** 2/5 direct signal groups; **source-only
  conditional match** pending a native +3190 stop.
- **review_notes:** no natural +3171 reach, +3193 successor effects,
  full-function, or gameplay credit is claimed.
