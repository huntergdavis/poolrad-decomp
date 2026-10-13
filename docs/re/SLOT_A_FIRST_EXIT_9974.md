# Saved slot-A Up: first ECL EXIT and caller-return boundary

The focused `pr_slot_a_first_exit_run_ram` composes the merged SAVE,
COMPARE, IF, GOTO, AND, EXIT and `pr_rec_ecl_run` models on the live
saved-slot-A Up input at script PC `0x9914`. It binds the native EXIT
environment (`DS:[4430]`, `[4436:4438]`, `[5D92:5D94]`, `[84DE]`) and
does not invent a resident box hook. On this measured input the C runner
fetches EXIT at `0x9974` as its tenth dispatch, returns `OK`, and retains
PC `0x9975` after the EXIT handler increments it. This is a bounded C
composition, not a complete C-native Up replay.

Before this work, `deja "slot A Up ECL EXIT 9974 1C 0001 3171 3458
caller return native RAM"` and the capture-filename query found no direct
indexed session. Reused and rechecked: [the merged EXIT model](U1C_EXIT_0001.md),
[runner/callsite contract](U1C_ECL_RUN_3171.md),
[the saved-entry route](SLOT_A_UP_ECL_GATE.md), and
[live input binding](SLOT_A_ECL_LIVE_BIND.md). The ignored run003 RAM
images below supply new numeric checkpoints.

| Ignored input or span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| run003 Up `+0x3171` entry RAM | 655,360 | `f71571a2f22f6264dc39f52424a9d4d300a3429f2d1c7932cec8735a98ae0558` |
| run003 AND `+0x0DEF` entry RAM | 655,360 | `9c82fae60d03939669c8f8a256a96ad0910376c80e07f31ade33eadc1ba04f95` |
| run003 later search `+0x3171` entry RAM | 655,360 | `cac6b2b97ea481dbb4bf4c165dec5035bc769beac80773e5fa661bab1600ee1d` |
| EXIT original unit `0x1C:+0x0001..+0x00F3` | 243 | `9f2483f23b51d3dbc54a83592241cbd6f5bc27e1173e62f17c54c3b6672ca493` |
| EXIT loaded at `3582:0001` | 243 | `c09b0f2dd45cb0ccad3b25afa54ea0e885dff87873679d630319ee47310d70ff` |
| Runner original and loaded `+0x3171..+0x31B2` | 66 | `4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489` |

The audit checks all source/RAM hashes, byte-equal live scripts against the
save transfer, and every original/loaded code byte. EXIT has twelve loaded
byte differences in six adjacent pairs at `+0x002D/+0x002E`, `+0x0032/+0x0033`,
`+0x005F/+0x0060`, `+0x0064/+0x0065`, `+0x008F/+0x0090`, and
`+0x00CF/+0x00D0`; the runner body is unchanged. These are observed
loaded-byte differences, with no new relocation-class claim.

## Native checkpoints and bound effects

The first native call frame has return IP `0x345B`, CS `0x3582`, argument
`0x9914`. The later AND handler stop has `DS:[49ED]=0x996B` and previous/
current opcodes `01/2F`, matching the C route's ninth callback PC. At that
stop, box gate `[84E2]=0`, active-pointer restore flag `[4430]=0`, and the
GOSUB call-list head `[6F70:6F72]=0:0`. The same values are zero at the
pre-call stop, so EXIT's resident box hook and heap release are not reached
on this measured input. The active pointer and saved copy both remain
`449A:0008`.

The C callback PCs are `9914, 991A, 9920, 9927, 992E, 9934, 993A,
993B, 996B, 9974`. EXIT returns `PR_U1C_EXIT_OK`, reports zero boxes,
zero freed nodes and no pointer restoration, sets the stop byte, and
increments PC. The runner then clears its stop byte and returns with PC
`0x9975`, previous/current opcodes `2F/00`, `[5E24:5E25]=01:11`, and
the active far pointer unchanged. These selected C outputs match the
later native search-call RAM. Its stack uniquely holds return IP `0x34EA`,
CS `0x3582`, argument `0x9975`, identifying the gameplay caller's later
search-entry call at `+0x34E7`.

There is **no direct native EXIT-entry or `+0x31B0` return capture**.
The later search-call stop occurs after caller work: native x changed
`15 → 14` and minute `0 → 1`, while the C EXIT probe retains x 15 and
minute 0. The selected matching fields support the EXIT/return boundary
and show the caller progressed to search; they do not establish a
bytewise EXIT-only RAM delta or coordinate/time attribution.

The probe returns a binding status and separately reports the raw runner
and handler statuses. A synthetic nonzero box gate with no hook returns
`PR_REC_ECL_RUN_HANDLER` / `PR_REC_ECL_HANDLER_HOOK` before EXIT clears
state or increments PC. A synthetic unbound opcode returns `UNMODELLED`.
A nonempty native GOSUB list is rejected by the live binder before running.
No such blocker is converted to success.

Run `bash tests/replay/check_slot_a_first_exit.sh` for strict C11
synthetic checks. Pass ignored `GAME.OVR`, `SAVGAMA.DAT`, Up pre-call RAM,
AND-entry RAM, and later search-call RAM paths in that order for the
hash-locked comparison. All private data stays under ignored `resources/`.

`pr_slot_a_native_one_up` remains `BLOCKED_KEY_DISPATCH`. The C probe stops
at the script runner return and does not execute the gameplay caller,
coordinate writer, or search script. There is no full one-Up parity,
playable claim, or added game-routine count. The next measured boundary
is the caller path from `+0x345B` to the search-entry call `+0x34E7`.
