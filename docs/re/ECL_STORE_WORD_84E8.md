# ECL store word, class 0: addresses 0x49FD and 0x49FE set [0x84E8]

`pr_rec_ecl_store_word` (lane A, `ecl_helpers.c`) already wrote the window-0
word for class-0 addresses, but the engine's store-word core also sets a data
segment byte for two of them. FUNCTION_LEDGER.md recorded this as "also set
`[0x84E8]` (not modelled)". This note verifies the bytes independently and the
core now models exactly that side effect. Lane-A files were edited with the PM's
approval. **No count or credit is claimed**, no strict full-C claim, and no claim
about a live replay: the saved run entry's SAVEs at ECL PCs `0x9927` and
`0x992E` ([SLOT_A_UP_ECL_GATE.md](SLOT_A_UP_ECL_GATE.md)) target these two
addresses, and the model shows what the core does for that shape.

`deja "Poolrad ECL store word class 0 addresses 0x49FD 0x49FE sets 0x84E8"` found
no earlier trace beyond the ledger line. Reused: FUNCTION_LEDGER.md (the store word
entry), ECL_STORE_HOOK_6E82.md (core layout).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| store word | unit `0x25` (file `29,907`) `+0x0C77..+0x0E0B` = file `0x814A..0x82DE`, **405 bytes** (the ledger bound; 141 instructions), `retf 4`; SHA-256 `3a7c7ed533244cf2eef7ab02bc9ac923f335b52099cf3e6f452fbe38f4e366c5` |
| frame | `sub sp, 2`; arguments: value `[bp+6]`, address `[bp+8]`; `retf 4`; `+0x0C7D..+0x0C81` calls the classifier `+0x06FB` (merged) |

## The class-0 path, byte-checked

`+0x0C8A..+0x0C8C`: class byte 0 (else `jne +0x0CB7`). `+0x0C8E..+0x0C9C`: `es:[[0x49D2]
+ 2*address + 0x6E00] := value` (the window-0 word; the displacement wraps to byte
offset `2*(address - 0x4900)` for a class-0 address). Then `+0x0CA1`: `cmp word
[bp+8], 0x49FD; je +0x0CAF`; `+0x0CA8`: `cmp word [bp+8], 0x49FE; jne +0x0CB4`;
`+0x0CAF`: `mov byte [0x84E8], 1`; `+0x0CB4`: `jmp +0x0E06` (epilogue). So the window
write comes first, the flag is set for either address and for any value, and no
other class-0 address touches it.

A byte scan of every unit for direct-address stores to `[0x84E8]`
(`C6 06 E8 84 imm`, `A2 E8 84`, `FE 06/0E E8 84`) finds exactly two: this `:= 1` at unit
`0x25` `+0x0CAF`, and a `:= 0` at unit `0x1c` `+0x2D42`. The scan sees direct stores
only (an inference, not a trace). The model never clears the byte.

## Change

* `pr_rec_ecl_loader_state` gains `b84e8`.
* `pr_rec_ecl_store_word`: after the class-0 window write, an address of `0x49FD`
  or `0x49FE` sets `b84e8 := 1`. Classes 1 to 4 are unchanged and never touch it.
* `test_store_word_84e8` in `tests/recovered/test_ecl_helpers.c`.

## Verification

`test_store_word_84e8`: both addresses with distinct values, including 0 and
`0xFFFF` (the window write and the flag, and the other address's word
untouched); a sweep of the whole window (`0x4900..0x4CFF`: exactly `0x49FD` and
`0x49FE` set the flag); neighbours and other class-0 addresses (`0x49FC`, `0x49FF`,
`0x49FB`, `0x4A00`, `0x49FA`, `0x4900`, `0x4CFF`) write the window and leave the
flag unchanged in both directions (0 stays 0, 1 is not cleared); classes 1, 2 and
4 do not touch it; the saved SAVE shape through `pr_rec_ecl_op_save` (immediate
byte to `0x49FD` and to `0x49FE` sets it, the same SAVE to `0x49FC` does not); the
null guard. Synthetic data only; strict `-Wall -Wextra -Wpedantic -Werror` full
build. The restatement shares my reading of the decode; the correspondence is the
bytes quoted above, not a DOS replay.
