# ECL store word: the class-1 hook returns with no effect for 0x6E7F

`pr_rec_ecl_store_word` reports HOOK after each class-1 (window 1) store except the addresses already proven
quiet ([6E82](ECL_STORE_HOOK_6E82.md), [6E7D](ECL_HOOKS_6E7D.md), [6DE1](ECL_STORE_HOOK_6DE1.md),
[6E79..6E7B](ECL_STORE_HOOK_6E79.md)). This note adds **`0x6E7F`** (window-1 word `0x37F`, byte offset
`0x6FE`) to the **store** list only; the load hook there is **not** exempted. No count, strict full-C or
DOS/native parity is claimed. `deja` had no earlier trace (only the assigning message); the earlier notes'
method and the private flow follower were reused.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` (slot A) | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| store hook, unit `0x25` `+0x0A74..+0x0C76` | 515 bytes, 163 instructions, `retf 4` | `258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938` |

## Original bytes

Caller (store routine `+0x0C77`, class 1): write `es:[[0x49D6] + 2*address + 0x2A00]` (`+0x0CBB..+0x0CC9`), then
`+0x0CCE..+0x0CD5` push address, push value, `push cs`, `call +0x0A74`, then `jmp +0x0E06`. Hook for `rel = 0x6E7F - 0x6B00 = 0x37F`: a
deterministic **42-instruction** path from `+0x0A74` to the epilogue `+0x0C71`: `cmp 0`, `cmp 0x20 / jb` (not taken),
`cmp 0x70 / ja` (taken), then the exact compares `0xB8, 0xBB, 0xBD, 0xBF, 0xC1, 0xC3, 0xF7, 0xF9, 0x100, 0x10C, 0x312,
0x322, 0x324, 0x326` all miss (`0x37F` is above the largest case `0x326`), landing on `+0x0C71`. No DS/ES write and no
call; the only stores go to its own `[bp+8]` argument copy. The exhaustive private follower over all 65,536 addresses
gives `0x6E7F` the empty effect set.

## Change and tests

`PR_REC_ECL_STORE_HOOK_NOOP_ADDRESS_E` (`0x6E7F`). Tests: the class-1 store sweep has exactly seven OK addresses; for
`0x6E7E`, `0x6E7F`, `0x6E80` the store status, the window write, the standalone hook model (case NONE, record and flags
untouched) and the load hook (still HOOK) are checked, with the neighbours reporting HOOK.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of
[SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound; explicit host inputs, none from DOS: `[0x84E2] = 0`,
PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result AL2 in {0, 1, 2}.

| Second result | Before | After |
| --- | --- | --- |
| 0, 1, 2 | stop at dispatch 35: HANDLER/HOOK at PC `0x9B90`, the second RANDOM's store to `0x6E7F` | RANDOM completes; **ON GOTO at `0x9B90`** (selector = word load of `0x6E7F`, count 3) runs to its GOTO target `0x9B9F`, `0x9BB1`, `0x9BC3` respectively, then reports **HOOK from its slot read**: the class-1 **load** hook at `0x6E7F`, not exempted here (dispatch 36) |

The private follower puts the load hook at `0x6E7F` in the quiet class (flag set to 1 then 0, no read, write or call), so
that load is the next audit; not changed here. Supplied-state probe only: no live DOS RNG, native search reachability or one-Up parity.
