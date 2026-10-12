# ECL load word: the class-1 hook leaves its flag 0 for 0x6E7F

`pr_rec_ecl_load_word` reports HOOK after every class-1 (window 1) load except the addresses proven plain
([6DD5](ECL_LOAD_HOOK_6DD5.md), [6E7D](ECL_HOOKS_6E7D.md), [6E82](ECL_LOAD_HOOK_6E82.md),
[6DCA](ECL_LOAD_HOOK_6DCA.md)). This note adds **`0x6E7F`** (window-1 word `0x37F`, byte offset `0x6FE`) to the
**load** list. The store exemption there ([ECL_STORE_HOOK_6E7F.md](ECL_STORE_HOOK_6E7F.md)) is separate. No count,
strict full-C or DOS/native parity is claimed. `deja` found only the assigning message; the 6DD5/6E7D/6E82/6DCA method
and private flow follower were reused, and the path below was traced from the original bytes.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` (slot A) | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| load hook, unit `0x25` `+0x07BC..+0x0A73` | 696 bytes, 233 instructions, `retf 6` | `6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451` |

## Original bytes

Caller (load routine, class 1, unit `0x25` `+0x0E41..+0x0E69`): push the address and the by-reference flag pointer,
`call +0x07BC`, `[bp-2] := AX`, `cmp byte [bp-4], 0; jne +0x0E69`, else `[bp-2] := es:[[0x49D6] + 2*address + 0x2A00]`.
Hook for `rel = 0x37F`: a deterministic **62-instruction** path (`+0x07BC..+0x0A71`, `retf 6`): `*flag := 1` at `+0x07C5`; the
compares `0x18, 0x9B, 0xA0` miss, `0xA5 jb` not taken, `0xAC ja` taken, then `0xB8, 0xBB, 0xBD, 0xBF, 0xC1, 0xC3, 0xD8,
0xF7, 0xF9, 0x100, 0x10C, 0x10D, 0x11B, 0x2B1, 0x2B4, 0x2CF, 0x312, 0x33E` all miss (`0x37F` is above the largest case
`0x33E`); `+0x0A64: les di,[bp+6]; mov byte es:[di], 0` (**flag := 0**, `+0x0A67`); `+0x0A6B` returns the never-written
`[bp-2]` as AX (the caller overwrites it). No read of the character record or DS, no ES/DS write other than the flag
byte, no call; no residue reaches the result because the caller reads the window word when the flag is 0. The private
follower over all 65,536 addresses places `0x6E7F` in the quiet class (exactly `[flag := 1, flag := 0]`).

## Change and tests

`PR_REC_ECL_LOAD_HOOK_PLAIN_ADDRESS_E` (`0x6E7F`). Tests: the class-1 load sweep has exactly five OK addresses; `0x6E7E`
and `0x6E80` stay HOOK with their words read; the store at `0x6E7F` is unchanged; a slot value read through the load
core completes at `0x6E7F` and reports HOOK at `0x6E7E`; the earlier store test's load-side expectation at `0x6E7F` now
reads OK.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of
[SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound; explicit host inputs, none from DOS: `[0x84E2] = 0`,
PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result AL2 in {0, 1, 2}.

| AL2 | Before | After |
| --- | --- | --- |
| 0, 1, 2 | HANDLER/HOOK after the ON GOTO at `0x9B90` (class-1 load of `0x6E7F`) | ON GOTO completes to `0x9B9F`, `0x9BB1`, `0x9BC3`; the target SAVE runs (and a GOTO for 0 and 1) and all three converge on PC `0x9BCF` |

**Next stop** (all three): run status UNMODELLED at PC `0x9BCF`, opcode `0x2A` (GETTABLE, handler `+0x0E5E`, 3 operands),
dispatch 39 (38 for AL2 = 2), previous opcode `0x01` (`0x09` for AL2 = 2). Supplied-state probe only: no live DOS RNG,
native search reachability or one-Up parity.
