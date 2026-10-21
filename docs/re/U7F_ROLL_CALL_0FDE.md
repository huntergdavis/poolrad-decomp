# U7F +0FDE far-call boundary adapter

`pr_u7f_roll_call_0fde` models only the original five-byte
`LCALL 0x00B0:0x0048` at U7F +0FDE. The stub maps to U9E public entry 8
at +0DE3. The merged `pr_u9e_entry8_roll` provides count/range/seed/AL
behavior, but does not expose that callee's complete RAM and CPU return
state. The callback therefore owns stub dispatch, U9E entry 8, nested
SYSTEM effects, live `RETF 4`, and any transfer. No missing register,
flag, or RAM effect is synthesized by this adapter.

The owner-supplied `GAME.OVR` is 206105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U7F +0FD2..+1010 (exclusive end) is file bytes `[154715,154777)`, 62 bytes / 27
contiguous instructions, SHA-256
`9205417006da7a4c928b547d116107e3f4d269d3d267d17b2f7cad2f3e63fe27`.
The exact far-call bytes at +0FDE are `9A 48 00 B0 00`.

## Contract

Entry is U7F CS:+0FDE after the source's two argument pushes. The
adapter pushes the original CS and return IP +0FE3, in that order,
using 16-bit SP wrapping and contiguous A20-off word bytes. It sets
CS:IP to the literal B0:0048 stub and invokes the callback with the
same full 1 MiB RAM and 14-word CPU state. A callback return at the
original CS:+0FE3 reports `RETURNED`; any other CS:IP reports
`TRANSFER` with state intact. A callback abort reports `CALL_ABORT`
with its live state intact. Invalid host arguments are rejected before
mutation. The adapter imposes no timeout or invented callee result.

The focused C test uses the existing high-level U9E roll helper only
inside a synthetic callback, then supplies explicit remaining register
and flag effects. It checks normal return, abort, transfer, and an SP
wrap frame with a linear CS decoy. This is a callback contract test,
not proof that the high-level helper alone reproduces U9E CPU state.
The original-byte checker stops when the actual call reaches B0:0048
and compares the entire 1 MiB RAM and all 14 CPU words with the C
adapter at that same boundary. All 40 bounded cases pass, including
stack offset wraps, A20 access, and varied register/flag inputs. It does
not execute the stub or callee. Strict C11 CTest and direct ASan+UBSan
also pass.

The bounded `deja` query for U7F +0FD2 timed out without a finding.
Reused evidence is the census packet's exact span and pG no-claim,
the merged [U9E entry8 model](U9E_ENTRY8_ROLL.md), and the existing
[U9E entry9 callback pattern](U9E_ENTRY9_ROLL_LATCH_0E2E.md).
Census explicitly cleared this adapter scope (i7023), and pG confirmed
no collision with its U7F or U9E work (i7043); the adjacent
U7F +0F7E body ends at +0FD2 and pG's +0020 leaf is disjoint.

No native game route, full U9E CPU composition, game identity, or strict
full-C credit follows from this call boundary.

## Correction, 2026-10-21: the LCALL's segment is the live code word, not 00B0h

The C adapter entered the roll with `CS := 00B0h`, a constant, where the original's `9A` at `0FDE` carries a segment word at
`0FE1` that the loader relocates (the overlay stub segment, `00B0` only at the unrelocated base). The 40 original-byte cases
and the unit test ran at that base, where constant and word agree, and the unit test asserted the constant, so neither could
see it. The adapter now enters at the word read from `CS:0FE1`; the unit test sets the word to `00B0`, `1234`, `0001`, `FFB0`
and `10B0` and checks the roll is entered at it (fails on the old adapter: `cpu->cs == ctx->word`). The 40-case original-byte
parity still passes. Body bytes, hash, identity and credit unchanged.
