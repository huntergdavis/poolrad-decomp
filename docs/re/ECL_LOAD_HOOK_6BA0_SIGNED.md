# Class-1 load-hook signed byte at 0x6BA0

The ECL load-word helper now resolves the class-1 address `0x6BA0` from a
supplied active character record. This is a static original-byte audit with
synthetic C tests, not a native breakpoint measurement. It reuses the
active-record input established by the prior [`0x6B18`](U1C_ARITH_CLASS1_RECORD_6B18.md)
and [`0x6B9B`](ECL_LOAD_HOOK_6B9B_RECORD.md) compositions. A `deja` MCP recall
for `0x6BA0` returned no matching prior session.

## Original byte path

The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x25`'s class-1 load hook `+0x07BC..+0x0A73` is 696 bytes at file
`0x7C8F..0x7F46`, SHA-256
`6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451`.
Its load-word caller `+0x0E0C..+0x0F52` is 327 bytes at file
`0x82DF..0x8425`, SHA-256
`e8f999e62ee3f04b194908f90ce7c0e15b0478a2af16a3843a1a076c86c67ce5`.

The hook sets its by-reference flag to 1 at `+0x07C5` and computes
`rel = address - 0x6B00`. For `rel = 0xA0`, `+0x07FF..+0x0810` compares the
selector, loads the far active-record pointer from DS:`[0x5D92]`, reads
`record[0x73]` into AL, executes `CBW` at `+0x080C`, saves AX, and jumps to
`+0x0A6B`. `CBW` sign-extends AL: `0x7F` becomes `0x007F`, `0x80` becomes
`0xFF80`, and `0xFF` becomes `0xFFFF`. The jump bypasses the flag-clear at
`+0x0A67`. The caller's `+0x0E41..+0x0E69` path therefore retains that AX
instead of reading the window-1 word. No additional side effect or call was
decoded on this branch beyond its flag and local return value.

## Model boundary and tests

With at least `0x74` supplied record bytes, `pr_rec_ecl_load_word` returns
the sign-extended word with `OK`; with a non-null shorter view it returns
`BOUNDS` before dereferencing offset `0x73`. With no supplied record it keeps
the existing bounded `HOOK` status and window-word value. This fallback is
a model boundary and does not claim DOS accepts a null active pointer. The
other 26 effect addresses remain bounded.

Synthetic direct-load and slot-value tests cover bytes 0, 1, `0x7F`,
`0x80`, and `0xFF`, a distinct window word, the short view, and the still
unresolved `0x6BA5` selector. A synthetic ADD uses record byte `0x80` and
literal 1 to store `0xFF81`; the old window-word fallback stores 11 and
reports `HOOK`. The short view stops before store. No strict full-C or new
game-function count is claimed; other hooks, class-4 stores and runtime
effects remain open.
