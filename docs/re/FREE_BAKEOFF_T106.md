# t106 bounded free-model bake-off

Run 2026-10-27 in the `free_bakeoff.sh` sandbox on two bodies whose verified C
exists on main: `OVL_0025_0136` (GAME.OVR offset 30217, 128 bytes) and
`OVL_0025_06AF` (offset 31618, 36 bytes). Five candidate free models, 10 runs,
parallel 3, 600 s per run. The systemd free pool was **not** restarted; the
harness calls the models directly.

| Model | 0136 | 06AF |
| --- | --- | --- |
| `recursive/flint-v1.0:free` | EMPTY | EMPTY |
| `thinkingmachines/inkling:free` | OFFSET UNRESOLVED | OFFSET UNRESOLVED |
| `cohere/north-mini-code:free` | OFFSET UNRESOLVED | OFFSET UNRESOLVED |
| `nvidia/nemotron-3.5-super-vl:free` | OFFSET UNRESOLVED | BYTES DIFFER |
| `xingchen-agi/xing4.0-29b-a4b:free` | EMPTY | EMPTY |

**Verified under the current strict byte judge: 0/10.** No body produced a
byte-identical dump in this run.

## What this does and does not test

- The harness checks a quoted dump at the body offset and scores byte identity;
  it does **not** compile a C draft, so the task's "compiles" criterion is not
  reached here.
- Earlier flint runs (2026-10-24, older checker) scored `OVL_0025_06AF` and
  `OVL_0025_0136` VERIFIED. The current strict checker rejects the 2026-10-27
  reports (empty, or an offset/dump declaration it cannot resolve); this is a
  model-output/harness result, not a claim that flint cannot decode these
  bodies.
- The unresolved-offset rows did argue about the unit/offset in prose but did
  not provide the single requested `FILE OFFSET: <decimal> (0x<hex>)` and a
  quoted dump in the accepted form.
- Limits: two bodies, one run each, no independent reviewer, no C compile, no
  whole-body/parity/credit claim. The free pool stays stopped.
