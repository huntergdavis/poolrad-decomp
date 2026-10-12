# Unit 0x1c +0x10D1: ECL HORIZONTAL MENU (0x2B), bounded model

Opcode `0x2B` (HORIZONTAL MENU, reference name; [ECL_CONTRACTS.md](ECL_CONTRACTS.md), `2+n` operands) dispatches to unit `0x1c`
`+0x10D1..+0x125C`, file `0x1E17..0x1FA2` (unit base `0x0D46`): **396 bytes, 161 instructions**, ending in the `retf` at `+0x125C`, a 0x140-byte
frame. SHA-256 `a8105d7ca5ec53c0877639cca6a545284777bc6b47bcc97490b97ea8f5a70e73` (GAME.OVR, 206,105 bytes, `872964a3…7928614`; matches the PM).
The next routine is `+0x125D` (CLEARMONSTERS). `deja` timed out (PM's and mine) with no hit. **Classification: bounded model**: two callees stay
unresolved hooks; no strict full-C, count, identity, DOS/native or one-Up parity claim.

## Original order (all instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x10D8..+0x10DC` | `[bp-5] := 0` (a byte passed by reference later); `[bp-0x38] := 0` (the empty 50-byte Pascal string `text`) |
| `+0x10E0..+0x10F5` | loader(2); `dest := word(slot 1)` via `0x2B:0x4D` |
| `+0x10F8..+0x1103` | `n := AL` of slot value(2); `dec word [0x49ED]` (undoes the loader's PC increment) |
| `+0x1107..+0x110B` | loader(`n`): the `n` item operands, string items landing in the string buffers `0x6FC8 + 256*i` |
| `+0x1114..+0x1154` | `n = 1`: `p3b := 1`, `p3d := p3e := 0x0F`; if buffer 1 equals the code constant at `cs:0x1085` (`0xAF8:0x0C10`), buffer 1 := the constant at `cs:0x10AA` (`0xAF8:0x0B25`, maximum 0xFF); else `p3b := 0`, `p3d := 0x0F`, `p3e := 0x0A` |
| `+0x1158..+0x116C` | `p3c := 1` if `[0x84E0] <> 0` and `[0x84E1] <> 0`, else 0 |
| `+0x116F..+0x11D5` | `m := byte (n - 1)`; for `i = 1..m`: `text := text + cs:0x10CD + buffer i + cs:0x10CF` (load, three concatenations, store with maximum `0x32`) |
| `+0x11D7..+0x1210` | `text := text + cs:0x10CD + buffer n` (maximum 0x32) |
| `+0x1215..+0x1237` | `0x2B:0x61` (unit `0x25` entry 13, `+0x1684`) with the far byte `[bp-5]`, the far string `text`, `0x0D`, `p3e`, `p3d`, `p3b`, `p3c`; `AL` kept |
| `+0x123A..+0x1243` | `0x2B:0x52 (dest, zero-extended AL)`, the store word |
| `+0x1248..+0x1254` | `0x709:0x0DD3` with the byte words `(0, 0x18, 0x27, 0x18)`; `mov sp,bp; pop bp; retf` |

## What is resolved and what is not

* **Resolved, translated in C:** the four Turbo Pascal string-runtime routines (decoded from the START.EXE expanded image at `0xAF80 + off`):
  `0xAF8:0x0B0B` (copy the length byte and text; `retf 4` leaves the destination pointer), `0x0B98` (append, length capped at 255), `0x0B25` (store with
  a maximum length, truncating) and `0x0C10` (compare; the handler's `jne` means not equal). The composed menu text is therefore exact. The four code constants
  are **host data** (game text stays out of the repository): `cs:0x1085` length 36 SHA-256 `35ec3137326f03c2b53ac1300f08c1a5b25d9a6002ffd1c26731fb8c290f2e8e`; `cs:0x10AA` length 34 SHA-256 `307bbae616e0fad25385b6a1323e2d84f7d0be6b15dcee55e9667b478f3d5b99`; `cs:0x10CD` length 1 SHA-256 `5270ec3ffe823f7ad46ff5c416ff1b374fd2744a368e2921a96051f6a47ffcb8`; `cs:0x10CF` length 1 SHA-256 `3145e7a95720a1db303f2198e796ea848f52b6079b5bf4f47d32fad69c2bce77`.
* **Unresolved hooks:** `0x2B:0x61` (the menu routine; the by-reference byte `[bp-5]` may be changed by it and its result `AL` is stored) and `0x709:0x0DD3` (a display
  routine over the byte words `(0, 0x18, 0x27, 0x18)`, gated by `[0x4A1E]`; not decoded here).
* **Bounds:** `n = 0` (the original's byte loop would run 255 iterations over buffers beyond the model's) is reported as COUNT; `n >= 64` is the loader bound.

## Model

`pr_u1c_op_horizontal_menu` (`overlay_u1c_horizontal_menu_10d1.{h,c}`). Failure order: loader(2); slot value(2): BOUNDS/UNRESOLVED **and HOOK** stop before `n` is used (a class-1 read whose load hook may override the word leaves `n` unknown, so the handler reports CORE/HOOK at that boundary with the PC not decremented, no loader(`n`), no hook and no store; only the literal/plain-address route continues); loader(`n`) failure after the decrement; missing code constants where reached (STRING); the menu hook (nothing stored on a missing or failing hook); the store word
(a class-1 effect destination stores and the display hook still runs, then HOOK; an unsupported destination stops before the display hook); the display hook.

Tests (`test_overlay_u1c_horizontal_menu_10d1.c`) are fully synthetic (six-bit packed items, one-character code constants): the composed text for `n = 3` and the truncation to 50 bytes
against an independent concatenation restatement, the `n = 1` buffer replacement and a near-equal string, the parameters and the `[0x84E0]`/`[0x84E1]` flag, hook order and arguments, failing and
missing hooks, missing constants, destination classes, `n = 0` and `n = 64`, loader and slot failures.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound (through DELAY, GOSUB, RETURN, PRINT
RETURN); explicit host inputs, none from DOS: `[0x84E2] = 0`, PICTURE DS bytes 0, first RANDOM result 13, second RANDOM result in {0, 1, 2}, SURPRISE rolls 1..6, the `0x2B:0x3E` result in {0, 1, 5}, the
four code constants read privately from GAME.OVR, counting menu and display hooks and the menu result `AL` in {0, 1, 2, 3}.

For the 72 combinations that reach it, HORIZONTAL MENU at `0xB6BC` has `dest = 0x9801`, `n = 1`, buffer 1 **equal to the `cs:0x1085` constant** (replaced by the `cs:0x10AA` constant),
a composed text of length 35, `p3b = 1`, `p3c = 0`, `p3d = p3e = 0x0F`, one menu hook call and one display hook call. **Next stop** (all 72 x 4 = 288 runs): UNMODELLED at PC `0xB1AF`, opcode `0x1C`
(CLEARMONSTERS, handler `+0x125D`, 0 operands), previous opcode `0x09`. Supplied-state probe only: no live DOS RNG, menu input, native search reachability or one-Up parity.
