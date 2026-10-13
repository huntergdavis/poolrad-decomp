# Saved slot-A Up: live RAM binding at the first script call

The replay-only `pr_slot_a_ecl_bind_ram` maps one native stop immediately
before unit `0x1C:+0x3171` into the existing `pr_rec_ecl_loader_state`.
It copies the three live little-endian windows, borrows the live script and
285-byte active character record, and maps the documented DS scalar fields.
The caller supplies the **stack argument** `0x9914`: pre-call `DS:[49ED]`
still holds `0x9AA5` and is not the new run entry. The resulting state can
be passed to the merged `pr_rec_ecl_run` with an explicit handler table.

Before implementation, `deja "Poolrad saved slot A Up 1C 3171 script PC
windows active record handler state pr_rec_ecl_run live native RAM wiring"`
found no direct session. The shorter `deja "3171"` found prior native
script-state watches and this lane's separate Up call-frame audit. Reused
and checked [the runner contract](U1C_ECL_RUN_3171.md),
[saved ECL entry map](SLOT_A_UP_ECL_GATE.md),
[same-save key gate and separate call frame](SLOT_A_KEY_GATE_TO_SCRIPT.md),
[save transfer map](SLOT_A_NATIVE_SAVE_ADAPTER.md), and the
[bounded RANDOM-to-EXIT example](SLOT_A_RANDOM_EXIT_CHAIN.md).

| Ignored input or span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Same slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Separate Up `+0x3171` RAM stop | 655,360 | `f71571a2f22f6264dc39f52424a9d4d300a3429f2d1c7932cec8735a98ae0558` |
| Original and loaded `+0x3171..+0x31B2` | 66 | `4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489` |
| Live script at `3AD6:0000` | 7,680 | `bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8` |
| Live windows 0 / 1 / 2 | 2,048 / 2,048 / 1,024 | `9890932b9bbe242f176efa7022d769a87e4417df2d7dd3772f970aca82826c86` / `cc41cd4e220d1575e980a7c4e3609e42a60736326958d8784bd3626b957fa634` / `00893ed7433feac0f1a7182d5eed9b0d6d4cebbc8c73352f30aa7abdf36c3aae` |
| Active record at `449A:0008` | 285 | `22efd553e03bcf9b89a35aaba1abaa9163b9811441c590c46206ec8c2c75e474` |

The live script is byte-equal to the save's `0x1401..0x3200` transfer.
The loaded runner body is byte-equal to the original 66-byte body. The
native stack at `SS:3EF0` holds return IP `0x345B`, CS `0x3582`, and
argument `0x9914`, identifying the gameplay `+0x3458` call. The active
record begins with a valid Pascal name length 14; no name or game text is
printed or committed.

The binder rejects a far span crossing the supplied RAM or its 64 KiB
segment, a record name longer than 15, and a nonempty native GOSUB call
list. It leaves the output state unchanged on rejection. It does not map
the previous operand scratch arrays or give the script a writable pointer;
a reached handler needing unsupported scratch or a class-3 script store
must stop at its own unresolved boundary. The caller owns RAM during the
run. Window writes live in the C state until a future replay adapter
deliberately commits them.

With an **empty** dispatch table, the hash-locked native state enters at
`0x9914`, fetches SAVE opcode `0x09`, and `pr_rec_ecl_run` returns
`PR_REC_ECL_RUN_UNMODELLED` after one dispatch. Binding the merged SAVE,
COMPARE, IF, GOTO, and AND callbacks makes the C runner visit callback PCs
`9914, 991A, 9920, 9927, 992E, 9934, 993A, 993B, 996B` in that order.
The tenth fetch is EXIT (`0x00`) at `0x9974`, left unbound, so the runner
again returns `UNMODELLED`, with PC `0x9974`. The live W1 word `0x6E7D`
was already 11 and the store flag was already set at the native stop;
their C end values do not prove a new native write. A synthetic callback
that returns `PR_REC_ECL_HANDLER_HOOK` makes the runner return
`PR_REC_ECL_RUN_HANDLER` with that exact handler status; a synthetic stop
callback checks the normal loop return. These outcomes are preserved, not
treated as successful one-Up progress. The native `+0x3171` stop is a
separate replay of the same hashed save as the earlier key gate; this work
does not establish a same-invocation bytewise transition between them or
a native post-run comparison.

Run `bash tests/replay/check_slot_a_ecl_live.sh` for strict C11 synthetic
checks. Pass ignored `GAME.OVR`, `SAVGAMA.DAT`, and Up breakpoint RAM paths
in that order for the hash-locked comparison. All inputs stay under
ignored `resources/`.

The next bounded step is to bind EXIT with its live environment and verify
the caller return, carrying any `UNMODELLED` or `HANDLER` result and any
missing engine side effect forward. The tested callback table is a focused
probe, not yet a production dispatcher for every ECL opcode.
`pr_slot_a_native_one_up` still returns `BLOCKED_KEY_DISPATCH`; there is
no coordinate-writer call, final tuple, full C-native parity, or added
game-routine count here.
