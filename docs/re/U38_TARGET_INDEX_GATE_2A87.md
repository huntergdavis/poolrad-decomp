# Unit 0x38 public entry 99: indexed target gate at +0x2A87

## Provenance and game route

The bounded `deja "poolrad U38 +2A87 entry99 +101D strict C parity"`
query timed out without a prior-session result. This work reuses the checked
[+0x101D target selector](U38_TARGET_SELECT_101D.md), its
[resident selector](RESIDENT_CHARACTER_TARGET_SELECT.md), and the
[effect-table installer](U38_EFFECTS_INSTALL_31A2.md). The source bytes
and entry boundaries were independently rechecked against these ignored
local inputs:

| Input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The complete public entry is unit `0x38:[+0x2A87,+0x2AE6)`, `GAME.OVR`
file `[0x116E2,0x11741)`: 95 bytes / 37 contiguous instructions,
SHA-256 `7e7c8ac21b1334814e21a84a1403ca7964c9397ea0477acde8a989e5d271fc40`.
The next public entry starts at `+0x2AE6`. There are no wrapper fixups.
Its only call is `+0x2A96 -> +0x101D`; the helper has the verified
loaded-segment resident call at `:2E8B`.

The +0x31A2 installer writes entry-99 stub `0041:020F` to `DS:69F4`
(effect-table item 115). Its four instruction sites are +0x3762,
+0x3765, +0x3768, and +0x376B; the segment operand has loader fixup
+0x3766.
The game's U9E dispatcher calls through this table, but no native frame
through item 115 into this entry has been reached. Function identity and
strict count remain provisional.

## Exact behavior and C boundary

The entry passes the active character far pointer from `DS:5D92/5D94`
to `+0x101D`. A null target leaves `DS:6818` unchanged. For a nonnull
target, it reads byte `+0x2E`, scales that index by 16, and checks
`DS:[0x5589 + 16*index]`. A zero table byte halves the unsigned effect
byte `DS:6818`. A nonzero table byte causes a second target-index read;
if the table byte's bit 0 is set, it also halves `DS:6818`. Otherwise it
preserves the effect byte. The routine exits with `RETF 10` after five
caller words.

`pr_u38_target_index_gate_2a87` models all 95 bytes in a 1 MiB RAM and
14-field CPU state. It composes the merged +0x101D C model, including
its original near-call frame and `RETF 4`. A callback handles the
resident far call and its register/flag/memory effects. Callback refusal
returns `PR_U38_2A87_CALL_ABORT` with the resident call frame intact.

## Measured parity and limits

The original-byte Unicorn checker executes the complete wrapper and
+0x101D helper, including both actual calls. Across 576 cases it
compares the entire 1 MiB RAM, all 14 CPU fields, and each resident
callback frame. Cases cover all eight helper selection modes, target
indexes 0, 7 and 255, zero, bit-0 and other table bytes, amount values,
two loaded far-call segments, and three DS/SS/stack layouts.

Four DOSBox-X normal-core synthetic far-entry cases execute the pinned
wrapper and helper with a deterministic resident stub. They compare all
14 CPU words, 52 stack bytes, and `DS:6818`. AF after `SHR AX,1` is
undefined and is masked only on the two halving frames; all other FLAGS
bits are checked. The native capture is under ignored
`resources/u38-2a87-parity/native/`, SHA-256
`a9f0d14f28982b35a2d682269e95f359ed6db0ae1dd88f6ba350660710ec8941`.
Focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan tests cover
null, both halving branches, preserve, and callback refusal.

These are bounded behavior checks with a synthetic resident stub. No
natural game caller or item-115 dispatch has been measured. No strict
identity or function-count credit is claimed.

## Pinned AF after SHR: plain model fixed (2026-10-23, census #1464 family 2)

The plain (non-`_cpu`) model ran `SHR AX,1` through `logic16`, which clears AF; pinned DOSBox-X sets AF after a non-zero shift (`get_AF` = `count & 1Fh`, `flags.cpp:172-177`; stored `763-771`). The `_cpu` variant already had both behaviours behind an explicit `shift_profile` (`PR_TARGET_SHIFT_UNICORN` AF=0, `PR_TARGET_SHIFT_DOSBOX386` AF=1). Fix: the plain model sets AF after the SHR.

- **Did the evidence depend on AF?** No. The credit rests on `u38_target_index_gate_2a87_cpu_audit.py`, which exercises the `_cpu` variant with an explicit profile (Unicorn: AF=0, against Unicorn) plus native runs; the plain model is used only by its C unit test, which passes with asserts enabled after the fix.
- **Caller-observable?** Possibly. The SHR flags reach `RETF 0Ah` unchanged, and this body is a **public entry** reached only through the unit's entry table (no direct near or far call to it exists in any unit or in resident code), so the consumer is an indirect dispatcher and cannot be walked statically.
- **Evidence for the verdict (par-e decides):** the evidence did not depend on the plain model's AF, and the plain model now matches pinned; we expect the credit to stand.
