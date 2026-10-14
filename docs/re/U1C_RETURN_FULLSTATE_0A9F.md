# Unit 0x1C +0x0A9F: provisional full-state RETURN model

The existing `pr_rec_ecl_op_return` in `ecl_helpers.c` is a verified,
path-complete semantic model over a bounded host array. It omits the original
heap node, DS temporary pointer words, and FreeMem call. This separate model
preserves those in-body RAM, CPU, and stack effects. It remains
**provisional** because native caller and live heap effects have not been
replayed jointly; no strict full-C credit, new identity, or count claim is
requested.

## Source and route

Ignored DOS v1.3 `GAME.OVR`: 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The verified-game ECL opcode `0x13` dispatcher calls unit `0x1C:+0x0A9F`
([ECL_CONTRACTS.md](ECL_CONTRACTS.md)). Its exact original range is
`[0x17E5,0x1833)`, **78 bytes / 25 instructions**, SHA-256
`a0b08ebfbd9bab9e5a735600cbc6120d0e56d67f77f585d2d07705db74d1681c`.
The `retf` at `+0x0AEC` is the last byte; the next prologue begins at
`+0x0AED`. A bounded `deja` query timed out, so no prior-session result was
reused. Existing source ownership and paths are documented in
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) section 1.20.

## Decoded body and dependency

The handler increments ECL PC `DS:[49ED]`. It tests the far list head at
`DS:[6F70/6F72]` by ORing its words. An all-zero head returns. Otherwise it
loads the head node's saved PC at `+0` into `DS:[49ED]`, copies the node's
next far pointer at `+2/+4` into DS temporaries `[6F74/6F76]`, and calls
runtime `AF8:0254` FreeMem with the head pointer and size 6. It then copies
the potentially callee-modified temporary pointer into the head and returns.
Only the all-zero far pointer is nil; either word alone may be nonzero.

The FreeMem ABI is independently documented in
[U1C_CLEARMONSTERS_125D.md](U1C_CLEARMONSTERS_125D.md): runtime success
uses `retf 6`; its heap free-list and error path are outside this model and
are supplied by a callback. The `OR` instruction leaves AF undefined, so the
audit compares all **defined** flags and masks AF. A bounded local route and
capture search found no reached native `+0x0A9F` entry frame. Host-supplied
new-game execution in [ECL_NEW_GAME_ROUTE.md](ECL_NEW_GAME_ROUTE.md) is not a
native frame.

## Verification

`check_overlay_u1c_return_0a9f.py` checks the original file/body hashes,
all instruction and call/branch/return boundaries, then runs original bytes
under Unicorn and the C model from identical states. Ten cases cover empty
heads, offset-only and segment-only heads, node-offset wrap, PC wrap, saved
next pointers, and callback modification of the DS temporary pointer. It
compares the **entire 1 MiB RAM**, all returned CPU registers and defined
flags, and FreeMem callback entry registers and three stack arguments. Both
executions receive the same bounded callback effects. The focused C test
checks nil, node release, temporary-pointer propagation, PC wrap, and far
return.

Native caller and live FreeMem heap parity remain open. This model extends
the existing semantic array model without changing it.
