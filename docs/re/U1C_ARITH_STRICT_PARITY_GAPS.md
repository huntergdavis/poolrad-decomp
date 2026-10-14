# Unit 0x1C arithmetic: strict C parity dependencies

> **Update:** the `0x6B18` mismatch below describes the model before the
> [active-record composition](U1C_ARITH_CLASS1_RECORD_6B18.md). A supplied
> record now resolves this branch, the [`0x6B9B` branch](ECL_LOAD_HOOK_6B9B_RECORD.md),
> and the [`0x6BA0` signed-byte branch](ECL_LOAD_HOOK_6BA0_SIGNED.md);
> the other dependencies below remain. The saved AL=13 class-2 path now has a
> [measured native DIVIDE/store/return trace](U1C_DIVIDE_AL13_NATIVE_EXECUTION.md),
> and the `0x6B18` record-byte path has a
> [controlled native load/DIVIDE/store trace](U25_LOAD_HOOK_6B18_NATIVE_RECORD.md).
> Its zero-divisor variant now has a
> [measured INT 0 and DOS exit trace](U25_LOAD_HOOK_6B18_ZERO_DIVIDE_NATIVE_FAULT.md).
> A controlled class-1 destination at `0x6B20` now has a
> [measured native window-then-record store trace](U25_STORE_HOOK_6B20_NATIVE_RECORD.md).
> These traces leave the other dependencies unresolved.

The bounded `pr_u1c_op_arith` model follows the 145-byte handler's arithmetic and call order
for its four dispatched opcodes. It cannot yet receive strict full-C credit for the whole
handler. The limiting paths are in the called operand and store helpers, and in the DOS
runtime reached by a zero divisor. This audit names the byte-derived dependencies and one
specific input on which the current C model's stored result can differ from the original.
The [saved AL=13 route audit](U1C_DIVIDE_AL13_ROUTE.md) is a narrower class-2, nonzero-divisor
path and does not remove these general dependencies.

## Exact input and code spans

The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
`START.EXE` hashes `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The following spans include both endpoints. Unit
`0x25` starts at file `0x74D3`; these hashes were recomputed from the original file.

| Routine | Unit offset / file span | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| Arithmetic handler | unit `0x1C:+0x01A3..+0x0233` / `0x0EE9..0x0F79` | 145 | `32f04c0374596652b532b94da3cd8eddfaa90108b8b3b7ed9f754c7591aa7707` |
| Operand loader | `+0x0000..+0x0135` / `0x74D3..0x7608` | 310 | `750610f1ea8c924651ee615595556377352cf441f3fd7e27a01c79ed3f6259cd` |
| Slot value | `+0x0136..+0x01B5` / `0x7609..0x7688` | 128 | `0dd25983c54922e1ebe6f9f1b693f32c6a1c2b69199ddf110fc7d040134ec42b` |
| Operand word | `+0x06AF..+0x06FA` / `0x7B82..0x7BCD` | 76 | `67ac535e265b952c33dbb4c24795e80be662365dbdbfde8afedabeb5faec9f04` |
| Class-1 load hook | `+0x07BC..+0x0A73` / `0x7C8F..0x7F46` | 696 | `6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451` |
| Class-1 store hook | `+0x0A74..+0x0C76` / `0x7F47..0x8149` | 515 | `258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938` |
| Store word | `+0x0C77..+0x0E0B` / `0x814A..0x82DE` | 405 | `3a7c7ed533244cf2eef7ab02bc9ac923f335b52099cf3e6f452fbe38f4e366c5` |
| Load word | `+0x0E0C..+0x0F52` / `0x82DF..0x8425` | 327 | `e8f999e62ee3f04b194908f90ce7c0e15b0478a2af16a3843a1a076c86c67ce5` |

The arithmetic body calls loader `0x2B:0x2A` once, slot value `0x2B:0x25` twice,
operand word `0x2B:0x4D` once, and store word `0x2B:0x52` after a successful
calculation. The relocated live sites and their hashes are in the saved-route audit.
The word assembler is a pure low/high-byte join. The other calls may reach the paths below.

## A concrete class-1 load blocker

For operand tag 1 with address `0x6B18`, slot value calls load word. Original load
word `+0x0E41..+0x0E69` first calls the class-1 hook with a by-reference flag.
The hook sets that flag to 1 at `+0x07C5`. For `rel = 0x6B18 - 0x6B00 = 0x18`,
`+0x07D5..+0x07E7` reads the byte at offset `+0x14` of the active character
record pointed to by DS:`[0x5D92]`, zero-extends it, and jumps to the hook return
at `+0x0A6B` without clearing the flag. Load word sees the flag still 1, so it
returns **that record byte**, not the window-1 word. The [class-1 load classifier](ECL_LOAD_HOOK_CLASSIFIER.md)
correctly marks this address among its 29 potential-effect cases; its other 995
class-1 addresses take the quiet window-read path.

The current `pr_rec_ecl_load_word` instead reads `state->window1[0x18]` and
returns `PR_REC_ECL_WINDOW_HOOK`. `pr_u1c_op_arith` retains that raw word,
calculates with it, and can write a destination before returning `CORE/HOOK`.
For example, with a valid active record byte `7`, a distinct window-1 word `10`,
DIVIDE by literal `3`, and a class-2 destination, the original computes `7 / 3 = 2`
while the current C path computes and stores `10 / 3 = 3` before reporting `HOOK`.
This is a synthetic counterexample derived from the bytes, not a native observed run.
The arithmetic state has no active-record pointer or class-1 load-hook implementation,
so changing only the final status would not repair the already made store.
A fresh, temporary C probe with that script shape and window word 10 returned
`CORE/HOOK`, `v1=10`, `result=3`, `stored=1`, destination word 3, and PC `0x9909`.
It supplied no record byte because this model has no place to supply one.

## Other paths that remain bounded

| Dependency | Original effect | Current C boundary |
| --- | --- | --- |
| Class-1 store hook `+0x0A74` | Store word writes window 1 first, then runs the hook. At `0x6B20` it also writes the low result byte to active record offset `+0x18`; other effect cases write record/DS state or call resident targets. | `pr_rec_ecl_store_word` writes the window and returns `HOOK` for 96 potential-effect addresses; it does not call the existing standalone [store-hook model](ECL_STORE_HOOK_0A74.md), which itself needs record state and callbacks. The other 928 class-1 addresses are byte-classified quiet. |
| Class-4 load and store | Load word has nine known pseudo-address results, with other cases returning an unwritten local. Store word's `+0x0D14..+0x0E06` includes no-op addresses and DS writes; for example destination `0x03DE` writes a word to DS:`0x84CC`. | Known load results are modeled; unwritten results stay `UNRESOLVED`. Class-4 store currently always returns `UNRESOLVED` and does not perform even the `0x03DE` write. The [class-4 load audit](ECL_LOAD_WORD_CLASS4.md) and original store bytes establish these boundaries. |
| Class-3 destination | Store word `+0x0CF5..+0x0D0A` writes the low result byte into the ECL script buffer through its far pointer. | The model performs this only when the host supplies `script_rw`; otherwise it returns `UNRESOLVED`. |
| Operand loader and slot forms | The loader can unpack string tag `0x80` or fetch string tag `0x81`; slot value can reach load-word classes 0 through 4. Unsupported tags and some pseudo-addresses expose unwritten locals. | The loader has bounded buffers and unresolved string/address cases; `slot_value` reports `UNRESOLVED` where original stack residue is not determined. |
| Zero divisor | Handler `+0x020B..+0x0213` sets `DX=0` and performs unsigned word `DIV`. Divisor zero raises INT 0 before the store. | `pr_u1c_op_arith` returns `DIVIDE_ERROR` and records 200, but does not enter the DOS runtime or reproduce its state changes and termination. |
| Invalid opcode latch | Outside 4..7, the handler passes its unwritten `[bp-9]` to store word. | The host supplies `stack_residue`; dispatcher reachability is only established for 4..7. |

The saved run-007 RAM snapshot (SHA-256
`e756eb4ff9a94fc63d434e57aa5d9c790ff61786b509edf87f38f1971aa65c80`)
has INT 0 vector `131C:00DF`. The first native instructions set AX to `0x00C8`,
pop CX and BX, set DS to `0x14A0`, enable interrupts, then store AX to
DS:`[0x441C]` at `131C:00F3`. The 64-byte runtime entry span at that vector
hashes `2e0d02cf1da6ef322f33ac78239cb6e20711c9be3ce83e29563a2a1bad2c2a5e`.
That earlier audit identified an installed path. A later
[controlled zero-divisor fault trace](U25_LOAD_HOOK_6B18_ZERO_DIVIDE_NATIVE_FAULT.md)
observed INT 0 entry, error word 200, and the DOS terminate call. Runtime
error handling is not represented in the arithmetic state.

## Classification and next interface

No strict full-C or new function-count claim follows from this audit. The
`0x6B18` path alone is a counterexample to whole-handler parity, even with a
class-2 destination and nonzero divisor. A correct implementation must pass
the active record and any other hook state into load-word evaluation **before**
the arithmetic or store, then compose the existing store-hook body and its
resident callbacks where needed. A narrow class-4 or quiet-address fix would
not remove that blocker. This branch records the dependency rather than adding
a callback that merely moves the unresolved behavior across an interface.
