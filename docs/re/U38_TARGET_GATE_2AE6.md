# Unit 0x38 public entry 100: target gate at +0x2AE6

## Provenance and gate

This candidate composes the merged
[+0x101D target selector](U38_TARGET_SELECT_101D.md), corrected helper
head `8ee237f0c7bf791c40345b4e15acda5d1501d623` (PR #470,
merged at `23f18d7`). The initial
`deja "poolrad U38 +2AE6 entry100 helper +101D parity"` query spent
several minutes rebuilding its index and returned no session result
before it was stopped. The candidate reuses the helper's checked
[resident selector](RESIDENT_CHARACTER_TARGET_SELECT.md) and
[character record context](../formats/DOS_CHARACTER.md); the original
bytes and behavior below were independently verified.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The complete public entry is unit `0x38:[+0x2AE6,+0x2B20)`,
`GAME.OVR` file `[0x11741,0x1177B)`: 58 contiguous bytes / 22
instructions, SHA-256
`eb6cda0aa8f426f8c77a9fc23394b44f251dd3e395e5b4f378f4f511ea31a36a`.
The next public entry starts at `+0x2B20`. The only call is a near call
at `+0x2AF5` to `+0x101D`; this entry has no loader fixups.

## Behavior and C boundary

The entry passes the active character far pointer from `DS:5D92/5D94`
to `+0x101D`. If that helper returns a null far pointer, it leaves
`DS:6818` alone. Otherwise it reads byte `+0x32` from the returned
target. A positive signed byte causes `DS:6818` to be halved as an
unsigned byte by `SHR AX,1`; zero or negative values leave it alone.
The routine takes five caller words, does not inspect them, and exits
with `RETF 10`.

`pr_u38_target_gate_2ae6` models all 58 original bytes in 1 MiB RAM
and the 14-field CPU state. It writes the original near-call return
frame and local words, composes the provisional +0x101D C model,
preserves its CPU/RAM effects, and models `SUB`, `OR`, `CMP`, `XOR`,
`SHR`, and `RETF 10` FLAGS/stack behavior. The callback boundary is
the helper's unresolved loaded-segment resident call at `:2E8B`;
callback refusal returns `PR_U38_2AE6_CALL_ABORT` with the call frame
still visible.

## Verification and limits

The original-byte Unicorn checker executes the complete +0x2AE6 and
+0x101D bodies, including the actual near and far calls. Its 240
cases compare the entire 1 MiB RAM, all 14 CPU fields, and each
six-word resident callback frame. They vary helper selection, target
byte sign, amount, loader fixup segment, DS/SS alias, and stack wrap.
The focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest
passes null, positive, zero, negative, and aborted selector cases.

Native DOSBox-X synthetic far-entry evidence is recorded by
`tests/recovered/native_overlay_u38_target_gate_2ae6.py` under ignored
`resources/u38-2ae6-parity/native/`. DOSBox-X sets AF after the
positive `5 >> 1` `SHR` case while Unicorn and the C model clear it;
AF is undefined for this instruction. The native comparison requires
every other FLAGS bit, all other CPU words, and the complete stack
frame to match across four cases and 52 frame bytes. Capture SHA-256:
`6d5791241c76b5ca83c538c1a52d619027900fdd6d4a67d3a9b8c69d1f4c8877`.
Its result is not a reached
gameplay frame. The +0x101D helper's game-versus-library ownership
classification remains an integration gate. No strict identity or
count credit is claimed for this entry.

## Pinned AF after SHR: plain model fixed (2026-10-23, census #1464 family 2)

The plain (non-`_cpu`) model ran `SHR AX,1` through `logic16`, which clears AF; pinned DOSBox-X sets AF after a non-zero shift (`get_AF` = `count & 1Fh`, `flags.cpp:172-177`; stored `763-771`). The `_cpu` variant already had both behaviours behind an explicit `shift_profile` (`PR_TARGET_SHIFT_UNICORN` AF=0, `PR_TARGET_SHIFT_DOSBOX386` AF=1). Fix: the plain model sets AF after the SHR.

- **Did the evidence depend on AF?** No. The credit rests on `u38_target_gate_2ae6_cpu_audit.py` (the `_cpu` variant with an explicit profile, plus native runs); the plain model is used only by its C unit test, which passes with asserts enabled after the fix.
- **Caller-observable?** Possibly. The SHR flags reach `RETF 0Ah` unchanged, and this body is a **public entry** reached only through the unit's entry table (no direct near or far call to it exists in any unit or in resident code), so the consumer is an indirect dispatcher and cannot be walked statically.
- **Evidence for the verdict (par-e decides):** the evidence did not depend on the plain model's AF, and the plain model now matches pinned; we expect the credit to stand.
