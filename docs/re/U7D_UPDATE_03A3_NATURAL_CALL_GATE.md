# U7D +03A3 natural-call closure gate

This is a static route and capture packet for the already merged
[complete +03A3 caller](U7D_UPDATE_03A3.md), its
[complete +02F2 callee](U7D_CARRY_02F2.md), and the separately merged
[wrap/alias fixture](U7D_UPDATE_03A3_WRAP_ALIAS_GATE.md). It changes no
C translation. PM assigned the natural-call gate after PR #682
(i7782). A bounded `deja` query for a U7D +03A3 natural caller
checkpoint timed out without a result. The three cited checked-in
notes supplied the earlier source/ABI boundaries; this packet checked
the route bytes against ignored original media again.

## Original route and identity limit

Owner-supplied DOS v1.3 `GAME.OVR` is 206105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U7D file base is `0x23DCC`. Static slice hashes are:

| U7D span | Size | SHA-256 | Role |
| --- | ---: | --- | --- |
| `+03A3..+03D4` | 50 | `3cef292a9346dadb89b80e93d5b78f701878140bf60bcd95aaba8d7022fc01ad` | translated target, outer `RETF` |
| `+047A..+0564` | 235 | `0c687ca966432b6c241673264978796173f6f31e91905a8a67ee3dee5110bc4e` | conditional predecessor |
| `+0721..+0870` | 336 | `1d3e90cba8a713d25f6d090da93c5d5af02ffa2adf1df7de27e088dcb3409f16` | menu loop, including trailing data |
| `+0C9A..+0E5F` | 454 | `9964836108967641b80c7c0983446af0b66055ff0bb2ad2302c626846ed8d631` | public entry 2 |

Public entry 2 `+0C9A` calls `+0721` at `+0D1C`. In that menu loop,
comparison with byte `0x49` takes the path to `+0813 CALL +03A3`
after an indexed count increment. Comparison with byte `0x44` takes
the path to `+0829` or `+0836 CALL +047A`, followed by `+083A CALL
+03A3`. The `+047A` predecessor can itself call `+03A3` at `+055A`
after an indexed subtraction. Public entry 2 also calls `+047A`
directly at `+0D9D` on a later route. The seven exact `E8` relative
targets and menu comparisons are checked by the static script. The
`+0721` ledger span contains trailing data; its full byte size is not
an instruction count.

The upstream static route starts in the U1C saved gameplay loop. Its
key `0x45` branch at `+33E2..+33E7` calls near `+2DD3` only after the
loop's window-1 and `[0x49FF]` guards. `+2DD3` far-calls U6A public
entry 1 at `+2DEA` (`75:0025`). In that menu, key `0x52` at `+1F98`
calls U6A `+077A` at `+1FA2`. A second U6A menu at `+13F1` has its
own `0x52` branch and call at `+149D`. U6A `+077A` computes party
time/count fields, then far-calls U7D public entry 2 at `+07FD`
(`8C:002A`). The upstream source pins U1C `+2DD3..+2E0F` (61 bytes,
SHA-256 `2097c463a248c594fa9d715b4c46d24335343fb630be7c9106df9a9369ae147e`)
and U6A `+077A..+0823` (170 bytes, SHA-256
`ffd763031e2e9fd1ddb8ce34b9789a383443844c57ef3901170fedd2790990c4`).
This gives a **candidate** saved-slot-A action sequence: reach the
gameplay key loop, choose `E`, then `R` in the U6A menu, then `I` or
`D` in the U7D menu. The documented slot-A load reaches the gameplay
loop, but this exact sequence and its guards have not been replayed
or captured natively.

This identifies a *candidate* action path through the public entries
and menu choices. The available replay JSON has no U7D caller marker,
and a directory of saved DOS files alone does not identify a reached
checkpoint or the exact input sequence. No saved-game entry frame or
native outer return has been captured for these calls. Public entry 2
and the menu edge therefore do not upgrade the provisional game
identity or strict full-C accounting.

## Live-frame conditions to capture

At the actual +03A3 entry, preserve all 14 CPU words and 1 MiB A20-off
RAM before calling the C translation. Record loaded U7D CS and the
exact predecessor call site (`+055A`, `+0813`, or `+083A`), because its
outer return IP must respectively be `+055D`, `+0816`, or `+083D`.
Record the live SS return words, BP/SP, DS:6DC2..6DCE array,
DS:363A..3646 carry words, DS:6DCA/6DCC count/accumulator, and
their physical aliases with SS and loaded code. The inner +03AC
near-call enters +02F2; normal continuation is +03AF after its
`RETF 4`. Any changed callee return, nonreturn, or code alias needs a
separate outcome, not a fabricated outer return. At +03D4, outer
`RETF` reads IP and CS through separately wrapped SS offsets.

## Held, bounded native plan

**Precondition:** verify an exact saved-slot-A checkpoint and scripted
`E`, `R`, then `I` or `D` route that reaches public U7D entry 2 and
one of its +03A3 predecessors. The U1C window-1 and `[0x49FF]`
guards, input timing, and intermediate menus must be observed. Pin
the checkpoint files, both DOS media hashes, loaded U7D CS, and the
input script. A reached +03A3 frame is not currently demonstrated.
After PM releases a host native slot, run that one checkpoint/input
under a 90-second wall-clock timeout, with breakpoints at U1C
`+33E7/+2DEA`, U6A `+1FA2/+07FD`, U7D entry 2 `+0C9A`, menu call
`+0D1C`, predecessor calls `+055A/+0813/+083A`,
target `+03A3`, inner call/return `+03AC/+03AF`, and outer `+03D4`.
Capture CPU14 and all RAM at +03A3 and at the *actual* outer return;
record which branch and return destination occurred. Replay the merged
C caller from exactly the captured entry state and compare all 14 CPU
words and all 1 MiB RAM bytes. If no target call is reached within 90
seconds, stop and report nonreach. A natural frame that does not wrap
SP cannot prove native SP=FFFE/FFFF outer `RETF`; the merged controlled
fixtures remain separate evidence. No emulator run is authorized under
the current host hold.

## Adversarial parity verdict

| Field | Finding |
| --- | --- |
| `target_function` | U7D +03A3..+03D4, with +02F2 callee |
| `source_candidates` | merged complete C composition; no competing owner identified |
| `signals` | original extents/hashes, seven call targets, public-entry/menu chain, 48 returning synthetic full-state original-byte cases, wrap/alias C fixtures |
| `score` | strong bounded source parity; zero natural reached U7D frames |
| `verdict` | `unclear` for natural game/full-function closure |
| `review_notes` | checkpoint/input, loaded CS, actual return state, and U7D-specific native wrap proof remain open; +047A identity is unresolved in the classifier |
