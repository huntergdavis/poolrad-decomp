# Saved slot-A Up key gate to the first script call

The next bounded dependency after the local
[key-return bridge](SLOT_A_KEY_RETURN_BRIDGE.md) is unit `0x1C:+0x3458`,
the first gameplay-loop call to script runner `+0x3171` after one Up. The
new `pr_slot_a_key_gate_next` reads explicit **live** window-1, caller-key,
exit-flag, and script-entry inputs at the `+0x33BD` gate. On the measured
Up slice it reports `PR_SLOT_A_KEY_GATE_BLOCKED_SCRIPT_3171` and the live
entry word; it does not run the script or move the party.

Before this work, `deja "Poolrad saved slot A one Up 1C 33BD 344C 3171
49E3 key gate window1 594 native C replay"` returned no direct match.
The merged [gameplay-loop source model](U1C_LOOP_3387.md),
[script-runner callsite audit](U1C_ECL_RUN_3171.md),
[saved ECL gate](SLOT_A_UP_ECL_GATE.md), and
[same-invocation native key watch](SLOT_A_NATIVE_KEY_CALL_RETURN.md)
supplied address and saved-input leads. The code spans, gate fields, and
separate native call frame below were independently hash-checked.

| Ignored input or span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Saved slot-A `SAVGAMA.DAT` in script-watch run | 13,137 bytes | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Gate/test `+0x33B9..+0x33D1`, GAME file `0x40FF..0x4117` | 25 bytes | original `1ddd391622c3fa430154267e383c5e87b98c345b5be24045c6b0fe73ef6f5dc6`; loaded `3c9de3e73f45621e739bad3af041d182cdfdc8f300b55015917fb1a01921e758` |
| Script-call arm `+0x344C..+0x345A`, GAME file `0x4192..0x41A0` | 15 bytes | original and loaded `78e63a36dc9b316d491208384157139d4d075881476e03b2e7bd870e4b2a313d` |
| Native key gate `+0x33BD` RAM | 655,360 bytes | `38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3` |
| Separate native `+0x3171` Up breakpoint RAM | 655,360 bytes | `f71571a2f22f6264dc39f52424a9d4d300a3429f2d1c7932cec8735a98ae0558` |

The two changed loaded bytes in the first span are in the runtime far-call
operand; the second span is unchanged. Both loaded hashes agree between
the gate and script-watch RAM images. Source decoding gives this bounded
route: window-1 word `+0x594 <= 1` falls through the unsigned `JA` at
`+0x33C3`; key `0x48` remains `0x48` after the upcase call and takes
`JNE +0x344C` at `+0x33D0`; zero exit flag `DS:49FF` falls through at
`+0x3451`; `+0x3453` pushes live `DS:[49E3]` before the near call at
`+0x3458`.

The hash-pinned gate RAM has local key `SS:3EF7=0x48`, window-1 word
`3A16:0594=0`, exit flag `DS:49FF=0`, and script entry
`DS:[49E3]=0x9914`. The C gate query returns the **host-supplied**
`0x9914`. In a separate replay of the same hashed save, the private
breakpoint-labelled `+0x3171` Up dump has the same four inputs and a
unique stack triple at `SS:3EF0..3EF5`:
`return IP=0x345B`, pushed `CS=0x3582`, argument `0x9914`. The return
IP identifies the `+0x3458` callsite; the distinct initial-entry and
search-entry callsites return at `+0x336A` and `+0x34EA` instead. These
are **two native runs**, so the evidence supports the same boundary and
argument, not a bytewise gate-to-runner RAM delta in one invocation.

The gate query deliberately returns `OUTSIDE_SLICE` for other key bytes,
window-1 words above 1, or a nonzero exit flag. Null and short window
inputs return `ARGUMENT` without changing the output. Synthetic tests
exercise those guards; the private test hashes the overlay, save, and
both RAM snapshots, checks both loaded spans, and compares the C result
with the native stack argument.

## Next integration blocker

The next reached call is the existing bounded `pr_rec_ecl_run` model for
unit `0x1C:+0x3171`. A C-native saved run still needs its live script,
PC, windows, active record, and handler state wired to the model, with
`UNMODELLED` or `HANDLER` propagated when a reached opcode or side effect
is unresolved. This gate result is a call argument, not script completion.
`pr_slot_a_native_one_up` remains `BLOCKED_KEY_DISPATCH`; there is no
coordinate-writer call, final party/clock tuple, routine count, or full
C-native parity claim here.

Run `bash tests/replay/check_slot_a_key_gate_boundary.sh` for strict C11
synthetic checks. Pass ignored `GAME.OVR`, `SAVGAMA.DAT`, gate RAM, and
script-watch RAM paths in that order for the hash-locked comparison. All
original data and captures remain under ignored `resources/`.
