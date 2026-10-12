# Unit 0x1c +0x03B4: ECL SETUP MONSTER (0x0C), bounded model

Opcode `0x0C` (SETUP MONSTER, reference name; [ECL_CONTRACTS.md](ECL_CONTRACTS.md), 3 operands) dispatches to unit `0x1c`
`+0x03B4..+0x044A`, file `0x10FA..0x1190` (unit base `0x0D46`): **151 bytes of code, 52 instructions**, ending in the `retf` at
`+0x044A`, two bytes of locals that are never used. The ledger's 156-byte span (SHA-256
`5dd2fccd9ab5895fcb07c01e1d931bf95396c9227fc66c1f4855c498905ecddc`) adds five trailing bytes (`+0x044B..+0x044F`), a Pascal
string constant never executed here; the next routine starts at `+0x0450`. SHA-256 of the 151-byte code span `a7c263b02ce580669c4216dee83234a7e80c0da5aaa033f2543dc4e4ec70962c`
(GAME.OVR, 206,105 bytes, `872964a3…7928614`). `deja` timed out (PM's and mine) with no hit. **Classification: bounded model**
with two unresolved callees as hooks; no strict full-C, count, identity, DOS/native or one-Up parity claim.

## Original order (all instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x03BA..+0x03BD` | loader(3), `0x2B:0x2A` |
| `+0x03C2..+0x03CA` | `[0x6F81] := AL` of slot value(1) |
| `+0x03CD..+0x03D9` | `es:[[0x49D6] + 0x580] := AX`, the **full** word of slot value(2): window-1 word `0x2C0`, script address `0x6DC0` (a direct window write, not the store word, so no store hook) |
| `+0x03DE..+0x03E6` | `[0x6F82] := AL` of slot value(3) |
| `+0x03E9..+0x0400` | `0x2B:0x3E` (unit `0x25` entry 6, `+0x0482`) pushed the three bytes `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]`; `es:[... + 0x582] := zero-extended AL` (window-1 word `0x2C1`, `0x6DC1`) |
| `+0x0405..+0x042B` | if `w580 < w582` (`cmp` / `jae`, unsigned): `w582 := w580` |
| `+0x042B..+0x0442` | `0x2B:0x48` (unit `0x25` entry 8, `+0x054E`) pushed `[0x6F81]`, `[0x6F82]`, the low byte of `w582` and the far pointer `DS:0x84C8`; return |

Net effect on the window words: `w580` = slot 2's full value, `w582` = `min(w580, cell)` (unsigned, `cell` the callee's byte).

## Model and unresolved boundaries

`pr_u1c_op_setup_monster` (`overlay_u1c_setup_monster_03b4.{h,c}`). The two callees are **unresolved** and explicit fallible hooks run
in this order: `unit25_0482(x, y, facing) -> AL` and `unit25_054e(state, a, b, c)` (the far pointer `DS:0x84C8` addresses the loader
state's `b84c8`/`b84c9` and an unknown extent of DS beyond; its effect is not modelled). NULL = HOOK, nonzero = CALLBACK, earlier
effects retained (the DS bytes `[0x6F81]`, `[0x6F82]`, the window words). The loader and each of the three slot reads fail in the original order:
slot 1 fatal leaves everything untouched, slot 2 fatal leaves `[0x6F81]` written but not `w580`, slot 3 fatal leaves both written but not
`[0x6F82]`; a class-1 slot read completes and then reports HOOK.

Tests (`test_overlay_u1c_setup_monster_03b4.c`) are fully synthetic: nine slot-2 values against six callee bytes with an independent
unsigned-minimum restatement (the full word kept in `w580`, DS low bytes from word slot values), hook order and arguments, failing and
missing hooks with retained partial effects, the slot failures at each read, a class-1 slot read, the direct window writes, loader failure.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of
[SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound (through the load-hook classifier); explicit host inputs, none from DOS:
`[0x84E2] = 0`, PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result AL2 in {0, 1, 2}, SURPRISE rolls `x`, `y` in 1..6, and the
`0x2B:0x3E` result `cell` in {0, 1, 5}; the `0x2B:0x48` hook is a counting stub.

At `0x9C1E` (24 of the 108 AL2/roll combinations): the slot values are 0 (window-2 word `0x9827`), the literal 2, and 0 (the plain class-1 load
of `0x6E80`), so `[0x6F81] = [0x6F82] = 0`, `w580 = 2`, `w582 = min(2, cell)` (0, 1, 2 for the three cells) and the `0x48` hook is called once.
**Next stop** (all 72 runs): UNMODELLED at PC `0x9C27`, opcode `0x0D` (APPROACH, handler `+0x0817`, 0 operands). The other 84 combinations still
stop at ENCOUNTER MENU `0x29` (PCs `0x9CC5`, `0x9D7E`, `0x9DDE`) as before. Supplied-state probe only: no live DOS RNG, native search reachability
or one-Up parity.
