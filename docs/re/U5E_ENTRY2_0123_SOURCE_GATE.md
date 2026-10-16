# U5E entry 2 +0123 complete local body

The original DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U5E entry 2 at `+0123..+0190` is file `0x1291E..0x1298B`, 110 bytes,
SHA-256 `adb2fe5a76a3a1ebad49a09614e60fc89080130eafab392b01bcfb2c70ba4f1e`.
The 36 aligned starts form one body with a single far call at `+0178`,
one outer `RETF 4` at `+018E`, and next boundary `+0191`. The callable
ledger classifies the one far caller and three near callers as game-evidenced
but provisional. No semantic name is assigned to the DS global bytes.

| Original span | Local C effect |
| --- | --- |
| `+0123..+013D` | Read character byte `ES:[DI+011C]` into a BP local; test character byte `+010E`. |
| `+013F..+014D` | On zero test, add word `ES:[DI+06E4]` through the far pointer at `DS:49D6` and keep its low result byte. |
| `+0150..+016C` | Write `DS:681D=1`; clamp the local unsigned byte to 1..96, double AX, publish AL at `DS:681A`. |
| `+016F..+0178` | Push AX with low byte `12`, the original far argument pointer, then call `00B0:002F`. |
| `+017D..+018E` | Write `DS:681D=0`; load the live `DS:681A` byte, store/reload it through BP local, restore frame, and return far removing four argument bytes. |

The source uses live 14-word CPU state and 1 MiB RAM with 20-bit physical
wrapping, 16-bit effective-offset wrapping, contiguous word bytes, original
stack order, arithmetic flags at the call boundary, and the outer return.
The host supplies the loaded callee CS and a full-state callee callback;
the original `00B0` operand does not establish its relocated CS. The
callback owns callee return, cleanup and side effects. Source assumes the
loaded U5E body is immutable.

The [callable ledger](overlay_function_ledger.csv) supplies the entry ID,
caller count and provisional classification. The [character field trace](../formats/DOS_CHARACTER.md)
supports that the input is a character pointer; the `+011C` and `+010E`
field names remain raw here. Before this translation, two bounded `deja`
ownership queries timed out without a reusable indexed result. A fresh
main source search found only the ledger/classification names for this
entry; PM cleared the disjoint ownership in i8320.

The synthetic C test checks the skipped and taken addition, both clamp
bounds, exact far-call frame, callback mutation of the later DS read, and
the outer `RETF 4`. The source checker locks the original 110-byte window,
callee operand and return. Original-byte full-state parity and a native
reached frame are pending the PM host-native slot. This bounded source
candidate does not change the strict full-C count of 82.
