# ECL load word: the class-4 pseudo-address table (unit 0x25 +0x0E0C)

`pr_rec_ecl_load_word` (lane A, `ecl_helpers.c`; FUNCTION_LEDGER.md "load word")
used to return UNRESOLVED for every class-4 address. This note records the
independent decode of the class-4 branch and the change that resolves exactly the
byte-derived pairs. Lane-A files were edited with the PM's approval; no other
worker owns them. **No count or credit is claimed**, and no strict full-C claim:
the class-1 load hook `+0x07BC` is still not read.

`deja "Poolrad ECL load word class 4 pseudo address 0xC04F unit 0x25 +0x0E0C"`
found no earlier trace. Reused: FUNCTION_LEDGER.md (the listed addresses),
U1C_AND_OR_0DEF.md and SLOT_A_UP_ECL_GATE.md (the `0xC04F` source at PC `0x996B`).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| routine | unit `0x25` (file `29,907`) `+0x0E0C..+0x0F52` = file `0x82DF..0x8425`, **327 bytes, 125 instructions**, `retf 2` at `+0x0F50`; SHA-256 `e8f999e62ee3f04b194908f90ce7c0e15b0478a2af16a3843a1a076c86c67ce5` |
| class-4 branch | `+0x0EA0..+0x0F49` (170 bytes; SHA-256 `904a4bc2b5cf73f668fc4cf3ecdce506be341c170710c17edfd9c0f8c6c1a4c3`), the dispatch on the class returned by `+0x06FB`, value 4 |
| frame | `sub sp, 6`: `[bp-2]` result word, `[bp-3]` class, `[bp-4]` hook flag; one word argument `[bp+6]`; result in `AX` |
| calls | near `+0x06FB` (classify, merged, full translation) and near `+0x07BC` (class 1 only, unread) |

## The class-4 branch, byte-verified (argument `[bp+6]`, unsigned)

| Condition | Result word `[bp-2]` | Offsets |
| --- | --- | --- |
| address `< 0xC04B` and `= 0x00B1` | word `[0x6F6E]` | `+0x0EB1..+0x0EB9` |
| `= 0x00FB` | word `[0x6F6A]` | `+0x0EBE..+0x0EC6` |
| `= 0x00FC` | word `[0x6F6C]` | `+0x0ECB..+0x0ED3` |
| `= 0x033D` | byte `[0x6AAF]`, zero-extended | `+0x0ED8..+0x0EE2` |
| `= 0x035F`, or any other address `< 0xC04B` | **not written**: the local is returned uninitialized | `+0x0EE7..+0x0EEC` |
| address `>= 0xC04B`: `off := address - 0xC04B` (16-bit), `off = 0` | byte `[0x6AAD]`, sign-extended (`cbw`) | `+0x0EFF..+0x0F03` |
| `off = 1` | byte `[0x6AAE]`, sign-extended | `+0x0F0D..+0x0F11` |
| `off = 2` | byte `[0x6AAF]` zero-extended, shifted right 1 | `+0x0F1B..+0x0F22` |
| `off = 3` | byte `[0x6AB0]`, zero-extended | `+0x0F2C..+0x0F31` |
| `off = 4` | byte `[0x6AB1]`, zero-extended | `+0x0F3B..+0x0F40` |
| `off = 0xE`, or any other `off` | **not written** (uninitialized) | `+0x0F45..+0x0F48` |

All paths join at `+0x0F4A` (`mov ax, [bp-2]`; `retf 2`). `0xC04F` is
`0xC04B + 4`, so it reads **`[0x6AB1]`**: the byte unit `0x3bd` entry 4 stores
(U3BD_CELL_ENTRY4.md) and that unit `0x68` `+0x073B` writes (U68_COORDINATE_WRITER_073B.md).
That is a byte-derived pairing. **The value of `[0x6AB1]` at the saved run's PC
`0x996B` is live-state dependent and is not claimed**; nothing here seeds it
from an expected move result.

## Change

* `pr_rec_ecl_loader_state` gains `w6f6e`, `w6f6a`, `w6f6c`, `b6aad`, `b6aae`,
  `b6aaf`, `b6ab0`, `b6ab1`: the host supplies what the engine holds.
* `pr_rec_ecl_load_word` resolves the nine pairs above (returning OK with the
  extension rules) and keeps **UNRESOLVED** for `0x035F`, `0xC04B + 0xE` and
  every other class-4 address, because the engine returns an unwritten local
  there. `*value` is untouched on UNRESOLVED.
* Lane-A tests that used `0x00B1` as the "unresolved class-4 source" now use
  `0x00B2`, which stays unresolved. Store word's class-4 table is **not** modelled.

## Verification

`test_load_word_class4` in `tests/recovered/test_ecl_helpers.c`: each exact pair
with distinct values; the full 16-bit words; zero extension of the byte sources
(`0xFF` gives `0x00FF`); all 256 values of the party bytes with the sign
extension, of `[0x6AAF] >> 1`, of `[0x6AB0]` and `[0x6AB1]`; the boundaries
`0xC04A`, `0xC050`, `0xC059`, `0xFFFF`, `0x035F` and ten near-miss addresses
(all unresolved with `*value` untouched); a sweep of all 65,536 addresses
checking that exactly the nine pairs resolve and every other class-4 address is
UNRESOLVED; the pairing through `pr_rec_ecl_slot_value` and the SAVE handler
(`SAVE [0x4A10] := [0xC04F]` stores the host's byte; `0xC059` stays
unresolved); null guards. Synthetic data only. The restatement shares my reading
of the decode; the correspondence is the table above, not a DOS replay.
