# Conditional first ECL dispatcher boundary

`pr_slot_a_runner_first_dispatch` exposes the existing raw CPU model of unit
1C `3171` until the first dispatcher entry `2F2D`. The caller supplies a full
1 MiB RAM image, the actual entry CPU, and independently authenticated 66-byte
runner code. Input CPU, RAM, and code-reference objects must not overlap; the
caller must also ensure runner code and writable stack/data do not alias. A set
`DS:49FF`, incorrect entry IP, short backing or mismatched code refuses before
writes. The runner clears `442E`, initializes `49ED`, latches the first opcode,
and pushes the dispatcher return frame. No dispatcher or handler is executed.

This is conditional attribution using the pre-existing differential-tested
`pr_u1c_run_3171` model and its dispatcher boundary; see
[CANDIDATE_SCOUT_DS41B_BATCH2](CANDIDATE_SCOUT_DS41B_BATCH2.md) and
[U1C_ECL_RUN_3171](U1C_ECL_RUN_3171.md). It is not an observed first-Up raw
entry CPU, a script return, or a full-Up claim. The retained script-entry RAM
and direct `345B`/`348D` RAM are from different invocations. Indexed prior
session `01a13101-709d-77e1-9fcb-2a6acbff0046` reports a second-key
entry-to-return delta; its filenames and per-write attribution have not been
established here. No same-run first-Up entry/return pair is claimed.

Run the strict synthetic test with:

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_runner_first_dispatch.c src/recovered/overlay_u1c_run_3171_cpu.c tests/replay/test_slot_a_runner_first_dispatch.c -o /tmp/slot-a-runner-prefix
/tmp/slot-a-runner-prefix
```

Next is an evidence-backed real dispatcher and reached handler effects, plus
same-invocation entry CPU/RAM and `345B` state. Do not treat the callback stop
as completion or serialize projected fields into a fabricated guest return.
