# The RES_0802 cluster composed end to end: the 0716 re-run with real leaves (t30, VP i20311 / i20371)

This document reports the receipt that has been the condition on 0716 since #1322. It covers 0716 run **with every one of
its seven near-call stops replaced by the real leaf model**, and its QUIT by the credited `0899:0000`. The run is one
execution of the composed C against one execution of the original.

**No body and no credit.** `pr_r0802_0716_composed` (`src/recovered/resident_0802_0716_composed_cpu.c`) is a driver. It
calls the twelve cluster models already on main and adds no code of the game's.

## What runs

| 0716 site | Before (#1322) | Now |
| --- | --- | --- |
| 0775 OPEN | named stop | `pr_r0802_05cd`, with 025D under it; 025D's prompt runs the real `pr_r0802_0193` and resumes 05CD at 02ADh |
| 084A PROMPT | named stop | `pr_r0802_0193` (0175, 0105, 0040, ReadKey 0899:0059 and its closure) |
| 084D QUIT | terminal stop | `pr_r0899_quit_0000_cpu` (credited) to Halt |
| 08AD, 091D CLOSE | named stop | `pr_r0802_06e7` |
| 08BC ALLOC | named stop | `pr_r0802_0020` (GetMem) |
| 0935 DECODE | named stop | `pr_r0802_050f` |
| 0941 FREE | named stop | `pr_r0802_0000` (FreeMem) |

- Each leaf must return to the instruction after its call in 0716's segment; otherwise the run stops **CALLEE**.
- The run can end only in an architectural end state:
  - RETURN;
  - EXIT (Halt `0AF8:00E9`);
  - RUNERROR (`0AF8:00E2`);
  - DIVIDE;
  - BREAK (ReadKey's Ctrl-Break);
  - STALLED (050F's proven fixed point).
- Otherwise it is a declared LIMIT or SERVICE refusal, which counts as non-architectural and is counted.

## The receipt (VP item 1)

Receipt: 2026-10-21 04:06:44 PDT (`resources/0802-0716-composed/final.json`, sha `cc41eb20…`), on a branch from main
`62871450`. The branch was then rebased onto `6be8393b`, whose new commits change only accounting tools (no `src/`
or `include/` file). The model side composes 49 C sources. The per-case progress log is `resources/0802-0716-composed/progress.log`.

**84 cases, all equal; non-architectural stops: 0 (measured).**

- Each case runs the ORIGINAL once from 0716's entry to its end state. Every byte of the cluster and its runtime closure
  executes: SYSTEM, the Dos unit, Crt, and the 08AE driver.
- Each case is compared unmasked at the end state:
  - CPU14;
  - all 1,048,576 RAM bytes;
  - **the DOS file-service state (handles and positions) and its ordered log**, so the two-handles-one-file mechanism of
    05CD is compared (VP i20376 item 2);
  - the ordered environment event log (INT 16h, INT 21h 2Ch/25h/35h, ports).

| End state | Cases | Where |
| --- | ---: | --- |
| RETURN | 67 | the 29 real ECL blocks, absent blocks, synthetic archives, positions, flags, insert-disk paths |
| RUNERROR | 8 | six read faults (IOCheck), the heap failing in 0020's GetMem and in 0716's own GetMem |
| EXIT | 8 | `packed-limit-prompt`, `packed-limit-before-match`, `position-negative`, and the five dirty-flag prompt cases (0716's prompt, then Quit) |
| STALLED | 1 | `stall-7f-through-0716` |

- **Seams composed: 324 leaf calls run to their return, with 13 prompts (0193).**
  - Distribution per case: 50 cases × 5 seams (open, alloc, decode, free, close), 22 × 2, 7 × 1, 1 × 0, 1 × 3, 1 × 6,
    2 × 7.
- **The dirty incoming flags 0202/0602/0A87/0402/0E57** are run both on an ECL1 block (RETURN) and on the over-limit prompt
  path (EXIT). All ten are equal.
- `packed-limit-before-match` ends in **EXIT** for a row that is not the wanted block. This is the composed confirmation
  that 0716's QUIT is data-dependent: the 8CA0h test precedes the id compare.

## Leaf stops surfacing through the composition (VP item 2)

- **050F's STALLED surfaces as STALLED at the top.** 0716 has no path of its own for it: the hang propagates, and 0716 never
  regains control.
  - The original reaches the **same fixed point at the same point**. Unicorn stops at the 050F loop-head arrival that the C
    reports, and the full state is equal there.
  - Unicorn then runs 1,000 further arrivals with the CPU identical and the RAM unchanged.
  - The hang is neither earlier nor later in the model.
- **025D's PROMPT under 05CD is composed, not surfaced.** The real 0193 runs and returns to 02ADh, and 05CD continues.
  - Cases: one prompt (`insert-disk-through-05cd`, 6 seams), two prompts (`insert-disk-two-prompts`, 7) and type-ahead
    drained by the first prompt (`insert-disk-typeahead-drained`, 7). All RETURN, equal on file state and logs.

## Seams that failed under substitution (VP item 3)

**None in the final run.** The first run failed one case, and the cause was the fixture, not a seam:

- **Run #1.** 72 cases were equal, including the stall and the one-prompt insert-disk path. Then `insert-disk-two-prompts`
  stopped inside the **second** 0193 called from 025D.
- **Measured on both sides:** the original and the C hit the same environment refusal at the same point, an INT 16h wait
  with an empty key script.
  - The composed audit had inherited the 0193/ReadKey audits' keyboard model, in which AH=01h peeks the whole script.
  - ReadKey 0059 waits and then drains, so the **first prompt consumed all three scripted keys**: `16/0` takes one, `16/1`
    sees the next, `16/0` drains it, and so on.
  - The second prompt's wait then found nothing. The original raised the audit's environment refusal; the C keyboard
    callback refused, and the driver surfaced that as CALLEE.
- **Fix:** the canonical contract below, on both sides. A keyboard refusal now maps to SERVICE, not CALLEE.

**Mutation checks of the driver:**

- **A planted AX corruption after ALLOC was NOT caught, and correctly so: it is an equivalent mutant.** At 08BF, 0716
  goes straight on to `LES DI,[BP+0Ah]; PUSH ES:[DI]; LCALL GetMem`, so AX is dead across that seam (decode).
- **A live corruption (BP ^= 2 after ALLOC) fails at once** (`ecl1-id18: C RUNERROR original RETURN`). The source was
  restored byte-identical.

## The keyboard contract, reconciled (VP i20379)

**Canonical:** the #1326 contract, stated over two declared lists:

- the **type-ahead**: keys already in the BIOS buffer when the code runs;
- the **waited keys**: keys pressed later, one per wait.

AH=01h reports the head of the type-ahead only. AH=00h takes the type-ahead first, otherwise the next waited key.

**The peek model is not a different DOS. It is this contract with the whole script declared as type-ahead and no waited
keys:** AH=01h reports script[0] and AH=00h pops it, in both.

- So every case evidenced under it reproduces unchanged under the contract, including the 0899:0059 audit's multi-key
  drain cases (`drain-3`, `drain-ext`, `zero-then`, `ctrl-s-drain`) and 0193's `break-drain`.
- What the peek model cannot express is a key pressed **after** the code has started waiting. That is exactly what a
  multi-prompt composition needs, one key per prompt.
- **No DOS run decides between them**, and none is needed. Both are faithful to "AH=01h reports the BIOS buffer"; they
  differ only in which keys are declared to be in the buffer beforehand. The prompt path has no native corroboration in
  this cluster (declared Unicorn-only, as in 025D/05CD), so no real-DOS keyboard evidence exists either way.

**Affected evidence:**

- **025D and 05CD** already used the canonical contract (#1326, #1328), not the peek model.
- **0193 and 0899:0059** used the peek model. Their claims stand as written **because of the equivalence above, whatever
  the number of waits in a case**. Each of their scripts is exactly the canonical contract with every key declared
  type-ahead, so the original and the model saw the same keys in the same order. What those audits do not cover is a key
  arriving only after a wait has begun. That is a coverage statement, not a defect: no such case exists in either audit,
  and the number of waits per case was not measured.
  - Their docs should say: "all scripted keys are declared type-ahead; no waited keys". That is par-d's text to add, and
    the request has gone to the VP.

## Waited-key coverage (VP i20386)

A **waited key** is one that arrives only after a wait has begun. While it is still waited, AH=01h must report no key; this is
where the peek model and the canonical contract differ. **It is covered, by named cases, in two audits:**

- **This composed run, under the canonical contract.** Every scripted key is a waited key unless the case declares
  type-ahead.
  - `insert-disk-two-prompts`: two waited keys, one per prompt. At the first prompt, ReadKey's drain asks AH=01h while the
    second key is still waited, gets "no key", and leaves it for the second prompt, which takes it. Under the peek model the
    same script was consumed by the first prompt; that was run #1's failure.
  - `insert-disk-typeahead-drained`: type-ahead (drained at the first prompt) **plus** two waited keys.
  - `insert-disk-through-05cd`, `packed-limit-prompt`, `packed-limit-before-match`, `position-negative` and the five
    `dirty-prompt-*` cases: one waited key each.
- **The 025D audit (#1326), under the same contract:** `insert-disk-2-prompts` (two waited keys) and `typeahead-drained`
  (three type-ahead keys plus waited keys).

**Not covered:** the 0193 and 0899:0059 audits standalone. Their scripts are all type-ahead (see above). A waited key reaches
0193 and ReadKey only through the composed run and the 025D audit, which is where every prompt loop in this cluster lives.

## Loop budgets (VP i20444, i20463)

The driver has two loops. Both are bounded, but not by a budget:

- The **seam loop** re-enters 0716 after each leaf. It is bounded because 0716 advances at every seam.
- The **05CD loop** re-runs 05CD after each insert-disk prompt. It is bounded only by input: every re-prompt consumes one
  scripted key, and ReadKey refuses when the script is empty.

A structural bound is not a budget. A defective model can break the structure, and then the structural argument proves nothing. That is exactly what happened at 0768 (#1357), where a mutant removed the row increment and the model spun for 67 minutes. A budget is the only bound a bug cannot remove (VP i20444, i20463). The receipt above predates this change. Before it, both loops had no counter, and my report i20456 (08:06 PDT) claiming they
did was wrong (corrected in i20457).

Each loop now has a declared budget. Running past it stops **LIMIT** with nothing of the next step executed:

| Budget | Counts | LIMIT state | Original stopped at |
|---|---|---|---|
| `max_seams` | leaf calls begun from 0716 | the leaf's entry; `seam` = the call site | the leaf entry whose previous instruction is one of 0716's eight call sites (`0775 084A 084D 08AD 08BC 091D 0935 0941`) |
| `max_disk_prompts` | insert-disk prompts per 05CD seam | 0193's entry; seam `0775`, nested `02AA` | the arrival at `0193` with `[SP] = 02AD`, counted per 05CD seam |
| `retries` | 025D's taken `02B7` loop-backs, via 05CD | IP `02B7`; seam `0775`, nested `02B7` | the taken `02B7` arrival, counted per 025D entry and per resume |

**Re-run, 2026-10-21 08:20-08:22 PDT:** 87 cases. The 84 earlier cases are unchanged: RETURN 67 / RUNERROR 8 / EXIT 8 /
STALLED 1, 0 non-architectural stops. There are 3 new budget cases, each LIMIT and equal to the original in CPU14, 1 MiB,
file state, the DOS log and the environment log:

- `budget-retries-0`: `0775/02B7`;
- `budget-disk-prompts-1`: `0775/02AA`;
- `budget-seams-3`: the fourth leaf, DECODE at `0935`.

The audit fails if LIMIT appears anywhere other than the budget cases, or if a budget case does not stop LIMIT.

**Mutation:** the seam budget off by one is caught in 152 s at `budget-seams-3` (`LIMIT at 0941, original 0935`). The
source was restored byte-identical.

## Reproduction

`CHECKER_ROWS = ('RES_0802_0716',)`: a second checker for the 0716 row, beside `res0802_0716_cpu_audit.py`.

    python3 tools/overlay/res0802_0716_composed_audit.py --compile-only
    python3 tools/overlay/res0802_0716_composed_audit.py --unicorn --output resources/0802-0716-composed/final.json
