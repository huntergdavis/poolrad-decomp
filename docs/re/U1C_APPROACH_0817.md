# Unit 0x1c +0x0817: ECL APPROACH (0x0D), bounded model

Opcode `0x0D` (APPROACH, reference name; [ECL_CONTRACTS.md](ECL_CONTRACTS.md), 0 operands) dispatches to unit `0x1c`
`+0x0817..+0x0850`, file `0x155D..0x1596` (unit base `0x0D46`): **58 bytes of code, 21 instructions**, ending in the `retf` at
`+0x0850`, no locals (`pop bp; retf`). The ledger's 63-byte span (SHA-256
`2e3daf4d3dc7e31dea568ef00e85b72a57d8b35ae7ec393755f0468ec31f61a3`) adds five trailing bytes (`+0x0851..+0x0855`), a Pascal string
constant never executed here; the next routine is `+0x0856` (PICTURE). SHA-256 of the 58-byte code span `43796e27c5f066c72c390f219381f3da43fae497dcc79f557a028a3a6610f002` (GAME.OVR, 206,105
bytes, `872964a3…7928614`). `deja` timed out (PM's and mine) with no hit. **Classification: bounded model**: its one
callee, `0x2B:0x48` (unit `0x25` `+0x054E`), is unresolved, so no strict full-C or count credit is proposed. No identity, DOS/native or
one-Up parity claim.

## Original order (all instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x081A..+0x0824` | `cmp word es:[[0x49D6] + 0x582], 0; jbe +0x084B` (unsigned: skip when the window-1 word `0x2C1`, script address `0x6DC1`, is 0) |
| `+0x0826..+0x082A` | `dec word` of that word (16-bit) |
| `+0x082F..+0x0846` | `0x2B:0x48` pushed `[0x6F81]`, `[0x6F82]`, the low byte of the decremented word and the far pointer `DS:0x84C8` |
| `+0x084B` | `inc word [0x49ED]` (the PC), reached on both paths; `pop bp; retf` |

No loader call and no operand. The word `w582` and the bytes `[0x6F81]`, `[0x6F82]` are the state SETUP MONSTER ([U1C_SETUP_MONSTER_03B4.md](U1C_SETUP_MONSTER_03B4.md))
leaves, and the `0x2B:0x48` call is the same as that handler's last call.

## Model

`pr_u1c_op_approach` (`overlay_u1c_approach_0817.{h,c}`) shares `pr_u1c_setup_monster_env` (the two DS bytes and the unresolved
`unit25_054e` hook). When `w582` is nonzero it decrements the word, calls the hook (NULL = HOOK status, nonzero = CALLBACK status; the
decrement stands and the PC is not advanced in both cases), then advances the PC; when `w582` is 0 only the PC advances. Tests
(`test_overlay_u1c_approach_0817.c`) are fully synthetic: twelve boundary values of `w582` (0, 1, 2, 0x7F, 0x80, 0xFF, 0x100 ... 0xFFFF) with the hook
arguments (the low byte of the decremented word), PC advance and 16-bit PC wrap, repeated calls counting down to zero, failing and missing
hooks, and a composition with the SETUP MONSTER model.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged
handlers bound (through SETUP MONSTER); explicit host inputs, none from DOS: `[0x84E2] = 0`, PICTURE DS bytes 0, first RANDOM result 13, second
RANDOM result AL2 in {0, 1, 2}, SURPRISE rolls in 1..6, the `0x2B:0x3E` result `cell` in {0, 1, 5}, counting `0x2B:0x48` stub.

For the 72 runs that reach SETUP MONSTER, APPROACH at `0x9C27` sees `w582 = min(2, cell)` = 0, 1, 2: for `cell = 0` it only advances the PC (no hook call),
for `cell = 1` and `cell = 5` it decrements to 0 and 1 and calls the hook once. **Next stop** (all 72): UNMODELLED at PC `0x9C28`, opcode `0x3A` (DELAY, handler
`+0x271F`, 0 operands), previous opcode `0x0D`. The other 252 combinations still stop at ENCOUNTER MENU `0x29`. Supplied-state probe only: no live DOS RNG, native
search reachability or one-Up parity.
