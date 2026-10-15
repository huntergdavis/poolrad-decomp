# Unit 0x9E entry 9: roll latch at +0x0E2E

## Selection and source

PM assigned this disjoint strict full-C candidate after U38 `+0x0918`.
The bounded `deja "poolrad U9E +0E2E entry 9 near call +0DE3 strict C parity"`
query timed out without a prior-session result. The merged
[entry-8 roll evidence](U9E_ENTRY8_ROLL.md) supplies the callee's dice and
seed behavior; its source and original span were checked again. The
[unit-38 effect evidence](OVERLAY_UNIT38_EFFECT_ENTRIES_NEXT.md) identifies
game consumers of the `DS:681C` latch.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The **complete original function** is `GAME.OVR` file
`[0x2BD18,0x2BD3C)`, unit `0x9E:[+0x0E2E,+0x0E52)`: 36 contiguous bytes,
16 instructions, SHA-256
`08dc2c3413d0fcfa12d26b589873f619761741e3f38c235477e89ed3a7350443`.
Public entry 10 starts at `+0x0E52`. The sole call is `PUSH CS` at
`+0x0E42` followed by near `CALL +0x0DE3` at `+0x0E43`. There is no fixup
or other call in this body.

Entry 9's far-call stub is `B0:004D`; 12 fixup-confirmed call sites target it:
unit `0x38` at `+1835,+1A37,+1A6A,+2B52,+2CE8,+2D6A`; unit `0x5E` at
`+01B4`; and unit `0x86` at `+10BD,+1460,+14E9,+2744,+306E`. These are
static game call sites, not a measured saved-game route.

## C boundary and callee composition

Entry 9 receives count at `[BP+8]` and range at `[BP+6]`, uses their low
bytes, and returns with `RETF 4`. It writes the count byte to `DS:681C`,
then pushes the count and range WORDs for entry 8. Both pushes retain the
incoming AH high byte; they do not push the original argument WORDs. The
`PUSH CS` plus near call builds a far-call frame with return IP `0E46`,
return CS, range, and count. Entry 8's `RETF 4` removes that frame and the
two pushed arguments. Entry 9 stores returned AL in its local byte,
reloads AL, and returns, removing its own two caller arguments.

`pr_u9e_entry9_roll_latch_0e2e` models every wrapper instruction's CPU and
1 MiB RAM effects. Its explicit callback enters at the **callee-entry**
frame and supplies all effects of `+0x0DE3` before the wrapper emulates the
callee's `RETF 4`. The focused C adapter invokes merged
`pr_u9e_entry8_roll` on the frame's count/range, reads and rewrites the
`DS:4428` seed DWORD, and returns AL. The callback also carries callee
register/flag effects, which the entry-8 high-level API does not expose.
The wrapper has no separate RNG algorithm or unmodeled branch.

## Verification and limits

The original-byte Unicorn checker executes all 36 wrapper bytes and the
real near call before hooking at `+0x0DE3`. Its independent Python dice
calculation is compared against merged C entry 8 in the C callback.
Across 96 cases it compares the entire 1 MiB RAM, all 14 CPU fields, and
the exact four-word callee-entry frame. The cases cover counts 0, 1, 3,
255; ranges 0, 1, 6, 255; distinct and aliased DS/SS, a `DS:681C` alias
with an argument, and 16-bit stack wrap. Focused C11
`-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTests pass for this wrapper
and merged entry 8, including seed advance, count/range truncation,
callee register/flag propagation, and call abort.

A second 16-case audit executes **both original U9E bodies** in Unicorn,
intercepting only the SYSTEM `AF8:11C0` random call. It checks that the
original callee is entered through the wrapper, that the runtime call frame
and call count match, and that final AL, `DS:4428` seed DWORD, `DS:681C`
latch, and far-return frame match merged `pr_u9e_entry8_roll` for the four
count and four range values above. The SYSTEM hook supplies the verified
seed recurrence from the merged entry-8 evidence; this audit checks the
composition and does not claim full CPU parity for the SYSTEM routine.

Eight DOSBox-X normal-core synthetic frames execute an actual far call into
the original wrapper, then its actual near call into a deterministic test
callee at `+0x0DE3`. All 14 returned CPU words, the `DS:681C` latch, and
22 stack bytes agree with the C model. The generated COM, copied body,
logs, and capture stay under ignored `resources/u9e-0e2e-parity/native/`.
Capture SHA-256:
`c2d7bbbade47925fed3e783136862441fbf37b1b96e3057b3521ba743434275e`.

The DOSBox-X frame is a synthetic caller and does not execute the original
entry-8 body. The 96-case full-state oracle hooks at entry 8; the separate
16-case audit executes its original body. The callee's non-AL CPU effects
remain an explicit callback obligation because the merged high-level
entry-8 API exposes AL and seed, not its complete register/flag state.
This is a strict full-C **candidate** for PM review; no identity or count
credit is claimed here.
