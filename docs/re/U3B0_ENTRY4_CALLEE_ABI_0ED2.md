# U3B0 entry4 `+0ED2`: source-only callee ABI gate

The reviewed [precall prefix](U3B0_ENTRY4_PRECALL_0E97.md) stops at
`+0ED2`, before `CALL +0CC5`. This note maps that call and its nested
dependency from the DOS v1.3 `GAME.OVR` bytes. It contains **no new C
translation, emulator trace, full-state parity, natural gameplay
reach, or strict full-C count**. PM's serial host-load hold precludes
an emulator audit for this gate.

The target-specific `deja "Poolrad U3B0 entry4 +0ED2 near call +0CC5
callee ABI arguments return static"` and nested `+0B03` recall
queries each timed out (exit 124); neither supplied a reusable claim.
The existing U3B0 entry1 `+0000` evidence on branch
`codex/fullc-u3b0-0000-entry1` identifies the indexed far-list leaf
called by the nested routine. That leaf remains a separate provisional
recovery and is not imported into this branch.

| Original DOS v1.3 byte span | Static result |
| --- | --- |
| `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `+0ED2` | `E8 F0 FD`, near `CALL +0CC5`; return address `+0ED5` |
| `+0CC5..+0D87` | file `[0x2D594,0x2D657)`; 195 bytes / 68 instructions; SHA-256 `3c0d86662dcc84e68f99fc4b4a056606cf0ac652f4918d523b7ceb650168538a`; `RETF 4` at `+0D85` |
| `+0B03..+0C32` | file `[0x2D3D2,0x2D502)`; 304 bytes / 103 instructions; SHA-256 `6f797c1ec66c90e89812e7154faa6417e1d895ed1311754ed8b9c0895b4120f2`; `RETF 4` at `+0C30` |
| nested calls | `+0CC5` calls `+0B03` at `+0D7F`; `+0B03` calls `+0000` at `+0B39` and `+0BC4` |

Let `S` be `SP` on entry to `+0ED2`. The prefix has pushed the
current `CS`, caller `BP`, and a word whose low byte is zero. The
near call pushes return IP `+0ED5`; `+0CC5` then pushes BP and sets
its own BP to `S-4`:

| Callee `SS:[BP+offset]` | Original stack word |
| --- | --- |
| `+0` | saved caller BP |
| `+2` | near return IP `+0ED5` |
| `+4` | pushed caller CS |
| `+6` | caller's BP, used to reach caller locals and far pointers |
| `+8` | pushed AX argument; `AL=00`, while AH can retain prior value |

The first callee branch compares only byte `[BP+8]` with zero. This
caller's `AL=00` selects `JE +0D22`; that path decrements the word
at the caller-supplied far pointer, applies signed bounds using
`DS:[6B29]` and caller locals, then pushes its own argument, caller
BP, and CS for `CALL +0B03`. At `+0B03`, the same low-byte zero
selects the `+0B9F` loop. That loop can call the separate `+0000`
indexed-list leaf with the caller's head pointer and an index, then
checks the returned node's byte `+29`. Both nested callee bodies end
in `RETF 4`. The mixed near-call / far-return sequence balances the
explicit CS word and discards the two argument words. If both callees
return normally, `+0CC5` resumes at `+0ED5` with `SP=S+6`.

These are instruction-level ABI deductions from pinned bytes. The
source-only gate does not establish which loop exits occur in a
reached game frame, what final RAM/CPU state results, or whether the
`+0ED5` successor is reached in natural gameplay. The next parity
gate requires serial original-byte execution through `+0CC5`,
`+0B03`, and `+0000`, with actual stack and memory effects measured
before any C composition or count claim.
