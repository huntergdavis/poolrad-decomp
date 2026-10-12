# ECL load word: the class-1 hook leaves its flag 0 for 0x6E82

`pr_rec_ecl_load_word` reports HOOK after every class-1 (window 1) load except the
addresses proven plain ([6DD5](ECL_LOAD_HOOK_6DD5.md), [6E7D](ECL_HOOKS_6E7D.md)).
This note adds **`0x6E82`** (window-1 word `0x382`, byte offset `0x704`) to the
**load** list. The store exemption for the same address
([ECL_STORE_HOOK_6E82.md](ECL_STORE_HOOK_6E82.md)) is separate and unchanged. No count,
strict full-C or DOS/native parity is claimed. `deja` had no earlier trace; the
6DD5/6E7D method and private follower were reused.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` (slot A) | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| load hook, unit `0x25` `+0x07BC..+0x0A73` | 696 bytes, 233 instructions, `retf 6` | `6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451` |

## Caller order (load routine, class 1: unit `0x25` `+0x0E41..+0x0E69`)

`push [bp+6]` (address), `lea di,[bp-4]; push ss; push di` (by-reference flag),
`push cs; call +0x07BC`; `[bp-2] := AX`; `cmp byte [bp-4], 0`; `jne +0x0E69`
(use the hook's AX); otherwise `[bp-2] := es:[[0x49D6] + 2*address + 0x2A00]` (the
window-1 word). The hook runs first, and with flag 0 the window word is read.

## Hook path for 0x6E82 (rel = 0x382)

62 instructions from `+0x07BC` through the `retf 6` at `+0x0A71` (59 to `+0x0A6B`),
identical in shape to `0x6E7D`: `*flag := 1` at `+0x07C5`; every exact and range
compare falls through (`0x382` is above the last case, `0x33E` at `+0x0A51`);
`+0x0A64: les di,[bp+6]; mov byte es:[di], 0` (**flag := 0**, `+0x0A67`); `+0x0A6B`
returns the unwritten `[bp-2]`. No read of the character record or DS, no ES/DS write
other than the flag byte, no call. The private flow follower over all 65,536
addresses puts `0x6E82` (and `0x6E7C`, `0x6E81`, `0x6E83`) in the quiet class
(exactly `[flag:=1, flag:=0]`); 29 of the 1,024 class-1 addresses are non-quiet and
`0x6E82` is not among them.

## Change

`PR_REC_ECL_LOAD_HOOK_PLAIN_ADDRESS_C` (`0x6E82`); `pr_rec_ecl_load_word` returns OK
with the host's window word. Tests: the all-1,024 class-1 load sweep has exactly three
OK addresses; `0x6E81`/`0x6E83` stay HOOK with their words read; a synthetic COMPARE of
`0x6E82` with an immediate byte completes with the flags of the window word as held,
and the same shape at `0x6E81` reports HOOK with the flags untouched.

## Private smoke, supplied-state search entry (documented, not committed)

Hash-pinned slot A, run entry `0x9914` then search entry `0x9975` on the retained
state, merged handlers bound, explicit `[0x84E2] = 0`, PICTURE DS bytes 0, counting
print hook.

| State | Result |
| --- | --- |
| before | HANDLER/HOOK at PC `0x99AF`, COMPARE (`0x03`), load of `0x6E82` |
| after | COMPARE completes, then the IF/GOTO chain; stop at dispatch 13: run status UNMODELLED, PC `0x99C9`, opcode `0x25` (ON GOTO, handler `+0x18F8`), previous opcode `0x01` (GOTO) |

This is a supplied-state probe: no DOS reachability, display or one-Up parity.
