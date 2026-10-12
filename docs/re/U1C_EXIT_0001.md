# Unit 0x1c +0x0001: ECL opcode 0x00 EXIT (bounded model)

`src/recovered/overlay_u1c_exit_0001.c` models DOS v1.3 `GAME.OVR` unit `0x1c`
`+0x0001..+0x00F3`, the handler of ECL opcode `0x00` (EXIT; ECL_CONTRACTS.md
sections 3 and 5, dispatcher case `+0x2F2D`). It is the handler at saved PC
`0x9974`, where the saved run entry ends
([ECL_HOOKS_6E7D.md](ECL_HOOKS_6E7D.md)). Classification: **bounded model, not
strict full C**: the resident box call is a fallible host hook, the heap call
list is the loader state's `call_stack` array (no allocator, no `FreeMem`, no heap
parity), and the box hook's argument widths are abstracted. **No count or credit
is claimed**, and nothing here claims the actual DOS branch of `[0x84E2]` or
one-Up parity.

`deja "Poolrad ECL opcode 0x00 EXIT unit 0x1C +0x0001 call stack release"` found
no earlier decode. Reused: ECL_CONTRACTS.md section 5, FUNCTION_LEDGER.md (the
RETURN/GOSUB node model and `pr_rec_ecl_op_return`), U1C_PICTURE_0856.md (the
same `0x3F1:0x02E8` box call and hook pattern).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| extent | unit `0x1c` (file `3,398`) `+0x0001..+0x00F3` = file `0x0D47..0x0E39`, **243 bytes, 91 instructions**, `retf` at `+0x00F3`, the last byte; the next routine starts at `+0x00F4`, so the ledger bound is exact. SHA-256 `9f2483f23b51d3dbc54a83592241cbd6f5bc27e1173e62f17c54c3b6672ca493` |
| before it | the byte at `+0x0000` is `0x00`: the empty Pascal string the handler copies through `cs:0x0000` |
| frame | `sub sp, 2` (a 1-byte string slot at `[bp-1]`); no arguments, no result |
| calls | runtime `0xAF8:0x0B0B` x2 (string copy, folded into the model), resident `0x3F1:0x02E8` x2 (the box call), runtime `0xAF8:0x16A8` x1 (FillChar), runtime `0xAF8:0x0254` in a loop (FreeMem) |
| control flow | all jumps internal; one backward jump, the list walk `+0x00DF -> +0x00A4` |

## Effects in the original's order (byte-checked)

| Offsets | Effect |
| --- | --- |
| `+0x0007..+0x000C` | `[0x84E2] = 0` jumps to `+0x0066`, skipping **both** box calls |
| `+0x000E..+0x002F` | copy `cs:0x0000`; resident `0x3F1:0x02E8` with the byte words `1, 1, 0x0F, 0x0F, 0, 0x0F` and the empty string |
| `+0x0034..+0x003E` | window-0 word `0xE6` (byte offset `0x1CC`) `= 0` jumps to `+0x0066` |
| `+0x0040..+0x0061` | copy again; `0x3F1:0x02E8` with `3, 3, 0x0D, 0x0D, 0, 0x0F` and the string |
| `+0x0066..+0x007B` | `[0x4430] <> 0`: `[0x5D92]:[0x5D94] := [0x4436]:[0x4438]`; `[0x4430] := 0` |
| `+0x0080..+0x008C` | `FillChar(DS:0x84C8, 2, 0)`: `[0x84C8] := [0x84C9] := 0` |
| `+0x0091`, `+0x0096` | `[0x84E0] := 0`; `[0x84E2] := 0` |
| `+0x009B`, `+0x00A0` | `[0x442E] := 1` (the run loop's stop byte); `inc word [0x49ED]` (the PC) |
| `+0x00A4..+0x00DF` | walk the 6-byte call list at far `[0x6F70]:[0x6F72]`: save the node's next link (`+2`/`+4`) to `[0x6F74]:[0x6F76]`, `FreeMem(node, 6)`, head := next; until nil |
| `+0x00E1..+0x00EB` | `[0x5E25] := 0x11`; `[0x5E24] := 1`; `[0x84DE] := 0` |

What the box call draws, and what the cleared bytes mean, is **not asserted**.

## Model, host state and status contract

* Loader-state fields used: `pc`, `b442e`, `b84c8`, `b84c9`, `b84e0`, `b84e2`,
  `b5e24`, `b5e25`, `call_stack`/`call_depth`, window-0 word `0xE6`. The lane-C
  env adds `[0x4430]`, `[0x4436]:[0x4438]`, `[0x5D92]:[0x5D94]` and `[0x84DE]`.
* The call list is emptied (`call_depth := 0`) and the node count is reported.
  `call_depth` above the array returns `PR_U1C_EXIT_ARGUMENT` before any effect.
* The box call is a fallible hook that runs only under `[0x84E2] <> 0`. A missing
  reached hook returns `PR_U1C_EXIT_HOOK` with **no effect made** (for the second
  call, with the first call's effects retained, since the first call may have
  changed the hooks); a failing hook
  returns `PR_U1C_EXIT_CALLBACK` with the effects before it retained (none, or the
  first box call when the second fails) and **no later clear, PC increment, stack
  release or `[0x5E24]`/`[0x5E25]` write**.
* `pr_rec_ecl_run` clears `[0x442E]` when the loop ends, so after a successful run
  the final `b442e` is 0 while the handler-local value is 1.
* **Box argument widths.** The six values are pushed as `mov al, imm; push ax`, so
  the high byte of each pushed word is whatever `AH` held; the hook takes bytes.
  Whether the callee reads only the low bytes is not established, and no argument
  parity for the upper bytes is claimed (the same caution as PICTURE).

## Verification

`tests/recovered/test_overlay_u1c_exit_0001.c` (synthetic state and script; no game
bytes): the zero-input path (every clear, the PC increment and its 16-bit wrap,
`[0x442E]`, `[0x5E24]`, `[0x5E25]`, `[0x84DE]`); `[0x4430]` restore with zero, one and
`0xFF`; the call list at depths 0, 1, 2, 3, 33 and 64 and an invalid depth;
the box gating (`[0x84E2]` zero skips both, nonzero with window-0 word `0xE6` zero
makes one call with `1, 1, 0x0F, 0x0F, 0, 0x0F`, nonzero as a word such as `0x0100`
makes two); missing hook (reached and not reached), failing first and second calls
with the retained and unmade effects; null arguments. An **analogous synthetic flow**
(not the saved entry's bytes: a different PC layout starting at `0x9A10`, a different
operand order, different immediates and branch targets, an OR in place of the AND)
runs through `pr_rec_ecl_run` with the merged SAVE, COMPARE, IF, GOTO, OR and this
EXIT handlers: a skipped SAVE, a SAVE and a load through `0x6E7D` (the hook
exceptions), the two `0x49FD`/`0x49FE` stores (the `[0x84E8]` flag), a taken GOTO,
the OR to `0x6E82`, and EXIT: ten dispatches, the run stops with status OK at PC
`0x9A5A` with latch `0x00` and previous opcode `0x30`, `[0x442E]` cleared by the run,
`[0x84E8] = 1`, the OR destination equal to the low byte of `0x30` or the host
`[0x6AB1]`, the six flags equal to those of the OR's own compare, the call list of two
nodes freed; a false `IF <` does not reach EXIT; `[0x84E2] <> 0` without a box hook
stops at EXIT with HOOK and `[0x5E24]` untouched. A second box hook that the first
call removed stops with HOOK, keeping the first call's effects and nothing after it.

## Private hash-pinned smoke on the restored slot-A state (not committed)

Inputs hash-checked: `SAVGAMA.DAT` `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`,
script `bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8`, `GAME.OVR`
`872964a3…`. `[0x84E2]`, `[0x4430]`, `[0x84DE]`, the call list and the box hook
are **explicit host inputs, not recovered save values** (the save frame carries
none of them); window 0 and 1, the script and `[0x6AAD..0x6AB1]` come from the save.
`pr_rec_ecl_run` at PC `0x9914` with the merged SAVE, COMPARE, IF, GOTO, AND/OR and
this EXIT bound, no HOOK mapped to OK:

| Host inputs | Result |
| --- | --- |
| `[0x84E2] = 0`, empty list | run status OK after **10** dispatches; final PC `0x9975`, latch `0x00`, previous opcode `0x2F`; `[0x442E] = 0` after the run (1 in the handler); `[0x84E8] = 1`; `[0x84C8] = [0x84C9] = [0x84E0] = [0x84E2] = 0`; `[0x5E24] = 1`, `[0x5E25] = 0x11`; no box call; no further opcode executed by this smoke |
| `[0x84E2] = 1`, no box hook | run stops at the **10th** dispatch (EXIT): run status HANDLER, handler status HOOK, PC `0x9974`, latch `0x00`; EXIT made no effect (`[0x5E24] = 0`); `[0x84E8] = 1` from the earlier SAVEs |
| `[0x84E2] = 1`, a counting box hook | completes as above with **2** box calls (the saved window-0 word `0xE6` is nonzero) |

The compare flags at the end are those of the AND's compare of 0 with its result (it
overwrites the gate COMPARE's flags). These are conditional model results on host
inputs, not a DOS trace and not one-Up parity; the next opcode after the run entry
(the search entry `0x9975` and its PICTURE at `0x997B`) is not run here.
