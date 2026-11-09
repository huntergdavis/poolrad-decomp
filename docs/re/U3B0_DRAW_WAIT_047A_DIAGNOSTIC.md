# Unit 3B0 initial drawer to wait integration

`pr_u3b0_draw_wait_047a` begins at a caller-supplied live `+047A` prompt
frame. It reads the prompt length byte at `[BP-2A]`, executes the source's
AX/flag and `[BP-65]` update, reads the live selected index, forms the
original argument/BP/CS near-call frame and composes the real `+01DA` drawer
on the caller's shared RAM/EGA bus. On a checked return to `+048E`, it invokes
the existing post-drawer wait and prompt-input pipeline. The nested wait/pass
status remains explicit: it may return, reach a loop boundary, stop at an
animation or clock dependency, or refuse a later profile/service. It does not
set entry2's host `provided` flag.

The caller owns and must establish all earlier prompt setup, list/string and
frame contents, the two prior `0709:0C3F` clock calls at `+0432/+043D`, and
any reached label draw at `+0475` when `[BP-2A]` was nonzero. Starting at
`+047A` does not prove those effects occurred. It also owns stable loaded
code and relocation identity, EGA/bus and services, expected prompt return,
and all preconditions of `U3B0_ENTRY_01DA_CPU.md`,
`U3B0_WAIT_048E_DIAGNOSTIC.md` and `U3B0_PROMPT_PASS.md`. Entry requires
`BP` in `[0090,FFE3]`, `SP` in `[0116,FFFC]` and ordinary non-aperture
CS/DS/SS windows, in addition to the drawer's reached-frame, immutable code,
set, storage separation and callback restrictions. Physical accesses wrap at
1 MiB and offsets at 16 bits. Earlier data writes and pushes are preserved on
a bad near-call opcode/target; nested drawer stops and unexpected returns keep
all prior CPU, RAM and EGA effects. No arbitrary-PC resume is implied.

Static source: owner GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, unit
3B0 file base `2C8CF`, span `[047A,048E)` 20 bytes SHA-256
`034659ac896a35f98153b942eda49be44364c8ec439956c3e23119a69cb2dc40`.
The call site is also documented in `SLOT_A_PREINPUT_PROMPT_ROW_PAGE.md`;
its native page observation does not establish that this synthetic composed
fixture is a native invocation. No game bytes are embedded.

Synthetic tests use the actual drawer, EGA, FIFO, CRT and downstream prompt
models for Up and no-key paths, animation stop, nested draw refusal, bad call
with prior stack and local writes, shared DS/SS alias, missing input service,
and invalid-entry atomicity. The live pre-`+047A` state and earlier clock and
label effects remain caller responsibilities. This does not prove a full
prompt, usable entry2 hook, saved gameplay, native/visual parity or strict
credit.
