# Unit 0x77 entry 12, +0x299C: provisional control model

`src/recovered/overlay_u77_spell_299c.c` translates the in-body branches,
external call order, one selected-byte store, and returned `AL` of this
routine. All external calls are callbacks: this is a **provisional control
model**, not a full-effects translation or a native reached-frame claim.

## Original source

The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x77` public entry 12 begins at file offset 145,878 (`+0x299C`, stub
`0x85:0x5C`). Its entire ledger bound `[145878,146247)` is code: **369
bytes, 164 instructions**, SHA-256
`04b04f1a72210df4e943e51139826ea6a1ab487b254dd717b7c20e9f4c361b99`.
The final `retf 0xC` starts at `+0x2B0A` and ends at the next bound
`+0x2B0D`; all 18 direct branches target instruction starts inside the
routine. There are 15 physical far-call sites: seven to `AF8:B25` and
one each to `0x96:0x2A`, `AF8:B0B` twice, `0x3F1:0x2E8`,
`0xBA:0x1816`, `AF8:B98`, `0x709:0x626`, and `0x96:0x25`.

The preceding 99 bytes `+0x2939..+0x299B` are data, including
length-prefixed text addressed by seven title-copy calls in this routine.
No source text bytes are reproduced in the public model. The active
character far pointer `DS:[5D92/5D94]` used on the negative branch is
also described in [U7F_DIV5_ENTRY10.md](U7F_DIV5_ENTRY10.md); the use
was rechecked in the original instructions here. A bounded `deja` query
timed out with no result, so no prior-session result was reused.

## Control flow represented

The low byte of `[BP+10]` selects one of seven `AF8:B25` calls for values
0..6; other values select none. The body then calls `0x96:0x2A` with that
word and stores returned `AL` to the byte at its first far-pointer
argument `[BP+6]`. If that byte is zero, it returns `AL=0` without the
later calls. Otherwise it reads the signed word at its second far-pointer
argument `[BP+0A]`. A negative value executes the message, prepare,
active-character, append, and display call sequence; a nonnegative value
skips those calls. Both nonzero paths call `0x96:0x25` with the byte at
`[BP+0E]` and the second far pointer, then return its `AL` through
`retf 0xC`.

The C callback receives a distinct event for each original call site and
supplies the selector and final `AL` values. It owns all unmodeled external
effects. The C routine reads the signed state word after the selector
callback, matching the original read order if that callback mutates it.
The C contract does not expose the original local strings, stack scratch,
registers other than returned `AL`, or runtime-owned side effects.

## Verification and gate

`test_overlay_u77_spell_299c.c` checks all seven title modes, no-title
modes, zero and nonzero selector results, negative and nonnegative signed
words, callback order, result values, and invalid host arguments.
`check_overlay_u77_spell_299c.py` hashes the original bytes, verifies all
164 instruction starts and 15 call sites, and executes them in 16-bit mode
with the same external-call stubs as the C model. **108 cases** compare the
call-site sequence, stored selector byte, and returned `AL` across nine
modes, three selector values, and four signed-state values. This is
control parity under stubs, not native runtime parity.

A bounded search of native notes and 17 local RAM captures found no +299C
entry frame or loaded entry-byte hit. No strict full-function or count claim
follows from this model. The separate +28BE helper is tracked in its own
branch and PR; neither model depends on the other.
