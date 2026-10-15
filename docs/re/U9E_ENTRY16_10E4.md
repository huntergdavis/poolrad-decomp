# U9E public entry 16: byte result helper at +10E4

`pr_u9e_entry16_10e4` translates all of DOS v1.3 `GAME.OVR` unit `0x9E`
public entry 16. It takes two word arguments on a far-call stack frame, uses
the low byte of the second to compute a default result, and returns
`low(first)+1` when the low byte of the second is `0x12`. The default is
`low(second)+100`. The 16-bit `AX` can carry into `AH`; `RETF 4` removes both
arguments. The final CPU flags are the original `CMP` or `INC AX` flags.

## Source and scope

`deja "Poolrad U9E entry 16 +10E4 leaf full C parity 41 bytes"` timed out
locally. The address and caller inventory come from the checked-in
[overlay function ledger](overlay_function_ledger.csv); the 1 MiB CPU/RAM
parity layout follows the nearby [U9E entry 9 audit](U9E_ENTRY9_ROLL_LATCH_0E2E.md).

| Evidence | Value |
| --- | --- |
| Original `GAME.OVR` | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Entry 16 body | file `[0x2BFCE,0x2BFF7)`, U9E `+10E4..+110C`; 41 bytes, 17 instructions; SHA-256 `12ce30806a561605b7f7be238444a28a0d22f7cc53bda4872d6e0800330f41db` |
| Identity | public entry 16; ledger grade A, one far and three near call references; no calls inside; `RETF 4` at `+110A` |

The three near-call opcodes resolving to `+10E4` are at U9E `+1199`,
`+1220`, and `+1271`, all within the following `+115C` routine.

The translation executes the complete body from `CS:10E4` through the far
return, including the saved `BP`, two-byte local, wrap-capable stack reads,
full `AX`, final arithmetic flags and caller `CS:IP`. It has no callback or
host approximation. Invalid API inputs return before memory mutation; that
host guard is outside DOS parity.

`tests/recovered/check_overlay_u9e_entry16_10e4.py` verifies the exact
original slice and compares the C model to 16-bit Unicorn at the far-return
target. It compares all 1 MiB RAM bytes and all 14 CPU words across 70
synthetic entry frames: five first argument bytes, seven second argument
bytes, both paths, normal and wrapped stack layouts, and two flag patterns.
It requires Python `capstone` and `unicorn`; they may be installed under
ignored `resources/` and supplied through `PYTHONPATH`.
`tests/recovered/test_overlay_u9e_entry16_10e4.c` pins both result paths,
wrapped stack behavior and API rejection under strict C11 ASan/UBSan.

These are synthetic frames. Native reach remains provisional: no natural
saved-game caller frame has been captured for entry 16. Function accounting
remains with PM review.
