# Unit 0x1c +0x271F: ECL DELAY (0x3A), bounded model with a timing boundary

Opcode `0x3A` (DELAY, reference name; [ECL_CONTRACTS.md](ECL_CONTRACTS.md), 0 operands) dispatches to unit `0x1c`
`+0x271F..+0x272C`, file `0x3465..0x3472`: **14 bytes, 6 instructions** (`push bp; mov bp,sp; inc word [0x49ED]; lcall 0x709:0x0DBE; pop bp;
retf`), SHA-256 `aec9bf6c4361ea61af680d3d4aed03b5bdc0f62e5d8b3221bc197845ca34f8b1` (GAME.OVR, 206,105 bytes, `872964a3…7928614`).
Pascal string constants trail it up to the next prologue (file `0x34B8`) and are never executed. `deja` timed out (PM's and mine)
with no hit. **Classification: bounded model**, with the busy-wait as a host timing boundary; no strict full-C, count, identity,
DOS/native or one-Up parity claim.

## The call chain, decoded from the original bytes

| Step | Evidence |
| --- | --- |
| handler | `+0x2722 inc word [0x49ED]` (the PC advances **before** the call), then `lcall 0x709:0x0DBE` |
| resident `0x709:0x0DBE` (START.EXE expanded image `0x7090 + 0x0DBE = 0x7E4E`) | `push bp; mov bp,sp; al := [0x49E2]; ah := 0; dx := 0xE1; mul dx; push ax; lcall 0xA40:0x029C; pop bp; retf` (**21 bytes, 10 instructions**, image `0x7E4E..0x7E62`, `retf` at `0x7E62`, next prologue `0x7E63`; SHA-256 `a7b0396a671f47e30f7123716ac21bce6271d5508b6f4acbe9e7fad20aae861d`; bytes `[scrubbed: span=U1C_DELAY_271F-1 sha256=a7b0396a671f47e30f7123716ac21bce6271d5508b6f4acbe9e7fad20aae861d bytes=21 offset=START.EXE:0x7e87]`): the word pushed is `[0x49E2] * 225`, at most `255 * 225 = 57,375`, so no 16-bit overflow and `DX` is discarded |
| resident `0xA40:0x029C` (image `0xA400 + 0x029C = 0xA69C`) | takes the word `n` (`retf 2`); `n = 0` returns at once; else repeats `n` times: `al := es:[0]` (`es = 0x40`, the BIOS data byte at `0x40:0x0000`), `cx := [0x84FC]`, a call to the spin subroutine at image `0xA6BD` (`bx := 4; dec bx` spin, `cmp al, es:[di]`) and `loope 0xA6BD`; the loop continues while the BIOS byte is unchanged and `cx <> 0` after its decrement. `cx` is a 16-bit counter, so an initial `[0x84FC] = 0` wraps to `0xFFFF` and may run 65,536 iterations: **no iteration bound is asserted** (the Turbo Pascal `Delay` calibrated wait) |

`[0x49E2]` is set by the world-save loader from window-0 byte `0x1F8` (unit `0x6f` `+0x190B..+0x1910`, `mov al, es:[di+0x1F8]; mov [0x49E2], al`).
The slot-A save holds 4 there, so the word is **900**. `0xA40:0x029C` reads memory and changes no game state; its only effect is elapsed time,
which depends on the calibration word `[0x84FC]` and on the BIOS byte. This is why the handler is modelled with a host timing hook and no
duration is asserted.

## Model

`pr_u1c_op_delay` and `pr_res_0709_0dbe_arg` (`overlay_u1c_delay_271f.{h,c}`): the PC is advanced first, the argument word is computed
exactly (`byte * 225`), and the explicit host hook `delay_a40_029c(ms)` is called (NULL = HOOK status, nonzero = CALLBACK; the PC has advanced in
both cases). Tests (`test_overlay_u1c_delay_271f.c`) are fully synthetic: every value of `[0x49E2]` against an independent repeated-addition
restatement (no overflow), the PC advance observed from inside the hook, 16-bit PC wrap, failing and missing hooks, no other state touched.

## Private supplied-state probe (documented, not committed)

Hash-pinned slot A, run entry `0x9914`, search entry `0x9975`, the six-record host party of [SLOT_A_PARTY_INPUT.md](SLOT_A_PARTY_INPUT.md), merged handlers bound (through
APPROACH, and additionally GOSUB, RETURN and PRINT RETURN); explicit host inputs, none from DOS: `[0x84E2] = 0`, PICTURE DS bytes 0, first RANDOM result 13,
second RANDOM result AL2 in {0, 1, 2}, SURPRISE rolls 1..6, the `0x2B:0x3E` result in {0, 1, 5}; the timing hook only counts (no waiting), and `[0x49E2]` is
the saved 4.

For the 72 runs that reach SETUP MONSTER and APPROACH: DELAY at `0x9C28` is called once with 900, then the run continues APPROACH (`0x9C29`), PRINTCLEAR
(`0x9C2A`), COMPARE (`0x9C3C`), an IF-family opcode (`0x19`, `0x9C42`), PRINT (`0x9C5C`) and GOSUB (`0x9C60`) to `0xB6BC`. **Next stop** (all 72): UNMODELLED at PC `0xB6BC`,
opcode `0x2B` (HORIZONTAL MENU, handler `+0x10D1`, `2+n` operands), previous opcode `0x02`. The other 252 combinations still stop at ENCOUNTER MENU `0x29`.
Supplied-state probe only: no live DOS RNG, native search reachability or one-Up parity.
