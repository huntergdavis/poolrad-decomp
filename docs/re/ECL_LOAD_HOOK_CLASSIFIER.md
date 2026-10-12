# ECL load word: the class-1 hook classifier (29 potential-effect addresses)

> **Update:** supplied active-record views now resolve the
> [`0x6B18`](U1C_ARITH_CLASS1_RECORD_6B18.md) and
> [`0x6B9B`](ECL_LOAD_HOOK_6B9B_RECORD.md) branches. The 29-address classifier
> still describes which addresses can have a hook effect; with no supplied
> record, these two retain the bounded `HOOK`/window behavior described below.

> **Further update:** a supplied record also resolves the
> [`0x6BA0` signed-byte branch](ECL_LOAD_HOOK_6BA0_SIGNED.md); 26 potential-effect
> addresses remain bounded.

`pr_rec_ecl_load_word` used to report HOOK after every class-1 (window 1) load except a growing list of single addresses,
each proven by [its own note](ECL_LOAD_HOOK_6DD5.md) (6DD5, 6E7D, 6E82, 6DCA, 6E7F). This change replaces that list with a
classifier, `pr_rec_ecl_load_hook_may_effect(address)`, so the decision is made for all 1,024 class-1 addresses at once:
**HOOK exactly for the 29 addresses at which the original hook can read the character record or DS, call, or leave its
by-reference flag at 1; OK for every other class-1 address**, where the hook sets the flag to 1 (`+0x07C5`), clears it to 0
(`+0x0A67`) and does nothing else, so the caller (`+0x0E41..+0x0E69`) reads the window-1 word. Load order and classes 0, 2, 3
and 4 are unchanged. The **store** hook has its own classifier ([ECL_STORE_HOOK_CLASSIFIER.md](ECL_STORE_HOOK_CLASSIFIER.md)).
No count, strict full-C or DOS/native parity is claimed; this is a bounded status classifier. `deja` timed out at the
assigning step; the per-address notes were reused.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` (slot A) | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| load hook, unit `0x25` `+0x07BC..+0x0A73` | 696 bytes, 233 instructions, `retf 6` | `6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451` |

## The effect set, verified twice from the original bytes

The hook subtracts `0x6B00` from its address argument (`+0x07C9`) and walks a compare chain on `rel`. Two separately written
static analyses of the 696 bytes (a flow follower from the earlier notes, and a second, independently coded abstract
interpreter that tracks only the address argument, the flags and the `es:`/DS accesses) explore **both sides of every
branch whose condition depends on unknown data** (the character record, DS bytes) for each of the 1,024 class-1 addresses
and record every DS/ES write, record/DS read and call, and whether the by-reference flag ends at 0 after the single
`mov byte es:[di], 0` at `+0x0A67`. They agree: a class-1 address is **quiet** (every path is exactly
`[flag := 1 at +0x07C5, flag := 0 at +0x0A67]`, 62 instructions to `retf 6`) for 995 addresses, and **not quiet** for exactly 29:

`rel` = `0x018`, `0x09B`, `0x0A0`, `0x0A5..0x0AC` (eight consecutive), `0x0B8`, `0x0BB`, `0x0BD`, `0x0BF`, `0x0C1`, `0x0C3`, `0x0D8`,
`0x0F7`, `0x0F9`, `0x100`, `0x10C`, `0x10D`, `0x11B`, `0x2B1`, `0x2B4`, `0x2CF`, `0x312`, `0x33E`,

(absolute `0x6B18`, `0x6B9B`, `0x6BA0`, `0x6BA5..0x6BAC`, `0x6BB8`, ... `0x6E3E`). The outcomes at these addresses are of three kinds:
paths that read the character record or DS and leave the flag at 1, paths that call, and paths that leave the flag at 1 without
clearing it. The classification does not depend on any value, because the load hook has no value argument and the data-dependent
branches are explored both ways. The earlier per-address traces (6DD5, 6E7D, 6E82, 6DCA, 6E7F, and the checked 6DCB) all fall in the
quiet complement.

## Change and tests

`pr_rec_ecl_load_hook_may_effect` (ecl_helpers.c) is the predicate (an address outside class 1 returns nonzero, never classified; the
load word does not call the hook there). `pr_rec_ecl_load_word` returns HOOK iff it is true for a class-1 address, after reading the
window word as before; the five per-address macros are removed. Tests (`test_ecl_helpers.c`, `test_load_word_classifier`): all 1,024
class-1 addresses with eight values: the status is HOOK exactly when an explicit table written in the test lists the address, the value
read is always the host's window word, and the counts are 29 and 995; boundaries (the old exceptions plus `0x6DCB`, neighbours of the
effect cases, `rel` 0, `0x17`, `0x19`, `0x9A`, `0x9C`, `0xA1..0xA4`, `0xAD`), the two classifiers being independent (`0x6B20` is a
plain load and a store-effect address; `0x6B18` the reverse), COMPARE and slot-value reads through the core at plain and effect
addresses. Many older tests used `0x6B40` as a "class-1 load that reports HOOK"; they now use `0x6B18` (rel `0x18`, an effect address),
keeping `0x6B40` as a store-effect destination.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of
[SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound (through SURPRISE); explicit host inputs, none from DOS:
`[0x84E2] = 0`, PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result AL2 in {0, 1, 2}, and the two SURPRISE roll results
`x`, `y` each in 1..6. The COMPARE of `0x6DCB` at `0x9C13` now completes. All 108 combinations (3 x 36) stop UNMODELLED, in four groups:

| Stop (run status UNMODELLED) | Combinations |
| --- | ---: |
| PC `0x9C1E`, opcode `0x0C` (SETUP MONSTER, handler `+0x03B4`, 3 operands) | 24 (all three AL2) |
| PC `0x9CC5`, opcode `0x29` (ENCOUNTER MENU, handler `+0x1EF8`, 14 operands) | 32 (AL2 = 1, 2) |
| PC `0x9D7E`, opcode `0x29` | 16 (AL2 = 0) |
| PC `0x9DDE`, opcode `0x29` | 36 (all three AL2) |

The path depends on the supplied party list, the RANDOM results and the SURPRISE rolls. Supplied-state probe only: no live DOS RNG,
native search reachability or one-Up parity.
