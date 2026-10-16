# U7D +03A3 complete caller and carry composition

`pr_u7d_update_03a3` translates the complete local U7D `+03A3..+03D4`
body and composes its sole near call with the existing complete
[`+02F2` carry normalizer](U7D_CARRY_02F2.md). It has no callback or new
callee translation. Game identity is provisional; this candidate adds no
strict full-C or reached-native credit.

## Original boundary and ownership

The owner-supplied `GAME.OVR` is 206105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit U7D starts at file `0x23DCC`. This function is exactly file
`[147823,147873)`, 50 bytes / 19 contiguous instructions, SHA-256
`3cef292a9346dadb89b80e93d5b78f701878140bf60bcd95aaba8d7022fc01ad`.
Its two conditional branches stay within the body; sole exit is `+03D4
RETF`, with no argument cleanup. The sole call is `+03AC CALL +02F2`,
preceded by `PUSH CS`. The called complete C routine is the distinct
177-byte `+02F2..+03A2` original, ending exactly where this one starts.

Three near-call encodings to `+03A3` align when decoded from their
containing U7D function starts: `+055A` from `+047A`, and `+0813/+083A`
from `+0721`. The classifier marks this address game-evidenced and
provisional. The census ownership screen reported no dedicated existing
implementation or active reservation in its available clones/ref set;
PM assigned this exact scope. These caller and state-global signals do
not independently settle game identity.

A bounded `deja` query for `U7D +03A3` timed out with no finding. Reused
repository evidence is the [versioned function ledger](overlay_function_ledger.csv),
the reviewed [`+02F2` source and note](U7D_CARRY_02F2.md), and the
[classification](overlay_classification.csv); the original body, call,
and callers were checked again from owner media.

## ABI and behavior

The API enters after a far CALL with CPU IP `+03A3`, a live SS return
frame, and exactly 1 MiB of A20-off RAM. It saves BP, pushes DS:6DC2
as a far array pointer with `PUSH CS; CALL +02F2`, then invokes the
merged callee C routine on the *same* RAM and 14-word CPU state. The
callee's `RETF 4` consumes its near return IP, pushed CS, and far
pointer argument. A changed return CS:IP stops at the callee-transfer
boundary with its state intact; the caller only continues when the
callee returns to its original `+03AF` continuation.

The caller compares DS:6DCC against zero. If nonzero, it multiplies
DS:3642 by the live DS:6DCC word, adds the low product word to
DS:6DCA, clears AX and DS:6DCC, and caps DS:6DCA at unsigned 99.
The high product word remains in DX. It then restores BP from live
SS RAM and executes `RETF`. RAM reads and writes, stack pushes, and
return-word reads retain source order. Each word's bytes are physically
contiguous with A20 off, but the CS word begins at the separately wrapped
16-bit SS offset `SP+2`; a linear `IP+2` address is wrong at SP FFFE/FFFF.
The last returned arithmetic FLAGS come from the zero comparison or
the compare with 99. The intermediate MUL's undefined FLAGS do not
cross an external call or survive the later arithmetic writers.

As in the original, malformed or cyclic input to the shared carry
callee can fail to return. The C composition has no host traversal cap.
The translation assumes immutable original code, as does the shared
callee. It models a callee return transfer as an explicit status rather
than inventing a destination instruction stream.

## Focused verification

The strict C11 CMake target and CTest pass; direct ASan+UBSan passes.
The focused C test includes a shared array/stack frame that changes the
callee's far return and verifies the explicit transfer status. It also
places distinct wrapped CS and linear decoy words at outer RETF SP FFFE
and FFFF. A composed entry SP 0008 fixture makes the merged +02F2 callee
pop its return IP at FFFE and CS at wrapped SS:0000, with a distinct
linear decoy. The original-byte differential runs **48** returning synthetic cases,
executing the exact 50-byte caller and exact 177-byte callee in Unicorn
and comparing the composed C's entire 1 MiB RAM and all 14 CPU words.
It visits **19/19** caller instruction starts and both outcomes of each
conditional branch (**4/4**). Cases include zero pending count, uncapped
and capped products, high-word MUL, a callee carry, A20 stack access,
variable arithmetic FLAGS, 40 seeded inputs, and a
DS/SS alias that overwrites live outer return words. Only returning
source paths are compared. Unicorn's RETF CS fetch differs at SP
FFFE/FFFF, so those frames are gated by focused C tests and the existing
U7F native decoy proof, not this Unicorn gate. No fresh DOSBox-X or
natural game call was launched under the host native hold.

```sh
cmake -S . -B resources/build-u7d-03a3 \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u7d-03a3 \
  --target poolrad_overlay_u7d_update_03a3_tests -j 1
ctest --test-dir resources/build-u7d-03a3 \
  -R '^poolrad_overlay_u7d_update_03a3_tests$' --output-on-failure
PYTHONPATH=/path/to/ignored/capstone-and-unicorn \
  python3 tests/recovered/check_overlay_u7d_update_03a3.py \
  --original /path/to/ignored/GAME.OVR
```

Original media and generated audit products remain under ignored
`resources/`. The candidate requires independent caller/identity review
and a reached native frame before any strict accounting decision.
