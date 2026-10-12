# ECL store word: the class-1 hook returns with no effect for 0x6DE1

`pr_rec_ecl_store_word` reported HOOK after every class-1 store except `0x6E82`
and `0x6E7D` ([6E82](ECL_STORE_HOOK_6E82.md), [6E7D](ECL_HOOKS_6E7D.md)). This
note adds **`0x6DE1`** (window-1 word `0x2E1`, byte offset `0x5C2`) to the
**store** list only. The load hook is **not** exempted at `0x6DE1`. No count or
credit is claimed. `deja` had no earlier trace of this address beyond the notes
above; the 6E82/6E7D method, spans and follower were reused.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` (slot A) | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| store hook, unit `0x25` `+0x0A74..+0x0C76` | 515 bytes, 163 instructions, `retf 4` | `258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938` |

## Caller order (store routine, unit `0x25` `+0x0C77`)

Class byte 1 (`+0x0CB7`): `es:[[0x49D6] + 2*address + 0x2A00] := value`
(`+0x0CBB..+0x0CC9`), then `+0x0CCE..+0x0CD5` push address, push value, `push cs`,
`call +0x0A74`, then `jmp +0x0E06` (epilogue). The hook runs after the window
write and nothing follows it.

## Hook path for 0x6DE1 (rel = 0x6DE1 - 0x6B00 = 0x2E1)

Deterministic, **42 instructions** from `+0x0A74` to the epilogue `+0x0C71`:
`cmp 0 / jne`, `cmp 0x20 / jb`, `cmp 0x70 / ja` (taken), then the exact compares
`0xB8, 0xBB, 0xBD, 0xBF, 0xC1, 0xC3, 0xF7, 0xF9, 0x100, 0x10C, 0x312, 0x322,
0x324, 0x326` all fall through (`0x2E1` is above `0x10C` and below `0x312`), and
the final `jne` lands on `+0x0C71`. No DS/ES write and no call; the only stores
go to its own `[bp+8]` argument copy. An exhaustive static flow follower over all
65,536 addresses (private, not committed) gives `0x6DE1` (and `0x6DD2`, `0x6DD3`,
`0x6DE0`, `0x6DE2`) the empty effect set.

## Change

`PR_REC_ECL_STORE_HOOK_NOOP_ADDRESS_C` (`0x6DE1`); `pr_rec_ecl_store_word` returns
OK after the window write there. Tests (`test_ecl_helpers.c`): the all-1,024
class-1 store sweep now has exactly three OK addresses; the standalone hook model
at `0x6DE1` (case NONE, record and flags untouched); neighbours `0x6DE0`/`0x6DE2`
store HOOK; `pr_rec_ecl_load_word(0x6DE1)` still HOOK.

## Private smoke, supplied-state search entry (documented, not committed)

Hash-pinned slot A, merged SAVE/COMPARE/IF/GOTO/AND-OR/EXIT/PICTURE handlers
bound, every other opcode unmodelled, host inputs `[0x84E2]=0`, `[0x49F9]`,
`[0x49FA]`, `[0x84E0]`, `[0x84E3]` all 0, PICTURE hooks counting stubs returning
0. Run entry `0x9914` first (10 dispatches, final PC `0x9975`), then
`pr_rec_ecl_run(0x9975)` on the retained state:

| Step | Result |
| --- | --- |
| before this change | stop at dispatch 1: HANDLER/HOOK, SAVE `0xFF` to `0x6DE1` (PC `0x997B` after operands) |
| after | SAVE `0xFF` to `0x6DE1` OK (word `0x00FF`); PICTURE (opcode `0x0E`, PC `0x997B`) OK; SAVE `0x00` to `0x4A1F` OK (window-0 word stays 0); then UNMODELLED at PC `0x9984`, opcode `0x12`, previous opcode `0x09` |

PICTURE's operand is a literal byte `0xFF` (slot-value tag 0, low byte), so
`slot_value(1)` takes the literal path and runs **no load hook**; the id is `0xFF`,
the clear path is taken, no picture hook is called (box, show, view all 0), and
`[0x84C8]`, `[0x84C9]`, `[0x84E0]`, `[0x84E1]`, `[0x84E3]` are 0. State otherwise
unchanged (`[0x84E8]=1`, `[0x5E24]=1`, `[0x5E25]=0x11`). The next gate is the
handler for opcode `0x12`, not a hook. This is a supplied-state probe: no DOS
reachability or one-Up parity is claimed.
