# Unit 3B0 bounded text search and redraw

`pr_u3b0_text_0634` continues the caller-supplied `+0634` prompt state. It
copies the live list length, compares each character to the live key, finds the
corresponding live selection entry and composes the existing `+01DA` drawer
on the same RAM/EGA bus. Completed list search stops before `+0691`. The
existing `pr_u3b0_mapped_060c` now also accepts a live `+0691` state and
executes only its gated mapped-key suffix; it does not repeat UpCase or the
first set check.

The inner selection lookup can cycle if no matching entry exists, and live
aliasing or drawer effects can change later loop inputs. Caller-supplied
limits count taken `JMP +0662` at `+0677` and taken `JNE +0648` at `+068F`.
A budget stop occurs before that jump with earlier state and effects retained;
it is not a guest return or a resumable yield. Reached drawer CODE, CALLEE and
TRANSFER stops retain all earlier CPU, RAM, stack and EGA effects. Reached
near-call opcode/target and actual drawer return are checked. Other instructions
and source identity must remain valid as caller-owned preconditions.

The caller owns the live prompt frame, lists and mapping bytes, CPU, shared
1 MiB A20-off RAM/EGA bus, stable loaded code, prior prompt/draw/timer effects,
and all drawer services and limits. Entry requires `IP=0634`, `BP >= 0090`,
SP in `[0048,FFFC]`, and ordinary non-aperture CS/DS/SS windows. The existing
`+01DA` drawer's non-aperture operand, stable code/set, frame, storage and
callback restrictions apply. Offset calculations wrap at 16 bits, physical
addresses at 1 MiB, and data aliases are read and written in source order.
The caller supplies provenance; it is not proved by the API. No full prompt,
entry2 provided flag, native/gameplay/visual parity, or strict credit follows.

Static source: owner GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, unit
3B0 base `2C8CF`, `[0634,0691)` 93 bytes SHA-256
`72028c936c3c870468d0b6fc5c81c6e9f401e1e35539ff7b6a23a7ad70a845d1`.
The adjacent `U3B0_MAPPED_060C_DIAGNOSTIC.md` and
`U3B0_ENTRY_01DA_CPU.md` document the reused models. No game bytes are included.

Synthetic fixtures exercise UpCase/first-set → text match → actual EGA redraw
→ `+0691` mapped suffix → `+06CF` epilogue, as well as no-match, full CPU/RAM
empty-list state, both loop limits, a shared DS/SS alias, invalid call after pushes, nested
drawer refusal and invalid frame atomicity. They do not establish natural reach or
native parity.
