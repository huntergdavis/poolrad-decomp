# U6F +02AE entry, directory event, and IOResult ABI

`pr_u6f_02ae_step` begins at loaded `2A8E:02AE`, executes the real
stack prologue and Copy/mkdir/IOResult path, and stores AX in live
SS:[BP-2]. Results zero and five follow the source comparisons and return
through the actual saved BP and far frame. Other results stop before
`+02E9` message setup. The shared `+031F` suffix rereads the live result
word and can reach the retry head; it does not implement the intervening
message/display body. This is the first bounded slice of one original,
not a whole-function translation or a new strict credit.

The whole original is `OVL_006F_02AE`, GAME file `0x1E294..0x1E314`,
129 bytes/51 instructions, SHA-256
`ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539`.
GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
START.EXE is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

All six decoded direct caller sites in U6F have a preceding PUSH CS:
`0024→0027`, `01C0→01C3`, `07ED→07F0`, `160A→160D`,
`19E7→19EA`, and `1DC1→1DC4` (call→return IP). The older note's
single known `19E7` caller describes only the saved-route focus.
Unseeded or indirect callers are not excluded by this static inventory.

| Addressed scope | Bytes / source instructions | Implementation |
| --- | ---: | --- |
| U6F +02AE..02E8 and +031F..032E | 75 / 30 | New entry, comparisons, retry decision, generic return |
| SYSTEM +0B57..0B97 | 65 / 29 | Existing Copy instruction adapter |
| SYSTEM +17D6..17EA | 21 / 9 | Directory wrapper, including RETF 4 |
| SYSTEM +1800..181A | 27 / 17 | Pascal-to-ASCIIZ, 127-character cap |
| SYSTEM +181B..1829 | 15 / 9 | DOS helper with explicit INT event |
| SYSTEM +04A2..04A8 | 7 / 3 | XOR/XCHG error word and generic RETF |

Static objdump decoding checked all 30 new caller and 37 new SYSTEM table
starts/next addresses; the remaining SYSTEM instruction is the explicit
DOS event. No original instruction was executed in this audit.
The excluded U6F +02E9..031E message body is 54 bytes/21 instructions.

The adapter reuses `pr_slot_a_string_0356_prefix_step` for Copy and
`pr_slot_a_directory_step` for the validated DOS event at `131C:1821`.
It preserves all existing APIs and models. A missing event preserves state;
a supplied event must return complete CPU state and an ordered, validated
RAM journal. Carry-clear DOS completion does not clear an existing error;
IOResult exchanges DS:4426 with zero. No host filesystem result is inferred.
The older shared model is used only at the event instruction, so its
hardcoded wrapper-return checks do not restrict this adapter's actual RETF.

Run the focused fabricated-state checks:

```sh
bash tests/replay/check_u6f_02ae_abi.sh
bash tests/replay/check_u6f_02ae_abi.sh --sanitize
```

Twelve cases compare all 16 CPU fields and 655,360 RAM bytes at the
shared INT-to-IOResult boundary against the existing directory model.
They exercise zero/five/other errors, stale errors on successful service,
empty and capped pathnames. Additional checks cover all six recorded
caller return IPs, an arbitrary return CS:IP, live SS changes between
comparisons, retry selection, defined arithmetic flags, overlapping
forward copies, and immutable memory/event blockers. These comparisons
start from the new path's pre-INT state; they do not independently prove
its preceding instructions against an original-byte executor.

The existing Copy span limits remain explicit: RAM at most 1 MiB,
single-segment accesses without A20 wrap, and forward string operations.
Unsupported spans block atomically. Arbitrary wraps/aliases, complete
message/UI/key effects, generic repeated error attempts, and whole-original
CPU/RAM differential plus same-run native gates remain open. The existing
native run 040/044/045 notes supply separate evidence; this change reruns
none of those native or oracle gates. U6F +1446 remains a separate follow-on.

Prior-session recall timed out without reusable output. Reused evidence:
[directory source contract](U6F_POSTINSERT_02AE.md),
[directory CPU/RAM boundary](SLOT_A_DIRECTORY_FULLSTATE_C.md),
[IOResult decision](SLOT_A_NATIVE_U6F_02DA_DECISION.md), and
[saved return suffix](SLOT_A_NATIVE_U6F_031F_RETURN.md).
Memory and flag primitives follow the existing string adapter conventions.

Parity review: `target_function=OVL_006F_02AE`;
`source_candidates=u6f_02ae_abi.c` with the existing Copy/directory models;
`signals=exact extents, instruction boundaries, source call/return shape,
synthetic CPU/RAM comparisons` (four evidence categories, not a probability);
`verdict=unclear` for the whole original. The incomplete error body,
bounded spans and missing original/native execution prevent promotion.
