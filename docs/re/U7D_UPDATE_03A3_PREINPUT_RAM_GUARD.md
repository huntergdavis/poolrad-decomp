# U7D +03A3 slot-A pre-input RAM guard

This local closure prerequisite follows the merged
[saved-input availability gate](U7D_UPDATE_03A3_SAVED_INPUT_AVAILABILITY.md),
[E/R/I-or-D route](U7D_UPDATE_03A3_ERID_PREFLIGHT.md), and
[natural caller map](U7D_UPDATE_03A3_NATURAL_CALL_GATE.md).
The targeted `deja` route query stalled and was stopped without a recall
result; those merged notes provide the prior route. This gate rechecks the
private one-Up evidence numerically without replaying or editing the game.

`tests/recovered/check_overlay_u7d_03a3_slot_a_preinput.py` requires the
ignored DOS v1.3 media, slot-A save, and both specific 640 KiB RAM dumps.
It refuses changed sizes or hashes, checks original U1C +3395 bytes
`9A 2A 00 72 00` at GAME file `0x40DB` (SHA-256
`b588dcf500a335a6dc95ae0ca430a5ec17d6f1af71a4c94ca6ccf97f6915ae4f`),
and checks their relocated live form `9A 2A 00 96 08` at
`3582:3395` (SHA-256
`b280d21502c60f0f32bbfeea9e8ff2d36c2b21a1c7465b1052adee93e2ed7a2d`).
The reached U68 entry-2 stub at `0896:002A` is still the cold
`CD 3F 89 09 00` form (SHA-256
`2fcb835df8ea8b601f72d1110db582051d9a21fccbd9b3d33300e9f57732eb57`).

The pre-input `+3395` RAM SHA-256 is
`4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc`;
the post-Up `+33BD` RAM SHA-256 is
`38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3`.
Using the live watch's measured `DS=14A0`, both dumps have
`DS:[49D6/49D8]=3A16:0000`, word at window-1 `+0594=0`, and
`DS:[49FF]=0`. These are values in the **Up** run. The pre-input guard
does not prove those values remain unchanged after an E key returns.
The prior native transcript reports the actual returned key `0x48`.
Neither 640 KiB dump includes the upper 384 KiB of a full A20-off RAM
image, and neither is a restartable emulator snapshot or a complete
CPU14 capture.

## Full-state natural-route contract, held

For a future single natural E/R/I-or-D run, retain one run ID, ordered
input event/ordinal, actual breakpoint address, 14 CPU words
`AX BX CX DX SI DI BP SP CS IP DS ES SS FLAGS`, and a full 1 MiB
A20-off RAM image at each required cut. Hash each private file, pin
loaded instruction bytes/segments against the original spans, and
retain the raw event transcript. Never splice an Up dump into an E
run or supply an invented key-return CPU state.

1. At the first reached U1C +3395 call, record its live far target,
   window-1 pointer and readable extent, word +0594, DS:[49FF], prompt
   state and SS frame. Send E only after that stop; at the actual input
   return and +33BD/+33E2/+33E7 cuts, record the returned key and both
   guard outcomes. A different branch is an explicit nonreach.
2. If +2DD3 and U6A entry 1 are actually reached, record loaded U6A
   CS and each menu input. Attempt R only at the observed R menu;
   distinguish the +1FA2 and +149D call sites. Record +077A and its
   far call to U7D public entry 2, or stop at the actual transfer.
3. If U7D entry 2 and its menu are reached, record I or D, +047A if
   taken, and the actual predecessor +055A, +0813, or +083A. At the
   +03A3 entry, preserve the live SS return IP/CS, all CPU14 and RAM;
   ordinary return IP candidates are +055D, +0816, and +083D, but
   live words decide. Capture the native outer return or nonreturn.

Replay the merged `pr_u7d_update_03a3` only from the *same-run*
reached +03A3 CPU14 and 1 MiB RAM. Compare every CPU word and RAM byte
at the actual outer return, including the inner +02F2 call, SS offset
wrap and A20 aliases. The controlled C parity fixtures remain separate
evidence. Host heavy native execution is held, so this branch adds no
E route, natural frame, full-function proof, or strict-C credit. The
project strict count remains 82.

Static/private check only:

```sh
python3 tests/recovered/check_overlay_u7d_03a3_slot_a_preinput.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --save <ignored-slot-A-SAVGAMA.DAT> \
  --pre-input-ram <ignored-pre-call-3395-ram.bin> \
  --post-up-ram <ignored-loop-gate-33bd-ram.bin>
python3 tests/recovered/check_overlay_u7d_03a3_natural_route.py \
  --original resources/DOS/POOLRAD/GAME.OVR
```
