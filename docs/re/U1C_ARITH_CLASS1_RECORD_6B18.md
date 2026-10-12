# Arithmetic operand 0x6B18: active-record byte composition

The unit `0x1C:+0x01A3` arithmetic model now composes the class-1 load-hook
value at address `0x6B18` when its caller supplies the active character record.
This repairs the concrete DIVIDE counterexample in
[the strict parity audit](U1C_ARITH_STRICT_PARITY_GAPS.md) for that address.
It does not establish strict full-C parity for the whole handler: the other 28
load-hook effect addresses, store hooks, class-4 stores and the divide-fault
runtime still have bounded or unresolved paths.

## Source evidence

The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The unit `0x25:+0x07BC..+0x0A73` load hook is 696 bytes at file
`0x7C8F..0x7F46`, SHA-256
`6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451`.
The independently checked span in the prior audit was rechecked against these
original bytes here. `deja` CLI could not open its read-only cache lock; MCP
recall for this address and arithmetic route returned no matching session.

At `+0x07C5`, the hook sets its by-reference flag to 1. For relative address
`0x18`, `+0x07D5..+0x07E7` compares the address, loads the far active-record
pointer from DS:`[0x5D92]`, reads byte `record[0x14]` into AL, zeroes AH,
saves AX as its return word, and jumps to `+0x0A6B`. That jump skips the
flag-clear at `+0x0A67`. The load-word caller at `+0x0E41..+0x0E69` calls
the hook, saves AX, and reads the window-1 word only when the flag is zero.
Thus this operand is the zero-extended record byte before arithmetic uses it.

The standalone [class-1 store-hook model](ECL_STORE_HOOK_0A74.md) already
accepts a host-supplied active record, but it is not wired into store word.
There is no standalone load-hook body; this change composes only its proven
`0x6B18` branch into the shared load-word helper.

## State and observed model behavior

`pr_rec_ecl_loader_state` has an optional `active_record` pointer and explicit
`active_record_size`. The caller owns the bytes for the duration of a load.
At `0x6B18` with at least `0x15` supplied bytes, load word returns
`record[0x14]` as a zero-extended word with `OK`. A non-null shorter buffer
returns `BOUNDS` without reading it; arithmetic stops before any store. With
no record, the previous bounded `HOOK` status and window-word value remain.
The other 28 effect addresses retain their previous `HOOK` behavior. Window
reset clears the pointer and size, matching the initializer's null store to
DS:`[0x5D92]`.

Synthetic tests use record byte 7, distinct window word 10, divisor 3 and a
class-2 destination: DIVIDE now stores 2, whereas the old bounded path stores
3 and reports `HOOK` when no record is supplied. With the record in operand
slot 2, `21 / 7` stores 3; changing that record byte to zero takes the
divide-error path with no store even though the window word remains 3. A
short supplied record also stops before store. Direct load and slot-value
tests cover the status, zero extension and isolation of address `0x6B9B`.
These are synthetic tests, not a native DOS execution of this operand.
