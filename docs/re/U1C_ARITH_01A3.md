# Unit 0x1c +0x01A3: ECL ADD / SUBTRACT / DIVIDE / MULTIPLY, bounded model

Opcodes `0x04` ADD, `0x05` SUBTRACT, `0x06` DIVIDE and `0x07` MULTIPLY (reference names,
[ECL_CONTRACTS.md](ECL_CONTRACTS.md), 3 operands) share one handler: unit `0x1c`
`+0x01A3..+0x0233`, file `0x0EE9..0x0F79`, 145 bytes, ten bytes of locals, `retf`. SHA-256 of
the span `32f04c0374596652b532b94da3cd8eddfaa90108b8b3b7ed9f754c7591aa7707` (GAME.OVR, 206,105
bytes, `872964a3…7928614`). The next routine (`+0x0234`) is RANDOM. `deja` found no earlier note.
**Classification: bounded model** composed of existing lane-A pieces; no new hooks. No count,
strict full-C, identity, DOS/native or one-Up parity claim; accounting is the PM's after review.

## Original order (all instructions checked against the bytes)

| Range | Effect |
| --- | --- |
| `+0x01A9..+0x01AC` | loader(3), `0x2B:0x2A` |
| `+0x01B1..+0x01D4` | `for i = 1, 2`: `v[i] := slot value(i)` (`0x2B:0x25`), the **full 16-bit AX**, stored at `[bp + 2i - 8]` |
| `+0x01D6..+0x01E3` | `dest := word([0x7088], [0x7048])` via `0x2B:0x4D` (slot 3) |
| `+0x01E6..+0x01F6` | latch `[0x6F7F] = 4`: `r := v1 + v2` (`add ax`, 16-bit) |
| `+0x01F8..+0x0205` | latch 5: `r := v2 - v1` (`mov ax,[bp-4]; sub ax,[bp-6]`: **the second operand minus the first**) |
| `+0x0207..+0x0216` | latch 6: `ax := v1; xor dx,dx; div word [bp-4]`: `r := v1 / v2` (unsigned) |
| `+0x0218..+0x0222` | latch 7: `ax := v1; mul word [bp-4]`: `r := AX` only (the high word `DX` is dropped) |
| other latch | `r` is the never-written stack word `[bp-9]` |
| `+0x0225..+0x022B` | `0x2B:0x52 (dest, r)`, the store word call; return |

All arithmetic is **unsigned 16-bit**: add/subtract wrap, divide truncates, multiply keeps the low
word. With `DX = 0` a word-by-nonzero-word quotient always fits, so there is no overflow fault. A
**divisor of 0 raises the processor divide error (INT 0)**: START.EXE's runtime install sets vector 0
(`mov ax,0x2500`, handler `cs:0x00DF`: `mov ax,0xC8` and the runtime halt path at `cs:0x00E2`, i.e.
runtime error 200). The model reports DIVIDE_ERROR (runtime error 200) with no store and does not model the halt itself.

## Model

`pr_u1c_op_arith(state, stack_residue, report)` (`overlay_u1c_arith_01a3.{h,c}`): the opcode is the
latch. A loader failure stops first; a slot-value BOUNDS/UNRESOLVED stops after the slot loop before the
operand word and store; a class-1 slot read runs to completion and then reports HOOK (PICTURE pattern);
the store word's status is reported after the attempt (HOOK = word written). A latch outside 4..7 stores
the host-supplied `stack_residue` (status RESIDUE); the dispatcher reaches the handler only with 4..7.

Tests (`test_overlay_u1c_arith_01a3.c`) are fully synthetic: every pair of twelve boundary words for all four
opcodes against an independent 32-bit restatement (including the subtract order, add wrap, unsigned
divide at `0xFFFF / 1`, zero-divisor fault, `0x100 * 0x100`), byte/word/variable operand forms, a
DIVIDE-into-the-same-word shape, destination classes, loader and slot failures, class-1 slot reads, and
the residue path.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A (`abca2483…c08f1`), run entry `0x9914`, search entry `0x9975`, with the six-record
host party of [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md) (PARTYSTRENGTH 10) and these **explicit host
inputs, none from DOS**: `[0x84E2] = 0`, PICTURE DS bytes 0, the first RANDOM result AL = 13 (a reachable
value, see that note), the second RANDOM result AL in {0, 1, 2} (its range is 3), all merged handlers bound
plus this one.

* After RANDOM at `0x9B3A` and the compare, the route reaches DIVIDE at `0x9B68`: `v1 = 10` (the stored
  strength), `v2 = 3`, `r = 3`, stored to the window-2 word `0x9808` (OK); MULTIPLY at `0x9B71`:
  `v1 = 3`, `v2 = 2`, `r = 6`, stored to `0x9808` (OK); then COMPARE and IF.
* **Next stop**: dispatch 35, run status HANDLER / handler HOOK, PC `0x9B90`: a second RANDOM at `0x9B8A`
  (slot byte 2, argument 3, three hook calls) whose store to the class-1 word `0x6E7F` is a not-exempt store
  hook (the word was written); the same for each supplied second result 0, 1, 2. The store hook at
  `0x6E7F` is not audited here.

The first result AL = 13 is the only value above 12 that the RANDOM range allows; the real AL depends on the
DOS clock seed. This is a supplied-state probe: no live DOS RNG, native search reachability or one-Up parity is claimed.

A later native one-Up snapshot did produce AL = 13. Its DIVIDE operand and call audit is recorded in
[U1C_DIVIDE_AL13_ROUTE.md](U1C_DIVIDE_AL13_ROUTE.md); that snapshot stops before DIVIDE executes.

The exact dependencies that still prevent strict full-C credit, including a concrete class-1
operand mismatch, are mapped in [U1C_ARITH_STRICT_PARITY_GAPS.md](U1C_ARITH_STRICT_PARITY_GAPS.md).
