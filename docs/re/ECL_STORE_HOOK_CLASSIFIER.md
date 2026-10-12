# ECL store word: the class-1 hook classifier (96 potential-effect addresses)

`pr_rec_ecl_store_word` used to report HOOK after every class-1 (window 1) store except a growing list of single
addresses, each proven by [its own note](ECL_STORE_HOOK_0A74.md) (6E82, 6E7D, 6DE1, 6E79..6E7B, 6E7F, 6DC1). This
change replaces that list with a classifier, `pr_rec_ecl_store_hook_may_effect(address)`, so the decision is made for all
1,024 class-1 addresses at once: **HOOK exactly for the 96 addresses at which the original hook can write game
state or call, OK for every other class-1 address**. The store hook is the one unit `0x25` routine at `+0x0A74`; the window
write happens first and the hook is the last call (`+0x0CCE..+0x0CD5`), as before. The **load-hook** behavior is unchanged
(its exceptions remain the separately proven addresses). No count, strict full-C or DOS/native parity is claimed; this is a
bounded status classifier. `deja` timed out at the assigning step; the per-address notes above were reused.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` (slot A) | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| store hook, unit `0x25` `+0x0A74..+0x0C76` | 515 bytes, 163 instructions, `retf 4` | `258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938` |

## The effect set, verified independently from the original bytes

The hook subtracts `0x6B00` from its address argument (`+0x0A7D`) and walks a compare chain on `rel`: `rel = 0`
(`+0x0A86`), the range `0x20..0x70` (`+0x0A99..+0x0AA1`, `jb`/`ja`) and the exact compares `0xB8, 0xBB, 0xBD, 0xBF,
0xC1, 0xC3, 0xF7, 0xF9, 0x100, 0x10C, 0x312, 0x322, 0x324, 0x326`, each leading to a body that writes DS/ES or
calls; the final `cmp 0x326 / jne` lands on the epilogue `+0x0C71` (`mov sp,bp`). A private static flow follower over the
515 bytes explores **both sides of every value-dependent branch** for each of the 1,024 class-1 addresses and records
any DS/ES write or call on any path: the effectful set is **exactly 96 addresses** (1 + 81 + 14), identical to the
list above, and every other address has the single empty effect set (42 instructions to the epilogue, only stores to its
own `[bp+8]` argument copy). Because both sides of value-dependent branches are included, the classification does not
depend on the stored value. The earlier per-address traces are consistent with it (the quiet ones are all in the
complement).

## Change and tests

`pr_rec_ecl_store_hook_may_effect` (ecl_helpers.c) is the predicate; `pr_rec_ecl_store_word` returns HOOK iff it is true
for a class-1 address; the individual exemption macros are removed. An address outside class 1 returns nonzero
(never classified) but the store word does not call the hook there. Tests (`test_ecl_helpers.c`,
`test_store_word_classifier`): for all 1,024 class-1 addresses and nine values (including 0, 0xFF, 0x100, 0x7FFF, 0x8000,
0xFFFF) the store status is HOOK exactly when an explicit range table written in the test says so, the standalone model of
the hook (`pr_u25_store_hook_0a74`, an independent oracle) agrees on every address, the window word is written first in both
cases, and the counts are 96 and 928; boundaries and every old exception (`0x6E82, 0x6E7D, 0x6DE1, 0x6E79..0x6E7B,
0x6E7F, 0x6DC1`) are quiet, with `0x6E80`, `0x6E7C`, `0x6E7E`, `0x6E81` and `0x6E83` also quiet and the cases around them
HOOK; the load hook is unchanged. Older tests that expected HOOK at quiet store addresses (rel `0x10`, `0x2CA`, `0x37E`)
were updated to a potential-effect address or to OK.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of
[SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound; explicit host inputs, none from DOS: `[0x84E2] = 0`,
PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result AL2 in {0, 1, 2}.

The second GETTABLE (store to `0x6E80`) now completes, as do the next three; the five GETTABLEs of the table block at
`0x9BCF..0x9BF7` all run (elements read from the saved script, e.g. the fifth at `0xB6EF`: 0, 2, 4 for AL2 = 0, 1, 2).
**Next stop** (all three): run status UNMODELLED at PC `0x9C01`, opcode `0x22` (PARTY SURPRISE, handler `+0x1629`, 2
operands), previous opcode `0x2A`, dispatch 44 (43 for AL2 = 2). Supplied-state probe only: no live DOS RNG, native search
reachability or one-Up parity.
