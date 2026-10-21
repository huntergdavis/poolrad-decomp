# Hybrid executor, stage 0: the observer log (VP i20556)

**Stage 0 is observation only.** No C ran in place of the game. The observer counted arrivals at the entry points of the 270
strict-credited bodies during one cold start of the audited v1.3 game, from the MZ entry to the main menu. Nothing was
substituted, no timing was changed, and no claim about our C running is made here. The design is in
[HYBRID_EXECUTOR_SCOPE.md](HYBRID_EXECUTOR_SCOPE.md).

## Result

**A cold start to the main menu reaches 12 of the 270 credited bodies.** The other 258 are not on that path. Per VP i20556,
that is a finding about which parts of the game our credits cover, not a failure of the harness.

These are the 12, in first-arrival order (run `hybrid-stage0-b`: 976 arrivals in 176,458,495 instructions):

| # | ID | Arrivals | First at instruction | Live CS | Model shape (its header) |
|---|---|---|---|---|---|
| 1 | `RES_0802_02EB` | 35 | 353,418 | `1026` | RAM + DOS file env (`pr_r0802_02eb_env`) |
| 2 | `RES_0709_05EB` | 4 | 468,963 | `0F2D` | `pr_bus` (private EGA) |
| 3 | `RES_0709_01DF` | 824 | 468,987 | `0F2D` | `pr_bus` |
| 4 | `RES_0709_0626` | 51 | 514,097 | `0F2D` | `pr_bus` |
| 5 | `RES_0899_0104` | 51 | 802,992 | `10BD` | RAM only |
| 6 | `OVL_0086_3413` | 1 | 16,586,564 | `22CC` | RAM only |
| 7 | `OVL_0038_31A2` | 1 | 16,586,852 | `27E8` | RAM only |
| 8 | `RES_0464_2595` | 2 | 37,498,187 | `0C88` | RAM + an extra argument (`system_cs`) |
| 9 | `RES_03F1_0137` | 4 | 58,742,747 | `0C15` | `pr_bus` |
| 10 | `RES_0899_0059` | 1 | 78,135,272 | `10BD` | RAM + keyboard env (`pr_r0899_key_env`) |
| 11 | `OVL_03B0_00CB` | 1 | 78,463,489 | `2116` | RAM + a callee status |
| 12 | `OVL_03B0_01DA` | 1 | 78,496,695 | `2116` | `pr_bus` |

- **The screen.** At 55 s it showed the main menu ("CHOOSE A FUNCTION": create, add, load, exit). The screenshot is in the
  ignored run directory and is not published.
- **Model shapes.** Of the 12, 5 are `pr_bus` models that draw into a private `pr_ega`. They are the first and the most
  frequent bodies reached: `RES_0709_01DF` alone has 824 of the 976 arrivals. So the scope's first "what breaks" item is
  confirmed by measurement. Three are RAM-only with no services. Two need a service env, one needs an extra argument, and
  one reports a callee status.
- **Not reached, by unit or segment** (258 in total): OVL_0038 69, RES_00BA 25, OVL_0025 19, OVL_0033 18, OVL_03B0 12,
  OVL_001C 12, OVL_03C6 11, OVL_007F 11, OVL_006A 8, OVL_0068 8, OVL_0086 7, OVL_005E 7, OVL_009E 6, OVL_006C 6, OVL_03C2 5,
  OVL_007D 5, OVL_03BD 4, OVL_009B 4, OVL_0077 4, RES_0709 3, OVL_03B4 3, OVL_0031 3, OVL_006F 2, OVL_002F 2, RES_0899 1,
  OVL_03CE 1, OVL_03B9 1, OVL_0020 1.
  - Unit 0038 (69 of its 70 credited bodies not reached) holds effects and roll code, per its docs
    (`docs/re/OVERLAY_UNIT38_EFFECT_ENTRIES.md`).
  - Segment 00BA (all 25 not reached) holds display and sidecar code (`docs/re/RES_00BA_DISPLAY_1693_CPU.md`,
    `docs/re/RESIDENT_SIDECAR_LOOKUP.md`).
  - Where in the game these bodies are reached is not measured. A stage 0 run along a gameplay route would answer it.

## Recorded with the result (VP i20594)

- **Coverage is a finding, not a failure.** 258 credited IDs are not on the cold-start path to the menu, and the biggest
  blocks are unit 0038 (69), segment 00BA (25) and unit 0025 (19). They run somewhere not yet measured. The VP's hypothesis,
  not yet tested, is the game overlays during play; stage (b) exists to answer it.
- **Arrival counts are kept in every report, not just reachability.** A single arrival (`OVL_03B0_00CB`, `OVL_03B0_01DA`)
  proves a body is on the path. 824 arrivals (`RES_0709_01DF`, 84% of this run's 976) prove it matters. A body that runs
  once and a body that runs a million times need different treatment, so future hybrid reports list both numbers per ID.

## The two unknowns, kept unknown (VP i20556)

- **How often `RES_0709_0626` completes before escaping: UNKNOWN.** Stage 0 runs only the original bytes, so a model escape
  cannot happen. The original 0626 ran 51 times. Whether our model would have reached RETURNED on those 51 entry states,
  or stopped at a callee, is only measurable by running the model, in stage (a) or shadow mode.
- **Which credited IDs are reached: measured above, for this path only.** It is one cold start, to the main menu, with Enter
  typed at 28 s. Other paths (a loaded game, character creation, travel, combat) are not covered.

## Checks that pin these numbers

- **The load base is consistent, and nothing in the observer enforces it.** Every resident match has CS minus its `RES_`
  segment equal to `0x0824`: 0709 at `0F2D`, 0802 at `1026`, 0899 at `10BD`, 0464 at `0C88`, 03F1 at `0C15`. That is the
  shift the native captures recorded (`docs/re/U9B_RUNTIME_DISPATCH_0000.md:28`). The observer matches by IP and bytes only,
  so a wrong match would show up as a different base.
- **Repeatability.** Run `hybrid-stage0-a` (32-byte signatures, 8 IDs not unique and therefore skipped) and run `-b`
  (all 270 unique) give identical per-ID arrival counts for every ID both could observe. The only difference is the newly
  observable `RES_03F1_0137` (4). First-arrival instructions drift after real-time waits (for example `RES_0464_2595`
  37,336,230 against 37,498,187). The counts do not.
- **Negative control.** Run `hybrid-stage0-negative` flips one unmasked signature byte of `RES_0709_01DF`. That ID drops
  from 824 to 0, and every other ID's count is unchanged (152 = 976 − 824). The matcher compares bytes; it is not matching
  on IP alone.
- **The registry signatures are unique.** Each signature is the first 32 bytes of the body, with relocated words masked. Each
  is checked to occur exactly once across all 34 overlay units' code plus the whole expanded resident image. 262 are unique
  at 32 bytes, 4 at 64, 2 at 128 and 2 at 256. All 270 are unique.

## What ran

- **The emulator.** DOSBox-X at the pinned source `5a9c7c53`, the heavy-debug configuration
  (`--enable-debug=heavy --enable-sdl2 --disable-dynamic-core`), forced to `core=normal`. The three locally modified files
  of the copy it came from were restored to pinned before the observer was added. The observer is
  [`tools/hybrid/dosbox-x-5a9c7c53-stage0-observer.patch`](../tools/hybrid/dosbox-x-5a9c7c53-stage0-observer.patch): a new
  header plus two lines in `core_normal.cpp`. It reads guest bytes and writes host files only, and it is inactive unless
  `PR_HYBRID_REGISTRY` is set. The build lives under ignored `resources/hybrid/`. Neither the system `/usr/bin/dosbox-x`
  nor the desk's deployed build was touched. PATH selected the observer for these runs only.
- **The run.** [`tools/hybrid/run_observer.sh`](../tools/hybrid/run_observer.sh) prepares an isolated run with
  `tools/oracle/prepare_run.sh`, which checks the v1.3 `START.EXE` and `GAME.OVR` hashes. It launches through
  `tools/oracle/launch.sh` (bwrap, no network) under Xvfb, types Enter at 28 s as `capture_menu.sh` does, takes a
  screenshot at 55 s, and stops at DOSBox-X's 60 s time limit. An outer `timeout` and the heavy-job wrapper bound each run.
  After each run, 0 `dosbox-x` processes remained.
- **The registry.** [`tools/hybrid/hybrid_registry.py`](../tools/hybrid/hybrid_registry.py) writes
  `resources/hybrid/registry.tsv`, which holds the signatures. They are game bytes, so the file stays ignored; only counts
  are published.

## What the registry cannot do yet

It identifies bodies and nothing more. It has **no call shape, CPU struct, env or budgets**, and a dispatch needs all four.

Its C-symbol column is a **candidate only**. Candidates come from the audits' `CHECKER_ROWS` plus the names a row's
evidence doc mentions that a header declares:

- 118 rows have exactly one candidate, 81 have several, and 71 have none.
- I checked the 12 reached IDs by hand: **11 are right and 1 is wrong.** `RES_0899_0104` got `pr_system_set_expand_0d12`
  from its evidence doc (`RES_0709_0768_PURE_CALLEES_CPU.md`, which names several functions). The right one is
  `pr_r0899_idle_0104_cpu`.
- So "one candidate" does not mean "the right symbol". Before stage (a), every hooked ID needs its symbol confirmed against
  its header.

## The scope's constraints, repeated (VP i20556)

Measured over `include/poolrad/recovered/` (from [HYBRID_EXECUTOR_SCOPE.md](HYBRID_EXECUTOR_SCOPE.md)):

- **31** prototypes take `pr_bus`, a private `pr_ega`, and need an adapter to the real VGA. 5 of the 12 reached bodies are
  among them.
- **237** prototypes are value-level C and cannot be dispatched at a guest address.
- **237** private CPU struct types each need their own copy-in and copy-out.
- **212** headers have callee, service or hook stops, so many bodies would run only partly as our C.

Together these say what a hybrid can and cannot claim.

## Not started

Stage (a), or any substitution. The VP asked to see this log first.
