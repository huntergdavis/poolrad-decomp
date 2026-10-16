# Slot-A U1C +3193 script word load

PM inbox i6874 approved a separate, bounded source gate after the merged
+3190 store. A targeted `deja` query found no prior +3193 session match.
The merged `overlay_u1c_script_store_al_3190.c` supplies the explicit CPU
and A20 preflight pattern; the broader high-level script runner in
`ecl_helpers.c` does not own this instruction-level CPU gate.

DOS v1.3 GAME.OVR file 0x3ED9..0x3EDB and saved loaded RAM physical
0x389B3..0x389B5 both contain `A1 ED 49`, SHA-256
`a3a6c864923d7041eee399509b2db45372283dbeeba687188aa23cd14e817e4d`.
The audit pins complete GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`
and saved +33BD RAM SHA-256
`38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3`.
That RAM pins loaded code, not a native +3193 stop.

`pr_rec_u1c_3193_load_script_ax` accepts explicit first-visit, ZF-set
CPU at 3582:3193 and complete 640 KiB or 1 MiB RAM. It checks supplied
A20-masked opcode bytes and both bytes of DS:[49ED], loads little-endian
AX, and stops at +3196. All other CPU words and complete RAM are
preserved. Rejected inputs leave CPU and RAM unchanged. Focused tests
compare all 14 CPU words and complete RAM for both sizes, word values,
DS A20 wrap, each wrong opcode byte, short RAM, and bad flag/address/null
inputs. One synthetic test composes the merged +318D load and +3190
store with this load; it does not establish natural reach from +3171.
Strict C11, ASan/UBSan, source audit, and CTest registration are local
checks. No native or full build is used.

## Parity review

- **target_function:** U1C +3193..+3195, stopping at +3196.
- **source_candidates:** `pr_rec_u1c_3193_load_script_ax`.
- **signals:** exact original/loaded bytes, merged adjacent gate interfaces,
  complete-state synthetic direct and composed tests.
- **score and verdict:** 2/5 direct signal groups; **source-only
  conditional match** pending a native +3193 stop.
- **review_notes:** no natural +3171 reach, +3196 successor effects,
  full-function, or gameplay credit is claimed.
