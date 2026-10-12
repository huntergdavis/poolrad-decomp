# ECL store word: the class-1 hook returns with no effect for 0x6DC1

`pr_rec_ecl_store_word` reports HOOK after each class-1 (window 1) store except the addresses already proven
quiet ([6E82](ECL_STORE_HOOK_6E82.md), [6E7D](ECL_HOOKS_6E7D.md), [6DE1](ECL_STORE_HOOK_6DE1.md),
[6E79..6E7B](ECL_STORE_HOOK_6E79.md), [6E7F](ECL_STORE_HOOK_6E7F.md)). This note adds **`0x6DC1`** (window-1
word `0x2C1`, byte offset `0x582`) to the **store** list only; the load hook there is not exempted. No count,
strict full-C or DOS/native parity is claimed. `deja` timed out at the assigning step; the 6E7F/6DE1 method and
the private flow follower were reused and the path below was traced from the original bytes.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` (slot A) | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| store hook, unit `0x25` `+0x0A74..+0x0C76` | 515 bytes, 163 instructions, `retf 4` | `258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938` |

## Original bytes

Caller (store routine `+0x0C77`, class 1): window write `+0x0CBB..+0x0CC9`, then push address, push value, `push cs`,
`call +0x0A74` (`+0x0CCE..+0x0CD5`), `jmp +0x0E06`. Hook for `rel = 0x6DC1 - 0x6B00 = 0x2C1`: a deterministic
**42-instruction** path to the epilogue `+0x0C71`: `cmp 0`, `cmp 0x20 / jb` (not taken), `cmp 0x70 / ja` (taken), then
`0xB8, 0xBB, 0xBD, 0xBF, 0xC1, 0xC3, 0xF7, 0xF9, 0x100, 0x10C, 0x312, 0x322, 0x324, 0x326` all miss (`0x2C1` is above
`0x10C` and below `0x312`, equal to none), `+0x0C71`. No DS/ES write, no call, only stores to its own `[bp+8]` argument
copy. The exhaustive private follower over all 65,536 addresses gives `0x6DC1` the empty effect set.

## Change and tests

`PR_REC_ECL_STORE_HOOK_NOOP_ADDRESS_F` (`0x6DC1`). Tests: the class-1 store sweep has exactly eight OK addresses; for
`0x6DC0`, `0x6DC1`, `0x6DC2` the store status, the window write, the standalone hook model (case NONE, record and
flags untouched) and the load hook (still HOOK) are checked, with the neighbours reporting HOOK.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of
[SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound; explicit host inputs, none from DOS: `[0x84E2] = 0`,
PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result AL2 in {0, 1, 2}.

First GETTABLE (`0x9BCF`) now completes (word `2` stored to `0x6DC1`, OK). **Next stop** (all three): the
**second GETTABLE** at `0x9BD9`: base `0xB6E6` (saved script table, class 3), index AL2, addresses `0xB6E6`, `0xB6E7`,
`0xB6E8`, elements `0`, `2`, `4` (the saved script bytes), `dest = 0x6E80`: a class-1 store whose hook is not exempt (the
word is written), so HANDLER / HOOK at PC `0x9BE3` (dispatch 40; 39 for AL2 = 2). Not audited here; the private
follower finds `0x6E80` quiet. Supplied-state probe only: no live DOS RNG, native search reachability or one-Up parity.
