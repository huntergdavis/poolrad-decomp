# ECL store word: the class-1 hook returns with no effect for 0x6E82

`pr_rec_ecl_store_word` (lane A, `ecl_helpers.c`) reported HOOK after every
class-1 (window 1) store because the engine then calls the hook at unit `0x25`
`+0x0A74` ([ECL_STORE_HOOK_0A74.md](ECL_STORE_HOOK_0A74.md)). This note records
an independent proof that, for the single address **`0x6E82`** (window-1 byte
offset `0x382`, the destination of the saved AND at PC `0x996B`,
[U1C_AND_OR_0DEF.md](U1C_AND_OR_0DEF.md)), the hook runs and returns without a
persistent game-state write (DS or ES) and without a call, and the narrow change that follows: `pr_rec_ecl_store_word`
returns **OK** after its window write at `0x6E82` and still returns HOOK at
every other class-1 address. Lane-A files were edited with the PM's approval.
**No count or credit is claimed**, no strict full-C claim, and no value is
seeded: the stored value is whatever the caller passes.

`deja "Poolrad ECL store word class 1 hook 0x0A74 window 1 store 0x6E82"` found
no earlier trace beyond the 0A74 note. Reused: ECL_STORE_HOOK_0A74.md (the case
list), `overlay_ecl_store_hook` (the standalone case model).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| hook | unit `0x25` (file `29,907`) `+0x0A74..+0x0C76` = file `0x7F47..0x8149`, **515 bytes, 163 instructions**, `retf 4`; SHA-256 `258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938` (matches the PM's independent hash) |
| store word | unit `0x25` `+0x0C77..` (405 bytes), class-1 path `+0x0CB7..+0x0CD8` |

## Store-word order, byte-checked (class 1)

`+0x0CB7`: class byte = 1. `+0x0CBB..+0x0CC9`: `es:[[0x49D6] + 2*address + 0x2A00]
:= value` (the window-1 word). `+0x0CCE..+0x0CD5`: push `address`, push `value`,
`push cs; call +0x0A74`. `+0x0CD8: jmp +0x0E06` (the epilogue). So the window
write comes first, then the hook with the same two arguments; nothing in the store
core uses the hook's result.

## The hook's control path for 0x6E82

`+0x0A7A..+0x0A83`: `[bp+8] := address - 0x6B00` (the hook works on its own copy
of the argument), giving `rel = 0x382`. Then a chain of unsigned compares on that
value decides the case; the cases are `rel = 0`, `0x20..0x70`, `0xB8`, `0xBB`,
`0xBD`, `0xBF`, `0xC1`, `0xC3`, `0xF7`, `0xF9`, `0x100`, `0x10C`, `0x312`,
`0x322`, `0x324`, `0x326`. For `0x382` every compare falls through (`+0x0A86`
`== 0`, `+0x0A99..+0x0A9E` the `0x20..0x70` range, `+0x0AC1` `0xB8`, `+0x0AE0`,
`+0x0AF4`, `+0x0B08`, `+0x0B1C`, `+0x0B30`, `+0x0B44`, `+0x0B58` `0xF9`,
`+0x0B6C` `0x100`, `+0x0BA1` `0x10C`, `+0x0BFA` `0x312`, `+0x0C0A` `0x322`,
`+0x0C2D` `0x324`, `+0x0C50` `0x326`) and the last `jne` lands on the epilogue
`+0x0C71`. The executed path is 42 instructions, all compares, conditional jumps
and the prologue moves: **no DS or ES (game-state) write and no call**. The only stores it makes are to its
own stack frame: the prologue pushes and `+0x0A80`, which stores the adjusted
address back to its stack argument `[bp+8]` (`+0x0A7A` loads the address, `+0x0A7D`
subtracts `0x6B00`, `+0x0A80` stores it). That is a mutation of the hook's local
argument copy, not of caller or game state.

**Method (tool-assisted static trace).** A small flow follower over the decoded
bytes (private scratch, not in the repository) runs the compare chain with a
concrete address for each of the 65,536 sixteen-bit values and records any
instruction that stores to ES or an absolute DS address or calls (stores to the
hook's own stack argument `[bp+8]` are tracked as the value flow, not counted as effects), exploring both
sides of any branch that depends on the value word. Result: within
`0x6B00..0x6EFF` exactly **96** addresses can do something (`0x6B00`,
`0x6B20..0x6B70`, `0x6BB8`, `0x6BBB`, `0x6BBD`, `0x6BBF`, `0x6BC1`, `0x6BC3`,
`0x6BF7`, `0x6BF9`, `0x6C00`, `0x6C0C`, `0x6E12`, `0x6E22`, `0x6E24`, `0x6E26`),
the rest of the window (including `0x6E82`) cannot. That agrees with
`overlay_ecl_store_hook`'s case list, which the test below sweeps. This is a
static proof for the instruction bytes, not a captured guest run.

## Change

* `pr_rec_ecl_store_word`: class 1 stores the window word; at address
  `PR_REC_ECL_STORE_HOOK_NOOP_ADDRESS` (`0x6E82`) it returns **OK**, otherwise
  HOOK. Classes 0 and 2, class 3 and class 4 are unchanged.
* The other no-op addresses (for example `0x6E81`, `0x6E83`) are deliberately
  **not** exempted: the trace shows they also make no game-state write and no call, but this slice only
  claims the proven address the saved route needs.
* `tests/recovered/test_ecl_helpers.c` `test_store_word_6e82`; the saved-shape AND
  test in `test_overlay_u1c_and_or_0def.c` now expects OK.

## Verification

`test_store_word_6e82`: the window write at `0x6E82` for several values
including 0 and `0xFFFF` with the neighbours untouched; a sweep of all 1,024
class-1 addresses (exactly `0x6E82` OK, every other HOOK); explicit HOOK checks
for `0x6E80`, `0x6E81`, `0x6E83`, `0x6E84`, the edges of the effectful ranges and
`0x6EFF`; the standalone hook model at `0x6E82` (case NONE, no call-out, record
and flags untouched); and a sweep of the standalone model over the whole window
showing exactly the 96 addresses above are not "no case". The merged AND/OR
saved shape (`0x7F` AND the host-supplied `[0x6AB1]` stored to `0x6E82`) now
completes with OK for every byte value 0..255. Synthetic data only; strict
`-Wall -Wextra -Wpedantic -Werror` full build. The restatement shares my reading
of the decode; the correspondence is the table above, not a DOS replay.
