# Class-1 load-hook value at 0x6B9B

The ECL load-word helper now resolves one additional class-1 hook address:
`0x6B9B` returns the zero-extended byte at offset `+0x6E` of the active
character record. This is a static original-byte result, with synthetic C
tests; no native breakpoint measurement is claimed. The previous
[`0x6B18` composition](U1C_ARITH_CLASS1_RECORD_6B18.md) supplied the state
interface reused here. A `deja` MCP recall for this address returned no
matching prior session.

## Original bytes and call path

The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x25`'s load hook `+0x07BC..+0x0A73` (file `0x7C8F..0x7F46`, 696
bytes) hashes
`6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451`.
The load-word caller `+0x0E0C..+0x0F52` (file `0x82DF..0x8425`, 327 bytes)
hashes `e8f999e62ee3f04b194908f90ce7c0e15b0478a2af16a3843a1a076c86c67ce5`.

The hook first sets its by-reference flag to 1 at `+0x07C5` and subtracts
`0x6B00` from the address. For relative address `0x9B`, `+0x07EA..+0x07FC`
compares, loads the active-record far pointer from DS:`[0x5D92]`, reads
`record[0x6E]` into AL, clears AH, saves AX, and jumps to `+0x0A6B`. It
skips the flag-clear at `+0x0A67`. At `+0x0E41..+0x0E69`, load word saves
the returned AX and reads the window-1 word only if the hook flag is zero.
No additional side effect was decoded on this branch beyond the hook's
by-reference flag and local return value.

## Bounded composition

With a supplied `active_record` of at least `0x6F` bytes, `0x6B9B` load
returns `record[0x6E]` as an unsigned word with `OK`. A non-null view of
`0x6E` bytes or fewer returns `BOUNDS` without reading past the buffer; if
arithmetic reached this operand, it stops before store. With no supplied
record, the helper retains its previous `HOOK` status and window-word value.
That is an explicit model boundary and does not imply DOS accepts a null
active pointer. The other 27 class-1 effect addresses remain bounded.

Synthetic helper tests cover record byte 0, 1 and 255, slot-value forwarding,
the distinct window word, the too-short view and an unchanged `0x6BA0`
effect. Arithmetic tests show record byte 255 plus 1 stores 256 rather than
window word 10 plus 1; `21 / record byte 7` stores 3 when the hook is the
second operand, while a short record in that position stops after slot 1
and before the store. Class-1 store hooks, class-4 stores and divide-error
runtime effects remain outside this change. No whole-handler strict full-C
or game-function count credit is claimed.
