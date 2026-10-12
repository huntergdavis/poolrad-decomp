# ECL load word: the class-1 hook leaves its flag 0 for 0x6DCA

`pr_rec_ecl_load_word` reports HOOK after every class-1 (window 1) load except the addresses
proven plain ([6DD5](ECL_LOAD_HOOK_6DD5.md), [6E7D](ECL_HOOKS_6E7D.md),
[6E82](ECL_LOAD_HOOK_6E82.md)). This note adds **`0x6DCA`** (window-1 word `0x2CA`, byte offset
`0x594`) to the **load** list only; the store hook there is **not** exempted. No count, strict
full-C or DOS/native parity is claimed. `deja` had no earlier trace; the 6DD5/6E82 method and the
private flow follower were reused.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` (slot A) | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| load hook, unit `0x25` `+0x07BC..+0x0A73` | 696 bytes, 233 instructions, `retf 6` | `6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451` |

## Caller order

Load routine (unit `0x25` `+0x0E41..+0x0E69`), class 1: push the address and the by-reference flag
pointer, `call +0x07BC`, `[bp-2] := AX`, `cmp byte [bp-4], 0; jne +0x0E69`, else
`[bp-2] := es:[[0x49D6] + 2*address + 0x2A00]` (the window-1 word). With flag 0 the caller reads
the host's window-1 word at index `0x2CA`.

## Hook path for 0x6DCA (rel = 0x6DCA - 0x6B00 = 0x2CA), traced independently

`*flag := 1` at `+0x07C5`; then the compare chain (62 instructions through the `retf 6` at
`+0x0A71`, 59 to `+0x0A6B`): `cmp 0x18`, `0x9B`, `0xA0` miss; `cmp 0xA5 / jb` not taken,
`cmp 0xAC / ja` taken; then `0xB8, 0xBB, 0xBD, 0xBF, 0xC1, 0xC3, 0xD8, 0xF7, 0xF9, 0x100,
0x10C, 0x10D, 0x11B, 0x2B1, 0x2B4` all miss; **`cmp 0x2CF / je` is not taken** (`0x2CA` is below
it, so the low-byte branch at `+0x09AD` is not entered) and the path goes to `+0x0A42`;
`cmp 0x312` and `cmp 0x33E` miss; `+0x0A64: les di,[bp+6]; mov byte es:[di], 0`
(**flag := 0**, `+0x0A67`); `+0x0A6B` returns the unwritten `[bp-2]`. No read of the character
record or DS, no ES/DS write other than the flag byte, no call. The private follower over all
65,536 addresses puts `0x6DCA` (and `0x6DC9`, `0x6DCB`) in the quiet class (exactly
`[flag:=1, flag:=0]`); 29 of the 1,024 class-1 addresses are non-quiet and `0x6DCA` is not among
them. `0x6DC9` and `0x6DCB` stay HOOK in the model because they are not reached and proven for a flow.

## Change

`PR_REC_ECL_LOAD_HOOK_PLAIN_ADDRESS_D` (`0x6DCA`). Tests: the all-1,024 class-1 load sweep has
exactly four OK addresses; `0x6DC9`/`0x6DCB` stay HOOK with their words read; the store word at
`0x6DCA` still reports HOOK; a synthetic COMPARE of `0x6DCA` with an immediate byte completes with the
flags of the window word as held, and the same shape at `0x6DCB` reports HOOK with the flags untouched.

## Private smoke, supplied-state search entry (documented, not committed)

Hash-pinned slot A, run entry `0x9914` then search entry `0x9975` on the retained state; handlers
bound: SAVE, COMPARE, IF, GOTO, AND/OR, EXIT, PICTURE, PRINT/PRINTCLEAR, ON GOTO/GOSUB,
PARTYSTRENGTH and RANDOM; explicit host inputs: `[0x84E2] = 0`, PICTURE DS bytes 0, an **empty**
party list, RANDOM's AL = 0 (none of these are saved data).

| State | Result |
| --- | --- |
| before | dispatch 25: HANDLER/HOOK, PC `0x9B46` after the COMPARE at `0x9B40` (class-1 word load `0x6DCA`) |
| after | the COMPARE completes; IF (`0x16`) at `0x9B46`, COMPARE at `0x9B50`, IF at `0x9B56`, COMPARE at `0x9B60`, IF `0x1A` at `0x9B66`, then EXIT at `0x9B67`: **run status OK**, stopped by `[0x442E]`, 31 dispatches, final PC `0x9B68`, `[0x5E24] = 1`, `[0x5E25] = 0x11` |

So under these supplied inputs the model runs the search entry to its EXIT with no HOOK,
UNRESOLVED or unmodelled stop remaining. The route depends on the supplied party list and RANDOM
result: this is a supplied-state probe, not proof that gameplay calls it, and no DOS reachability,
display or one-Up parity is claimed.
