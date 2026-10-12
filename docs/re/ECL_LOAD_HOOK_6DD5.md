# ECL load word: the class-1 load hook leaves its flag 0 for 0x6DD5

> **Superseded in part (load hook classifier):** the per-address load exemption described below is now one case of `pr_rec_ecl_load_hook_may_effect`, which classifies all 1,024 class-1 addresses ([ECL_LOAD_HOOK_CLASSIFIER.md](ECL_LOAD_HOOK_CLASSIFIER.md)); the evidence and path trace here still stand, but "neighbours stay HOOK" statements no longer hold for plain neighbours.

`pr_rec_ecl_load_word` (lane A, `ecl_helpers.c`) reported HOOK after every
class-1 (window 1) load because the engine first calls the load hook at unit
`0x25` `+0x07BC`, which may override the window value. This note records an
independent proof that for the single address **`0x6DD5`** (window-1 byte offset
`0x5AA`, word index `0x2D5`: the word the saved run entry's COMPARE at PC
`0x9934` reads, [SLOT_A_UP_ECL_GATE.md](SLOT_A_UP_ECL_GATE.md)) the hook leaves
its by-reference flag at 0 and does nothing else, so the caller then reads the
window word. `pr_rec_ecl_load_word` returns **OK** at `0x6DD5` and still HOOK at
every other class-1 address. Lane-A files were edited with the PM's approval.
**No count or credit is claimed**; the value of the word on the live route is
whatever the host holds and is not seeded or asserted.

`deja "Poolrad ECL load word class 1 hook unit 0x25 +0x07BC window 1 0x6DD5"`
found no earlier trace. Reused: ECL_STORE_HOOK_6E82.md (the same method on the
store hook), FUNCTION_LEDGER.md ("load word": class 1 calls `+0x07BC`).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| load hook | unit `0x25` (file `29,907`) `+0x07BC..+0x0A73` = file `0x7C8F..0x7F46`, **696 bytes, 233 instructions**, `retf 6` at `+0x0A71`; the next routine is the store hook at `+0x0A74`. SHA-256 `6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451` (matches the PM's independent hash) |
| frame | `sub sp, 4`; arguments: far pointer to a flag byte at `[bp+6]`/`[bp+8]`, the address word at `[bp+0xA]`; result in `AX` (`[bp-2]`); `retf 6` |
| calls | two near calls to `+0x0756` (at `+0x0981`, `+0x099A`), in other cases only |

## Caller order and flag meaning (byte-checked in load word)

`+0x0E3D..+0x0E69` (class 1): push the address; `lea di,[bp-4]; push ss; push di`
(far pointer to the caller's flag byte `[bp-4]`); `push cs; call +0x07BC`;
`[bp-2] := AX`; `cmp byte [bp-4], 0; jne +0x0E69`: a **nonzero flag keeps the
hook's `AX`**; a **zero flag** falls to `+0x0E56..+0x0E66`, which reads the
window-1 word `es:[[0x49D6] + 2*address + 0x2A00]` into `[bp-2]`; both reach the
epilogue `+0x0F4A`.

## The hook's path for 0x6DD5

`+0x07C2..+0x07C5`: `*flag := 1` (through the argument pointer). `+0x07C9..+0x07CF`:
`[bp+0xA] := address - 0x6B00` (its own copy), giving `rel = 0x2D5`. A compare
chain on `rel` selects a case (the cases handle `rel` values such as `0x18`,
`0x9B`, `0xA0`, the range `0xA5..0xAC`, `0xB8`, and so on up to `0x312` and
`0x33E`; some read the active character record or `[0x5376]`). For `0x2D5` every
compare falls through to `+0x0A64`: `*flag := 0` (`+0x0A67`), then `+0x0A6B`
returns `[bp-2]` (never written on this path, and ignored by the caller because
the flag is 0). The executed path is 62 instructions; the only stores are the two
flag stores and the hook's own stack argument; there is **no read of the
character record or DS, no ES/DS game-state write and no call**.

**Method (tool-assisted static trace).** A small flow follower over the decoded
bytes (private scratch, not in the repository) runs the compare chain with a
concrete address for each of the 65,536 sixteen-bit values, following both sides
of any branch that depends on data it does not know, and records the flag stores,
reads of ES/DS, stores to ES/DS and calls. For `0x6DD5` the single outcome is
"flag := 1, then flag := 0, nothing else". Within `0x6B00..0x6EFF` 995 of the
1,024 addresses have exactly that outcome; the other 29 (`0x6B18`, `0x6B9B`,
`0x6BA0`, `0x6BA5..0x6BAC`, `0x6BB8`, `0x6BBB`, `0x6BBD`, `0x6BBF`, `0x6BC1`,
`0x6BC3`, `0x6BD8`, `0x6BF7`, `0x6BF9`, `0x6C00`, `0x6C0C`, `0x6C0D`, `0x6C1B`,
`0x6DB1`, `0x6DB4`, `0x6DCF`, `0x6E12`, `0x6E3E`) read the record or DS, leave the flag at 1, or
call. **Only `0x6DD5` is exempted here**; the other quiet addresses (for example
`0x6DD4`, `0x6DD6`) deliberately stay HOOK because this slice claims only the
address the saved route needs. This is a static proof for the instruction bytes,
not a captured guest run.

## Change

* `pr_rec_ecl_load_word`: class 1 reads the window word; at
  `PR_REC_ECL_LOAD_HOOK_PLAIN_ADDRESS` (`0x6DD5`) it returns **OK**, otherwise
  HOOK. Classes 0, 2, 3 and 4 are unchanged.
* `test_load_word_6dd5` in `tests/recovered/test_ecl_helpers.c`.

## Verification

`test_load_word_6dd5`: the load at `0x6DD5` for eight host values (0, 1, `0xFF`,
`0x0100`, `0x7FFF`, `0x8000`, `0xFFFF`, `0x1234`) returns OK with exactly the
value the host holds (nothing seeded); `0x6DD4`, `0x6DD6` and several of the
non-quiet addresses stay HOOK with the window value still read; a sweep of all
1,024 class-1 addresses (only `0x6DD5` OK); classes 0 and 2 unchanged; the saved
gate shape, COMPARE of word `0x6DD5` with an immediate zero (`03 01 D5 6D 00 00`),
completes with OK and the six flags equal to a plain comparison of the host word
with 0, while the same COMPARE at `0x6DD6` still stops at HOOK with the flags
untouched; slot value through the load core (OK at `0x6DD5`, HOOK at `0x6DD6`).
Synthetic data only; strict `-Wall -Wextra -Wpedantic -Werror` full build. The
restatement shares my reading of the decode; the correspondence is the tables
above, not a DOS replay.
