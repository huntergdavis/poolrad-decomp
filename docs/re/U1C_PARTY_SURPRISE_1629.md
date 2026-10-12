# Unit 0x1c +0x1629: ECL PARTY SURPRISE (0x22), bounded model

Opcode `0x22` (PARTY SURPRISE, reference name; [ECL_CONTRACTS.md](ECL_CONTRACTS.md), 2 operands) dispatches to unit `0x1c`
`+0x1629..+0x16C8`, file `0x236F..0x240E` (unit base `0x0D46`), 160 bytes (the `retf` at `+0x16C8`), twelve bytes of locals.
The next routine (`+0x16C9`) is separate. SHA-256 of the span
`d63cdc3dadb9a9cffce7594b4e7a564eda5a9ee01af2b3a447807d51f6d64e07` (GAME.OVR, 206,105 bytes, `872964a3…7928614`). `deja`
timed out (PM's and mine) with no hit. **Classification: bounded model** composed of lane-A pieces, the party list being
host state; no new hooks. No strict full-C, count, identity, DOS/native or one-Up parity claim.

## Original order (all instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x162F..+0x1632` | loader(2), `0x2B:0x2A` |
| `+0x1637..+0x164C` | `node := far [0x5D96]:[0x5D98]`; byte locals `a := b := c := 0` (`[bp-5]`, `[bp-6]`, `[bp-7]`) |
| `+0x1650..+0x1656` | `or` of the two pointer words; zero ends the loop (`+0x1689`) |
| `+0x1658..+0x166A` | `byte es:[di + 0x2F] = 4` or `= 0x0A` |
| `+0x166C..+0x1670` | then `a := 1` and `c := 1` (`c` is never read) |
| `+0x1674..+0x1687` | `node := far pointer at +0x104/+0x106`; loop |
| `+0x1689..+0x16A6` | `d1 := word([0x7086],[0x7046])`, `d2 := word([0x7087],[0x7047])` via `0x2B:0x4D` (slots 1 and 2, value bytes whatever the tags) |
| `+0x16A9..+0x16B2` | `0x2B:0x52 (d1, zero-extended a)` |
| `+0x16B7..+0x16C0` | `0x2B:0x52 (d2, zero-extended b)`; return |

The byte `b` is initialized and never written, so the **second store is always 0**. Result: `d1 := 1` if any party record has
byte `+0x2F` equal to 4 or 10, else 0; `d2 := 0`.

## Model and missing state

`pr_u1c_op_party_surprise` (`overlay_u1c_party_surprise_1629.{h,c}`). The party list is explicit host state (the same
`pr_u68_0236_node` view as the unit `0x68` party-byte routine and PARTYSTRENGTH): field reads are `view[(offset + k) mod
65536]`; a null or too small view, or a list past the cap (the original has no cycle check), is a host guard (status LIST) with
nothing stored. Failure order: loader first (list untouched); list failure before any store; each store word status is
reported after its attempt: an unsupported first destination stops before the second store, a class-1 potential-effect
destination stores, continues to the second store, then reports HOOK.

Tests (`test_overlay_u1c_party_surprise_1629.c`) are fully synthetic: all 256 values of `+0x2F` for one node, every match
subset of lists of 0..5 nodes against an any-match restatement, neighbouring bytes ignored, the 16-bit offset wrap, store
order and destination classes, tag-independent destination words, host guards, loader failure.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of
[SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound; explicit host inputs, none from DOS: `[0x84E2] = 0`,
PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result AL2 in {0, 1, 2}.

At `0x9C01` the handler visits 6 nodes; **none has `+0x2F` equal to 4 or 10**, so word `0` is stored to the window-2 word
`0x9802` and `0` to `0x9803` (both OK). **Next stop** (all three): run status UNMODELLED at PC `0x9C08`, opcode `0x23`
(SURPRISE, handler `+0x16C9`, 4 operands), previous opcode `0x22`, dispatch 45 (44 for AL2 = 2). Supplied-state probe
only: no live DOS RNG, native search reachability or one-Up parity.
