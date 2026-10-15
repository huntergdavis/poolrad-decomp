# U9E +0160 sidecar lookup setup: bounded raw-state prefix

`pr_u9e_sidecar_prefix_0160` translates only `GAME.OVR` unit `0x9E`
`+0160..+018A`, stopping at `CS:018B` before the resident sidecar lookup
`lcall 00BA:2199`. It takes a complete 1 MiB A20-masked RAM image and the
14-word CPU state; it returns with the original frame and five lookup
argument words still on the stack. This is a bounded prefix, with no claim
for the external sidecar effect, the `+018B` continuation, the full 410-byte
function, native dispatch reachability, or strict full-C credit.

## Recall and provenance

`deja "Poolrad U9E +0160 external effect boundary first call native parity prefix"`
timed out locally. Reused [U9E_0160.md](U9E_0160.md) for the full extent,
ABI, and earlier bounded high-level model; [U9E_ENTRY2.md](U9E_ENTRY2.md)
for the neighboring entry and `+0140` set constant; and
[overlay_function_ledger.csv](overlay_function_ledger.csv) for the grade-B
internal classification. The raw bytes and callers below were independently
checked against the ignored local executable.

| Item | Pinned evidence |
| --- | --- |
| `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Full routine | file `[0x2B04A,0x2B1E4)`, `+0160..+02F9`; 410 bytes, 140 instructions; SHA-256 `89a45482f4f42e1f4805cc4aa2448ebb685b777dbc7b91be985da4473bb208ba`; next prologue `+02FA` |
| This prefix | file `[0x2B04A,0x2B075)`, `+0160..+018A`; 43 bytes, 15 instructions; SHA-256 `55f45b6b7f6389eeb943ff7cd049e4168dd6f0232b59801c581bfe00c9fc9517` |
| Stop | the next original bytes at file `0x2B075` are `9A 99 21 BA 00`, `lcall 00BA:2199` at `+018B`; this instruction does not execute |
| Callers | unit `0x9E` public entry 3 `+02FA` contains 140 instruction-aligned `E8` near calls targeting `+0160`, first `+0311`, last `+0AB2`; each is preceded by `push cs`. The ledger likewise records 140 near calls and no public entry slot for `+0160` |

At entry the far return frame is at `SS:SP`; Pascal arguments are type byte
at `[BP+6]`, character offset at `[BP+8]`, and character segment at
`[BP+0A]` after `push bp; mov bp,sp`. The prefix reserves `0xE4` stack
bytes, clears `[BP-E1]`, copies the far party cursor from
`DS:[5D96]:[5D98]` to `[BP-DC]:[BP-DA]`, and prepares lookup arguments:
character segment, character offset, a word whose low byte is type and high
byte is the preserved `AH` from `DS:[5D96]`, `SS`, and `BP-E0`.
`SP` arithmetic wraps at 16 bits; memory words use contiguous physical bytes
with a 1 MiB A20 mask. All registers and flags except the instruction's
documented effects are retained. The model leaves `IP=018B` and `CS` intact.

The merged `overlay_u9e_0160.c` is a higher-level behavior model of the
whole function with host hooks. This separate raw-state slice proves the
opening setup against original bytes and does not extend that model's
behavioral claims past the first external call.

## Verification

`tests/recovered/check_overlay_u9e_sidecar_prefix_0160.py` verifies the
input SHA, instruction boundary, and next far-call opcode, then executes
the original bytes in 16-bit Unicorn. In 72 cases spanning distinct and
aliased `DS`/`SS`, stack wrap, zero-result stack subtraction, `AX`, flags,
and type values, it stops before
`+018B` and compares all 1 MiB RAM bytes and all 14 CPU words against the
C function. `tests/recovered/test_overlay_u9e_sidecar_prefix_0160.c`
checks a hand-pinned frame and argument rejection. The focused C target is
also built with C11, `-Wall -Wextra -Werror`, ASan and UBSan.

These are synthetic entry frames. A natural reached frame and the resident
lookup's effects are separate evidence gates.
