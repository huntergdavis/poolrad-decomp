# ECL store word: the class-1 hook returns with no effect for 0x6E79..0x6E7B

`pr_rec_ecl_store_word` reports HOOK after each class-1 (window 1) store except the
addresses already proven quiet ([6E82](ECL_STORE_HOOK_6E82.md),
[6E7D](ECL_HOOKS_6E7D.md), [6DE1](ECL_STORE_HOOK_6DE1.md)). This note adds the three
contiguous addresses **`0x6E79`, `0x6E7A`, `0x6E7B`** (window-1 words `0x379..0x37B`,
byte offsets `0x6F2..0x6F6`) to the **store** list only. Each is proven separately
below, and each is the destination of a SAVE the saved search entry reaches in
order. The load hook is **not** exempted. No count, strict full-C or DOS/native
parity is claimed. `deja` had no earlier trace of these addresses; the 6E82/6E7D/6DE1
method and follower were reused.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` (slot A) | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| store hook, unit `0x25` `+0x0A74..+0x0C76` | 515 bytes, 163 instructions, `retf 4` | `258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938` |

## Caller order and hook path

Same as the earlier notes: the store routine (`+0x0C77`) writes
`es:[[0x49D6] + 2*address + 0x2A00]` for class 1 (`+0x0CBB..+0x0CC9`), then
`+0x0CCE..+0x0CD5` push address, push value, `push cs`, `call +0x0A74`, then
`jmp +0x0E06`. In the hook, `rel = address - 0x6B00 = 0x379, 0x37A, 0x37B`: `cmp 0 / jne`,
`cmp 0x20 / jb`, `cmp 0x70 / ja` (taken), then every exact compare
(`0xB8, 0xBB, 0xBD, 0xBF, 0xC1, 0xC3, 0xF7, 0xF9, 0x100, 0x10C, 0x312, 0x322, 0x324`)
falls through and `cmp 0x326 / jne` lands on the epilogue `+0x0C71`: **42
instructions**, no DS/ES write, no call, only stores to its own `[bp+8]` argument copy.
The exhaustive static follower over all 65,536 addresses (private, not committed)
gives each of the three, and their neighbours `0x6E78` and `0x6E7C`, the empty effect
set; neighbours are still reported HOOK by the model because they are not yet
reached and proven for a flow.

## Change

`PR_REC_ECL_STORE_HOOK_NOOP_FIRST_D` / `_LAST_D` (`0x6E79..0x6E7B`). Tests: the
all-1,024 class-1 store sweep has exactly six OK addresses; for `0x6E78..0x6E7C` the
store status, the window write, the standalone hook model (case NONE, record and
flags untouched) and the load hook (still HOOK) are checked, with `0x6E78` and
`0x6E7C` reporting HOOK.

## Private smoke, supplied-state search entry (documented, not committed)

Hash-pinned slot A, run entry `0x9914` then search entry `0x9975` on the retained
state, merged handlers bound (SAVE, COMPARE, IF, GOTO, AND/OR, EXIT, PICTURE,
PRINT/PRINTCLEAR), explicit `[0x84E2] = 0`, PICTURE DS bytes 0, counting print hook.

| State | Result |
| --- | --- |
| before | stop at dispatch 6: HANDLER/HOOK, PC `0x9996`, SAVE 0 to `0x6E79` (the value was written) |
| after | the SAVEs to `0x6E79`, `0x6E7A`, `0x6E7B` and one more SAVE (to window-2 word `0x9800`) complete; dispatch 10 stops at PC `0x99AF`: HANDLER/HOOK, opcode `0x03` COMPARE, previous opcode `0x09` |

That COMPARE has one class-1 operand, a word load from `0x6E82` (the other operand
is `0x9800`), so the stop is the **load hook at `0x6E82`**; the store-only exception
for `0x6E82` does not cover loads. Not audited here. Supplied-state probe only; no
DOS reachability, display or one-Up parity.
