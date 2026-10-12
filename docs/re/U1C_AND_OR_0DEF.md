# Unit 0x1c +0x0DEF: the AND / OR handler (opcodes 0x2F, 0x30)

`src/recovered/overlay_u1c_and_or_0def.c` is a **path-complete composition** of
lane A's merged unit `0x25` cores for the handler at unit `0x1c`
`+0x0DEF..+0x0E5D`, the dispatcher target of ECL opcodes `0x2F` (AND) and `0x30`
(OR) (ECL_CONTRACTS.md section 3). It is the handler the byte-checked saved
slot-A run entry reaches at PC `0x996B`
([SLOT_A_UP_ECL_GATE.md](SLOT_A_UP_ECL_GATE.md)). Lane A's route note
([ECL_NEW_GAME_ROUTE.md](ECL_NEW_GAME_ROUTE.md)) lists this handler as
`UNMODELLED`; `deja "Poolrad unit 0x1C +0x0DEF AND OR handler opcode 0x2F"`
found no earlier model. Classification: a composition like lane A's function 22
(SAVE): every callee is a merged core, but the cores carry their own bounded
parts (the class-1 load hook `+0x07BC`, the class-4 pseudo-address tables), so
this is **not** presented as a strict translation by itself. **No count or
credit is claimed.**

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| extent | unit `0x1c` (file `3,398`) `+0x0DEF..+0x0E5D` = file `0x1B35..0x1BA3`, **111 bytes, 43 instructions**, `retf` at `+0x0E5D` (the last byte); the next prologue is `+0x0E5E`, so the ledger bound is exact and there is **no trailing data**. SHA-256 `0c16bd23a8d5f91bf2131b9b8c2ae9cf785a88f919bf2f613ff6abb10f226967` (matches SLOT_A_UP_ECL_GATE.md) |
| frame | `sub sp, 8`: `[bp-2]` destination word, `[bp-4]` second value, `[bp-6]` first value, `[bp-7]` result byte; no arguments, no result |
| calls | six far calls, all unit `0x25` stubs, no near or runtime call |
| DS reads | `[0x6F7F]` (the latched opcode), `[0x7088]` and `[0x7048]` (slot 3 low and high bytes) |
| DS writes | none directly; every effect is inside the callees |

## Paths (in the original's order)

| Offsets | Call or effect | Merged core used |
| --- | --- | --- |
| `+0x0DF5..+0x0DF8` | `lcall 0x2B:0x2A` with 3: the operand loader, count 3 | `pr_rec_ecl_load_operands(state, 3)` |
| `+0x0DFD..+0x0E05` | `lcall 0x2B:0x25` with 1 -> `[bp-6]` | `pr_rec_ecl_slot_value` (function 21), slot 1 |
| `+0x0E08..+0x0E10` | `lcall 0x2B:0x25` with 2 -> `[bp-4]` | slot 2 |
| `+0x0E13..+0x0E20` | push `[0x7088]`, `[0x7048]`; `lcall 0x2B:0x4D` -> `[bp-2]` | `pr_rec_ecl_operand_word` (function 4), slot 3 low/high |
| `+0x0E23..+0x0E3B` | latch `[0x6F7F] = 0x2F`: `result := low byte of (v1 and v2)`; otherwise `result := low byte of (v1 or v2)` (`mov byte [bp-7], al`) | arithmetic here |
| `+0x0E3E..+0x0E47` | push 0, push result (zero-extended); `lcall 0x2B:0x70` | `pr_rec_ecl_compare_words(state, 0, result)` (function 24): sets the six flags |
| `+0x0E4C..+0x0E55` | push `[bp-2]`, push result; `lcall 0x2B:0x52` | `pr_rec_ecl_store_word(state, dest, result)` |
| `+0x0E5A..+0x0E5D` | `mov sp, bp; pop bp; retf` | |

Only the low byte of the 16-bit AND/OR survives, so `0xFFFF or 0x0100` stores
`0x00FF`. Every latch value other than `0x2F` takes the OR path (a latch of
`0x30` is the normal OR case). The compare precedes the store, so the flags
are set even when the store core ends in a hook. The destination is function 4
applied to slot 3's raw low/high bytes whatever its tag.

## Status contract (follows lane A's SAVE model)

A loader result of bounds/argument, or a slot value that is BOUNDS,
UNRESOLVED or ARGUMENT, returns at once with neither the compare nor the store
made (the engine would use a value this model does not have; slot 2 is not
read after a slot-1 stop). A class-1 window read (HOOK) lets the handler
continue and reports HOOK after a successful store; a class-1 destination makes
the store core report HOOK. The report records which slots were read, their
statuses, and whether the compare and store were made. **No status is invented
as success**; a host dispatching opcodes 0x2F/0x30 must treat HOOK and
UNRESOLVED as stops.

## The saved slot-A shape, and the blocker it exposes

The saved run entry's AND at PC `0x996B` is, by SLOT_A_UP_ECL_GATE.md, an AND of
the immediate `0x7F` with the pseudo source `0xC04F`, stored to `0x6E82`
(window 1). The script bytes `2F 00 7F 01 4F C0 01 82 6E` run through this
model give: loader OK (PC advanced by 9), slot 1 = `0x7F` read, slot 2 =
**UNRESOLVED** (address `0xC04F` is class 4 in `pr_rec_ecl_address_class`), so
the handler returns UNRESOLVED with no compare and no store. That is the
precise blocker for closing this handler on the saved route: the **class-4
pseudo-address table of the load-word core** (unit `0x25` `+0x0E0C`, 327 bytes,
`pr_rec_ecl_load_word`). FUNCTION_LEDGER.md function "load word" lists the class-4
addresses (`0xB1`, `0xFB`, `0xFC`, `0x33D`, `0x35F` and the `0xC04B+0..4`
group) and the data-segment bytes they map to (`[0x6F6E]`, `[0x6F6A]`,
`[0x6F6C]`, `[0x6AAF]`, `[0x6AAD]`, `[0x6AAE]`, `[0x6AB0]`, `[0x6AB1]`) but does
not model them or fix the one-to-one pairing for `0xC04F`. Reading that table
is the next audit; until it is, the value of the source, and therefore the byte
stored to `0x6E82` and the flags, are **not claimed**.

## Verification

`tests/recovered/test_overlay_u1c_and_or_0def.c` (synthetic scripts, windows and
flags; no game data): AND and OR of immediate bytes with the exact stored word,
flags and PC advance; every non-`0x2F` latch taking OR; word immediates showing
the low-byte truncation (`0x1234 and 0x00FF`, `0xFFFF or 0x0100`,
`0x0100 or 0x0200` giving zero and the equal/greater-or-equal flags); a window-0
variable source; about 2,000 generated word pairs against plain arithmetic for
the stored word and the six flags of comparing 0 with the result; a class-1
source in slot 1 and in slot 2 (HOOK after the store); class-4 sources in slot
1 and slot 2 (UNRESOLVED, no compare, no store, slot 2 unread after a slot-1
stop); a class-1 destination; truncated operands (BOUNDS); a null state. Strict
`-Wall -Wextra -Wpedantic -Werror` full build. The restatement shares my reading
of the decode; the correspondence is the table above, not a DOS replay.
