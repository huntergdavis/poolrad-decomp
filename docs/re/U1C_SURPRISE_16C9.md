# Unit 0x1c +0x16C9: ECL SURPRISE (0x23), bounded model

Opcode `0x23` (SURPRISE, reference name; [ECL_CONTRACTS.md](ECL_CONTRACTS.md), 4 operands) dispatches to unit `0x1c`
`+0x16C9..+0x178E`, file `0x240F..0x24D4` (unit base `0x0D46`): **198 bytes of code, 84 instructions**, ending in the `retf` at
`+0x178E`. The 210-byte ledger span extends 12 bytes further (`+0x178F..+0x179A`, to file `0x24E0`): embedded Pascal string
constants, never executed by this routine; the next routine starts at `+0x179B`. SHA-256 of the 198-byte code span `85c9d34635ee54fac2bcf25953df145e1ba558d9531a88da6375f0f1c1e1b6d8`; of the 210-byte span
`1e707378558b6b7e710fe14b8d13539e417579ee55664d2ded29ca9372b0e675` (GAME.OVR, 206,105 bytes, `872964a3…7928614`). `deja` timed out
(PM's and mine) with no hit. **Classification: bounded model**; no strict full-C, count, identity, DOS/native or one-Up
parity claim.

## Original order (all instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x16CF..+0x16D2` | loader(4), `0x2B:0x2A` |
| `+0x16D7..+0x16FD` | `r := 0`; for `i = 1..4`: `s[i] := AL` of slot value(i) (`0x2B:0x25`), stored as a byte at `[bp + i - 9]` |
| `+0x16FF..+0x170F` | `a := byte (s4 + 2 - s1)` (16-bit register arithmetic, `AL` kept) |
| `+0x1712..+0x1722` | `b := byte (s2 + 2 - s3)` |
| `+0x1725..+0x173E` | two calls `0xB0:0x48` pushed `(1, 6)` (unit `0x9E` entry 8, count then range): `x := AL`, `y := AL` |
| `+0x1741..+0x1763` | `cwde` of `a` (sign extension); if **not** `x > a` (`jg`, signed 16-bit, `x` zero-extended): `r := 1` if `y > b` (`b` sign-extended) else `r := 3` |
| `+0x1769..+0x1778` | if **not** `y > b`: `r := 2` |
| `+0x177C..+0x1786` | `0x2B:0x52 (0x6DCB, r)`; return |

Net result `r = 2` if `y <= b`; else `1` if `x <= a`; else `0` (signed). The `3` assigned at `+0x175F` always has `y <= b`
and is therefore always overwritten by `2`; it is never stored. The destination is the **constant class-1 word `0x6DCB`**,
not an operand.

## Model

`pr_u1c_op_surprise` (`overlay_u1c_surprise_16c9.{h,c}`): the two `0xB0:0x48` calls are an explicit fallible host hook run in
this order with `(1, 6)` (NULL = HOOK, nonzero = CALLBACK, neither stores); no dice semantics. Loader and slot-value
BOUNDS/UNRESOLVED stop before any roll; a class-1 slot read completes and then reports HOOK; the store status is reported after
the attempt. Tests (`test_overlay_u1c_surprise_16c9.c`) are fully synthetic: 11 boundary bytes for each of the four slot values
and 10 for each roll against an independent signed restatement, the 8-bit wrap of the
derived bytes, the signed compare against a zero-extended roll, that `3` is never stored, hook order and arguments, failing
and missing hooks, the constant destination, loader and slot failures, and a class-1 word slot value (low byte only).

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A (`abca2483…c08f1`), run entry `0x9914`, search entry `0x9975`, the six-record host party of
[SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound (including PARTY SURPRISE); explicit host inputs, none
from DOS: `[0x84E2] = 0`, PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result AL2 in {0, 1, 2}, and the two SURPRISE roll
results `x`, `y` each in 1..6 (all 36 pairs tried).

At `0x9C08` the four slot values are all 0 (the saved window-2 words), so `a = b = 2`; the result and the word stored to `0x6DCB`
follow the supplied rolls (OK, a quiet class-1 store). **Next stop** (all 36 pairs, all three AL2): HANDLER / HOOK at PC `0x9C19`
after the COMPARE at `0x9C13` (dispatch 46; 45 for AL2 = 2) whose first operand is the class-1 word `0x6DCB`: the **load** hook,
not exempted (the private load follower finds `0x6DCB` quiet). Supplied-state probe only: no live DOS RNG, native search
reachability or one-Up parity.
