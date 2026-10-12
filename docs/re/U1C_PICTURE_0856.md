# Unit 0x1c +0x0856: ECL opcode 0x0E PICTURE (bounded model)

`src/recovered/overlay_u1c_picture_0856.c` models DOS v1.3 `GAME.OVR` unit
`0x1c` `+0x0856..+0x0950`, the handler of ECL opcode `0x0E` (PICTURE, one
operand; ECL_CONTRACTS.md section 3, dispatcher case `+0x2FBD`, its only near
caller). The saved slot-A search entry reaches opcode `0x0E` at PC `0x997B`
after its `SAVE 0xFF` ([SLOT_A_UP_ECL_GATE.md](SLOT_A_UP_ECL_GATE.md)).
Classification: **bounded model**: the loader and slot-value cores are lane A's
merged cores, and every other callee is an explicit fallible host hook. **No
count or credit is claimed**, and no claim is made about which branch the live
route takes or what the PC `0x997B` operand holds.

`deja "Poolrad unit 0x1C +0x0856 PICTURE opcode 0x0E"` found no earlier decode
of this handler (only the lane-A issue thread for the run loop). Reused:
ECL_CONTRACTS.md, FUNCTION_LEDGER.md (unit `0x25` stubs `0x2B:0x2A`,
`0x2B:0x25`), U1C_AND_OR_0DEF.md (the composition and status contract),
ECL_LOAD_WORD_CLASS4.md.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

| Item | Value |
| --- | --- |
| extent | unit `0x1c` (file `3,398`) `+0x0856..+0x0950` = file `0x159C..0x1696`, **251 bytes, 91 instructions**, `retf` at `+0x0950`, the last byte; next prologue `+0x0951`, so the ledger bound is exact and there is **no trailing data**. SHA-256 `51628c4f1c9879395122bd027bd60463b7ad58181b768d48be6f1b697ee17663` (derived independently; it matches the PM's) |
| data before it | `+0x0851..+0x0855` (5 bytes): two Pascal string constants read through `cs:` offsets: `cs:0x0851` of length 0 and `cs:0x0852` of length 3. Their text is not reproduced; the model takes them as host strings |
| frame | `sub sp, 6`: `[bp-1]` the picture id, `[bp-2]` a 1-byte string slot, `[bp-5..-2]` a 4-byte string slot (overlapping `[bp-2]`, consumed earlier); no arguments, no result |
| control flow | 10 conditional and unconditional jumps, all internal, all forward (no loop) |
| calls | `0x2B:0x2A` (unit `0x25` entry 2, the operand loader) and `0x2B:0x25` (entry 1, slot value), both merged; `0xAF8:0x0B0B` x2 (string copy, folded into the model); resident `0x3F1:0x02E8` x1; unit `0x3b9` entry 2 (`0x3D0:0x2A`) and entry 1 (`0x3D0:0x25`); unit `0x25` entry 7 (`0x2B:0x43`); unit `0x3b4` entry 1 (`0x3CA:0x25`) |
| DS and window accesses | reads `[0x49F9]`, `[0x49FA]`, `[0x84E0]`, `[0x84E3]`, `[0x6AC4]`, `[0x6AC6]`; window-0 word `0xE6` (byte offset `0x1CC`) and window-1 word `0x2E1` (byte offset `0x5C2`, script address `0x6DE1`); writes `[0x84C8]`, `[0x84C9]`, `[0x84E0]`, `[0x84E1]`, `[0x84E3]` (constants only) |

## Paths (in the original's order)

| Offsets | Effect |
| --- | --- |
| `+0x085C..+0x085F` | loader, count 1 (`pr_rec_ecl_load_operands(state, 1)`) |
| `+0x0864..+0x086C` | `id := low byte of slot value 1` (`pr_rec_ecl_slot_value`) |
| `+0x086F..+0x08A2` | window-0 word `0xE6` = 0 **and** `id <> 0xFF`: copy `cs:0x0851` to the local; resident `0x3F1:0x02E8` with the byte words `3, 3, 0x0D, 0x0D, 0, 0x0F` and the string |
| `+0x08A7..+0x08AB` | `id = 0xFF` goes to `+0x0913` |
| `+0x08AD`, `+0x08B2` | `[0x84C9] := 1`; `[0x84E0] := 1` |
| `+0x08B7..+0x0911` | window-1 word `0x2E1` = `0xFF` (a 16-bit compare): copy `cs:0x0852` to the local; `0x3D0:0x2A` with (string, id, 0, DS offset `0x6ABE`); `0x3D0:0x25` with (3, 3, 1, far pointer `[0x6AC6]:[0x6AC4]`). Otherwise `0x2B:0x43` with (id, low byte of window-1 byte `0x5C2`). Then return, **skipping the clears below** |
| `+0x0913..+0x0943` | `id = 0xFF`: when `[0x49F9] > 1` or `[0x49FA] = 1`, and `[0x84E0] <> 0` or `[0x84E3] <> 0`: `0x3CA:0x25`; `[0x84E0] := 0`; `[0x84E3] := 0`; `[0x84E1] := 1` |
| `+0x0943..+0x094D` | `[0x84C8] := 0`; `[0x84C9] := 0`; `mov sp, bp; pop bp; retf` |

So an id other than `0xFF` shows a picture (setting `[0x84C9]` and `[0x84E0]`
first) and an id of `0xFF` clears the picture state, refreshing the view only
under the flag conditions. What the id values, the picture routines and the
flags mean in game terms is **not asserted**.

## Pushed argument widths (an abstraction, stated)

Most arguments are pushed as `mov al, imm or [mem]; push ax`, so the high byte of
each pushed word is whatever `AH` held at that point: the box call's six words
(`+0x0881..+0x0890`), the id and the zero for `0x3D0:0x2A` (`+0x08D3..+0x08D9`),
the `1` for `0x3D0:0x25` (`+0x08EC`), and the id and the window byte for
`0x2B:0x43` (`+0x0902..+0x090B`). The `3, 3` for `0x3D0:0x25` are full words
(`mov ax, 3`). The hooks are byte-typed (the two far-pointer words and the DS
offset are 16-bit). Whether each callee reads only the low byte of those words
has **not** been established, so the model does **not** claim argument parity
for the upper bytes.

## Call-site order of the hook checks

Each hook is checked at its own call site: with `0x3D0:0x2A` present and
`0x3D0:0x25` NULL, the `0x2A` call runs (and stays logged and counted) before
the missing `0x25` hook returns `HOOK`; a missing `0x2A` stops before it, so `0x25`
never runs.

## Status contract

* Loader failure or a slot value that is BOUNDS, UNRESOLVED or ARGUMENT returns
  `PR_U1C_PICTURE_CORE` before any effect (the report carries the core status).
  A class-1 slot read (HOOK) lets the handler run to completion and then
  reports `PR_U1C_PICTURE_CORE` with core status HOOK, as lane A's SAVE does.
* Every hook is fallible: nonzero stops with `PR_U1C_PICTURE_CALLBACK`, keeping
  the effects made so far (for example `[0x84C9]` and `[0x84E0]` stay set when a
  picture hook fails) and nothing after it. A NULL **reached** hook returns
  `PR_U1C_PICTURE_HOOK` before its call; hooks the path does not reach may be NULL.
* The two string constants are validated only where the original copies them:
  `cs:0x0851` must be non-NULL with length 0 (the slot is one byte and the next
  byte is the id) and `cs:0x0852` non-NULL with length at most 3. Otherwise
  `PR_U1C_PICTURE_STRING`, keeping the writes made before the copy.

## Verification

`tests/recovered/test_overlay_u1c_picture_0856.c` (synthetic script, windows, DS
bytes, strings and hooks; no game data): all 256 ids against both window words
(including `0x0100`/`0x01FF`, which are not `0xFF` as words) with every DS flag
combination for representative ids and one combination for the rest, against an
expected-call list built from the conditions (call order, hook arguments,
`[0x84C8]`, `[0x84C9]`, `[0x84E0]`, `[0x84E1]`, `[0x84E3]`, PC); each hook failing
in turn with the retained and unmade effects; NULL reached and unreached hooks;
the string checks and sizes; truncated operand (BOUNDS), an unsupported class-4
source (UNRESOLVED, no effect), the resolved `0xC04F` source, a class-1 source
(HOOK after completion), and immediate word operands (the low byte is the id,
`0x01FF` still means 0xFF); null arguments. Clean under AddressSanitizer and
UndefinedBehaviorSanitizer; strict `-Wall -Wextra -Wpedantic -Werror` full build.
The restatement shares my reading of the decode; the correspondence is the
tables above, not a DOS replay.
