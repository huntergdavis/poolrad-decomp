# U7D +03A3 saved-input availability gate

This is a source-only follow-on to the merged
[natural caller route](U7D_UPDATE_03A3_NATURAL_CALL_GATE.md) and
[E/R/I-or-D preflight](U7D_UPDATE_03A3_ERID_PREFLIGHT.md). The targeted
`deja` query for this route returned no indexed session. Those two notes
provide the original-byte route and input guards; this gate checks the
availability and identity of the private starting evidence. It does not run
the game or change a C translation.

The ignored DOS v1.3 `GAME.OVR` is 206105 bytes with SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The merged `check_overlay_u7d_03a3_natural_route.py` passes against it.
That checker pins U1C's E comparison and call, both U6A R menu branches,
U6A's far call to U7D entry 2, the U7D menu tests, and all three near
predecessors of +03A3. It proves static transfers only.

## Available private inputs

A separate ignored evidence checkout contains the slot-A save and the
one-Up watch described in
[SLOT_A_NATIVE_KEY_CALL_RETURN.md](SLOT_A_NATIVE_KEY_CALL_RETURN.md).
The files were read and rehashed locally; no bytes were copied into this
public branch.

| Private artifact | Bytes | SHA-256 | What it establishes |
| --- | ---: | --- | --- |
| `SAVGAMA.DAT` | 13137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` | Same saved slot-A input used by the one-Up watch. |
| RAM at U1C +3395 before the input call | 655360 | `4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc` | Observed pre-input state for the **Up** run. |
| RAM at U1C +33BD after the input return | 655360 | `38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3` | The **Up** run's window-1 gate state. |

The corresponding live watch reported `3582:3395`, `DS=14A0`,
`SS:SP=1D16:3EF6`, and window-1 `+0594=0`; it returned key `0x48`
(Up). The RAM dumps are observations, not restartable emulator state.
No U1C E-input return, U6A R menu, U7D I/D menu, or U7D +03A3
entry/return frame is present in these files. The private run directory
has the saved DOS files but no preserved emulator checkpoint in its
`.dosbox-saves` directory. Reaching E therefore requires a new run from
the pinned save after the host native hold is lifted.

## Bounded next observation

Reprepare the same save in an isolated ignored run. At the first reached
U1C +3395 input call, pin loaded CS, DS, SS, SP/BP, window-1 pointer and
word +0594, DS:[49FF], CPU14, and the 1 MiB A20-off RAM before sending
E. Then capture the actual key return and +33BD guard. If the route does
not reach +2DD3, record its real branch and stop. Only after an observed
E path should an R input in the reached U6A menu and an I or D input in
the reached U7D menu be attempted. Keep +055A, +0813, and +083A caller
sites separate: their normal return IPs are +055D, +0816, and +083D,
but the live stack words and loaded CS decide the actual return.

At a genuinely reached +03A3 entry, compare the merged C composition
against native CPU14 and all 1 MiB RAM at the actual outer return.
The existing controlled parity and wrap fixtures do not substitute for
that frame. Under the current host native hold this remains a capture
plan. Natural reach, complete gameplay route, and strict full-C credit
are unproved; the project count remains 82.
