# Unit 3B0 restricted prompt poll diagnostic

`pr_u3b0_poll_0566` projects the checked source interval from the far call at
`+0566` through the first reached branch boundary at `+059C` or `+06CF`.
It composes the existing CRT KeyPressed and game `0899:0059` wrapper models
with one caller-owned CPU/RAM and service environment. The caller may supply
the merged FIFO as the keyboard service. Original static source spans
`[0566,059C)` and `[06CF,06FB)` have SHA-256
`216ee14662a01f4ba3130d45ea3508de00726bf23224e8e7925ef08746424b42` and
`deae8526a9c65a69af1e413b4bc0ee9bb22c5d1f5a4fcfe98b21e63ea8ffff19`;
GAME.OVR SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The instruction boundary and behavior were independently reviewed in t3825.

Caller must supply the actual pre-`+0566` prompt frame, loaded code and
relocations, prior prompt setup/draw/timer effects, mutable A20-off 1 MiB RAM,
CPU, and all reached host services. This API does not verify their provenance.
It checks reached far-call opcodes, target offsets and caller-declared relocated
segments, validates far returns, and restricts entry stack/frame ranges. Source
instruction identity outside reached far calls and code immutability throughout
callbacks are caller obligations. The caller must ensure any code aliases and
callee side effects preserve the assumed loaded non-call instructions. Offset
arithmetic is 16-bit and physical accesses wrap at 1 MiB, as in the composed
models. Earlier completed effects remain visible after later refusal; no status
authorizes restarting the entry to resume it.

The no-key status stops before `+06CF`; the live local stop byte determines
what the original later does. The extended result also stops there, before
window clearing, result/flag propagation and `RETF 16h` at `+06F8`. Nested CRT
and 0899 nonreturn statuses are retained in the diagnostic. Empty read refusal
is a terminal model stop after earlier effects, never a resumable yield. The
function does not mark entry2's prompt hook provided, complete the prompt,
initialize a saved game, or claim native/gameplay parity or strict credit.

Synthetic tests exercise the existing wrapper and FIFO: empty poll, extended
Up, queued-key drain, disabled extended path, output/local alias, nonextended
branch, bad call target and nested driver stop after visible effects. They do
not constitute a native comparison or prove caller-provided prior effects.
