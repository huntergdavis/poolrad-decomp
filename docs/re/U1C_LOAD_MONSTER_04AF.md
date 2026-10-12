# ECL LOAD MONSTER, unit 0x1c +0x04AF..+0x0536 (slice after the entry-5 wrapper)

Continues `U1C_LOAD_MONSTER_PREFIX_0450.md`. GAME.OVR (206,105 bytes, SHA-256 `872964a3...7928614`), unit 0x1c file base 0x0D46.
The slice is file 0x11F5..0x127C, 136 bytes, 49 instructions, SHA-256
`1f65be4a3e667f96a5a670e960c2b51836b49b07ad06b83f7fcca0545dda222e`. No original bytes are reproduced here.

## Order (offsets in the unit)

| Offset | Effect |
|---|---|
| +0x04AF..+0x04BE | `Move(far A, local record copy, 0x11D)` through 0xAF8:0x0A3F (`rep movsb`). A is the object the wrapper left in variable A. |
| +0x04C3..+0x04DF | save the two list heads the wrapper returned (B and C) into two frame slots |
| +0x04E3..+0x04F8 | count := low byte of slot value(2); if it is 0 (unsigned `ja` test) it becomes 1 |
| +0x04F8..+0x0500 | p3 := low byte of slot value(3) |
| +0x0503..+0x050D | cur := far [0x5D96]:[0x5D98] (read only) |
| +0x0510..+0x051F | StrStore (0xAF8:0x0B25, maximum 12) of the code constant cs:0x044B into a local string |
| +0x0524..+0x0532 | far call 0x3E9:0x34 = unit 0x3ce entry 4 (+0x01C8, file 0x31C34) with (far local string, p3, byte [0x6F83]) |
| +0x0537.. | list tail walk and link (not modelled) |

cs:0x044B is a Pascal string of length 4 (hash prefix `939d3c2882b9dbca`); its text is kept out of the repository.

## Model

`pr_u1c_load_monster_04af` (`overlay_u1c_load_monster_04af.{h,c}`) takes the prefix outputs (A, B, C) and an explicit environment:
the entry-5 object table, the constant, `[0x5D96]/[0x5D98]`, `[0x6F83]`, and one fallible hook for 0x3E9:0x34. It stops at the
hook's return (+0x0537). Stops before any later effect: `UNMAPPED` (A not in the table), `VIEW` (fewer than 0x11D bytes from the
offset; each source address wraps at 16 bits), `CORE` (slot value(2)/(3) BOUNDS/UNRESOLVED, or a class-1 load-hook HOOK: the count
steers later loops and p3 is passed to the callee, so a hook that may override either stops the model), `STRING`, `HOOK`,
`CALLBACK`. Nothing about the callee's behaviour, the party links or the final PC is invented.

## Real saved slot-A inputs (private probe, fabricated hooks elsewhere)

Running the same private supplied-state route as the prefix note (explicit host inputs, none from DOS) through the 72 combos that
reach LOAD MONSTER, with the saved slot-A script and windows: slot value(2) (window-2 0x9808) = 6 (equals the saved party count of six),
slot value(3) (class-1 0x6E80) = 0. No HOOK at either slot; the 0x3E9:0x34 hook is reached with p3 = 0 and a 4-byte string.

## Next unresolved dependency

0x3E9:0x34 (unit 0x3ce entry 4, +0x01C8) is the next unresolved callee; after it, the party-list tail walk at +0x0537.

## Claims

Bounded model of a slice; synthetic tests only (`tests/recovered/test_overlay_u1c_load_monster_04af.c`). No strict full-C, count,
identity, DOS-native or one-Up parity claim.
