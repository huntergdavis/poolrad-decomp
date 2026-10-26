# Media-host checker execution gate

CI's checker-build gate proves compile, load and symbol binding. Before merging
a recovered-C change, a media host must also run the affected registered
checkers' actual differential contracts. This is a written pre-merge rule,
not a CI job: original game media remains under ignored `resources/`.

For a body computing arithmetic FLAGS, final-register equality alone does not
verify intermediate flag writes. New or changed audits must observe each claimed
producer before its flags are overwritten and require a wrong-arithmetic-FLAGS
mutation to fail at that observation. Use the shared
[intermediate flag contract](INTERMEDIATE_FLAG_OBSERVATIONS.md) and state the
declared sites and any unwitnessed producers. This requirement does not mean the
existing checker catalogue has already adopted or passed the new control.

Run from a branch that includes freshly fetched `origin/main`:

```sh
python3 tools/overlay/check_checker_builds.py --execute OVL_0038_0400 --output resources/checker-execution.json
python3 tools/overlay/check_checker_builds.py --all-touching src/recovered/resident_00ba_party_count_23db_cpu.c --output resources/shared-source-execution.json
```

The gate submits each checker through the site's `poolrad-heavy.sh`, installed
next to the checkout, and waits for it before submitting another. Do not wrap
this orchestrating command in another heavy-job slot. Existing build-only
commands, including CI's `--all`, retain their behavior. `--execute --all`
explicitly runs all registered checker entry points and can take substantial
time. Configure the same pinned Python dependencies used by the audits.

Exit status is 0 when all selected checker contracts agree, 1 when a checker
fails (the affected row IDs are printed), and 4 when execution cannot be
established: stale branch, unknown selection, missing profile/media/dependency,
missing wrapper, timeout, build-only output or empty successful output. A
cannot-run result is never agreement. The receipt retains each command, row
IDs, checker hash, stdout, stderr and result, together with gate/profile and
media hashes and base/head commits. It is updated after each checker, so a
failed run retains earlier results. A cannot-run checker stops further
submission; ordinary differential failures remain recorded while the remaining
selected checkers run. Only a process group created by this invocation is
terminated on timeout.

`checker_execution_profiles.json` is the explicit per-checker argument table.
It contains 222 current entry points: `--unicorn` where that is the differential
mode, otherwise the checker's default differential/native mode. No profile uses
`--compile-only`, `--quick` or `--allow-incomplete`. New registrations require a
profile; an absent profile refuses execution instead of silently skipping the
checker. A profile must be reviewed against its checker's CLI when that changes.
The runner trusts the reviewed checker's exit contract; it does not turn a
broken or weakened checker into proof of correctness.

## Shared-source selection

There is no handwritten source-to-row table. The selector derives inputs from
compiler argument expressions, declared `SOURCES` lists, composed lists/sets
and local helper imports. CMakeLists.txt supplies the recovered-C source
universe. Recovered-source globs select that universe. An opaque build also
selects the CMake universe and records the reason: it never becomes an empty
set that could hide an affected checker. Local imports are followed
conservatively, including cycles, so unused helper imports may select extra
checkers. This is a conservative registered-checker map, not an exact dynamic
call graph or coverage claim for unregistered legacy scripts.

Inspect the selection before launching a potentially large run:

```sh
python3 tools/overlay/check_checker_builds.py --all-touching src/recovered/overlay_u38_handler_0400_cpu.c --selection-only
```

This mode prints source evidence, fallback reasons, row IDs and arguments;
its successful exit means selection only, not execution or agreement. A path
outside the current recovered-C CMake universe is refused. At the t79 base,
the 0400 source selects 78 entry points, 77 carrying a conservative fallback
or whole-library reason. This breadth is explicit; it must not be described as
78 independently proven direct dependencies.

## Validation and limits

All 38 contract tests pass. Synthetic controls cover unknown/missing execution prerequisites,
empty/build-only success, differential failure with named rows, serial order,
retained receipts, composed compiler inputs and cyclic helper imports. The
real media-host probe of OVL_0038_0400 ran the current full differential:
197 returns and six resident budget stops, CPU14 and 1 MiB RAM, no flag masks,
59/59 original starts and 8/8 outcomes on the returning set. The stale-main
precondition was also observed refusing a run before branch refresh.

This change does not assert that all 222 configured checkers were executed or
that their existing exclusions, masks and bounds disappeared. Those remain
the individual checker/evidence document's contract. No strict, identity,
replacement or natural-game-reach credit follows from this infrastructure.
