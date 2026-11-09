# Unit 3B0 restricted UpCase and mapped-key path

`pr_u3b0_mapped_060c` continues caller-supplied `+060C` CPU/bus state from
the existing selection diagnostic. It composes the real SYSTEM UpCase and
set-membership leaves using live stack frames and caller-supplied relocated
segment. It implements the source's space handling and the gated second
membership/table path through `+06CF`. A non-space member of the first set
stops before the unimplemented text-list search at `+0634`; other paths stop
at `+06CF`. It does not execute the prompt epilogue or report entry2 provided.

Sets at `CS:037D` and `CS:039D`, the mapping table at `DS:288C`, the extended
flag pointer, the prompt frame and all previous prompt effects are live,
caller-owned state. No private literal or lookup table is embedded. The
caller also owns the 1 MiB A20-off RAM/EGA bus, loaded code identity, stable
code, and relocated SYSTEM segment. CS/DS/SS and SYSTEM 64 KiB windows must
be outside the EGA aperture for the flat-RAM leaves; entry BP is restricted
to `[0090,FFF3]` and SP to `[000A,FFF4]`. The flag write uses the bus and
can reach EGA; all storage and callback restrictions of the bus apply. Offset
arithmetic wraps at 16 bits; physical addresses wrap at 1 MiB. Aliases remain
live and writes occur in source order. Code identity and prior effects are
caller obligations, not proved by passing an environment.

Each reached far opcode/offset/segment is checked before its call; each leaf
returns through its real stack frame and is checked for CS:IP:SP. Bad code,
leaf or transfer stops preserve earlier pushes, CPU, RAM and bus effects; no
rollback or resumption is implied. A stop at `+06CF` is not a completed
prompt or native execution. Text-list matching/redraw, earlier setup/draw and
timer effects, full entry2 provider, saved gameplay, visual parity and strict
credit remain outside this slice.

Source: owner GAME.OVR SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`, unit
3B0 base `2C8CF`. `[060C,0634)` is 40 bytes SHA-256
`6ccb1da67d55c7f3832fef4cdfce1a1597407187961cc8621928bdffb5f46d71`;
`[0691,06CF)` is 62 bytes SHA-256
`75d09b4ef4000f81cdc9747219008fae05a4ccaf8f4632e9bd87a05a8d111fd8`.
The unmodeled interval `[0634,0691)` is not claimed. No game bytes or private
set/table contents are included in the repository.

Synthetic tests compose `+059C` → `+05C4` → this model → the existing
`+06CF` epilogue using the actual SYSTEM leaves. Cases cover a mapped key,
uppercase text-search stop, space, unmapped key, flag/local alias, bad first
and second set calls with partial effects, and atomic invalid entry. They
verify this bounded host composition, not native reachability or parity.
