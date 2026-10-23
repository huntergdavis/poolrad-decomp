# C tests: the NDEBUG gate and the per-credit back-catalogue (2026-10-23)

**CLASS CLOSED (VP-accepted).** All 281 of 281 credited bodies are accounted for: **222** have C tests that pass with asserts on; **1** (OVL_007F_0FD2) has a failing C test that is a stale expectation, while its credit rests on a registered checker; **58** have no C test. Credits that rest on composed Unicorn or DOSBox-X differentials are **not in question**: those harnesses compare full state and do not use `assert()`.

58 credits have no C test; each rests on its differential (13 registered checkers, 25 audits), which is the primary evidence and does not use `assert()`, or, for 20, on a cited shared tool.

**The C suite was green and blind.** A Release build defines `NDEBUG`, which compiles every `assert()` away. With the
assertions gone, the assert-based tests could not fail; with them on, current main (`5813276e`) passes **612 of 613** C
tests, and the one real failure predates tonight. CI never builds or runs the C tests, so nothing caught it.

## The gate (this change)
- Every executable built from `tests/` gets `-UNDEBUG`, after `-DNDEBUG` on the command line, so it wins
  (`CMakeLists.txt`, end of file). Verified in a Release build: the compile line ends `-DNDEBUG ... -UNDEBUG`, and the
  previously hidden failure below aborts as it should.
- `tests/recovered/test_assert_canary.c` is a **refusal**, not a warning: it `#error`s if `NDEBUG` reaches it, and at
  run time it checks that `assert(0)` really aborts (in a forked child).
- `tools/overlay/count_assert_tests.py` derives the count instead of quoting it: **221 of 659** C test files under
  `tests/` include `<assert.h>` and call `assert(` outside comments. My earlier quick figure, 211 of 578, counted only
  `tests/recovered` and the include alone. `test_count_assert_tests.py` (CI) pins the counting rule and checks that the
  CMake gate and the canary are in place.

## Retractions (what was reported, from which build, and what is true with asserts on)
| reported | build | true now (asserts on, `5813276e`) | why the difference |
|---|---|---|---|
| full suite 609/613 | Release (`23acc5bf`) | **612/613** | NDEBUG compiled out the asserts of the assert-based tests; the 600 s cap cut off two slow tests |
| `u3bd_in_grid_031f` aborts (not #1433) | Release (`1b3943e2`, `9389f3e8`) | **passes**, with asserts on and in Release+gate | it really failed at those older commits and has been fixed on main since; not an NDEBUG effect |
| `u6f_02da_decision` times out | Release, 600 s cap | **passes** | slow, not failing: within ctest's default 1500 s |
| `u6f_1937_precall` times out | Release, 600 s cap | **passes** | same |
| `u9e_entry8_fullstate_prefix_0de3` passes | Release | **failed** on the AF fix until its expectation moved | its `assert()` was compiled out; fixed in #1465 |
| `u7f_roll_composed_0fd2` passes | Release | **fails** (below) | its `assert()` was compiled out |

## The one real failure
`poolrad_overlay_u7f_roll_composed_0fd2_tests`, `test_overlay_u7f_roll_composed_0fd2.c:34`: the test's callback asserts
entry at `CS:IP 00B0:0048`. Commit `17498aef` (2026-10-21, "OVL_007F_0FDE: enter the roll at the live segment word
(CS:0FE1), not 00B0") deliberately changed the call adapter, and the test written on 2026-10-15 was not updated. The
failure has been hidden since then because the suite ran with asserts compiled out. **Verdict: test defect** (the
expectation is stale; the model change was intended). **Owner: par-c** (PM i21860). It also fails identically before
#1433 (`9389f3e8`) and at the #1433 merge.

## Per-credit back-catalogue (asserts-on run as the arbiter)
Each of the 281 credited bodies (`strict_full_c_addresses.csv`) mapped to its C tests: tests whose source name
carries the body's unit and offset, plus the `ctest` names the evidence table (`credit_evidence_kind.csv`) cites for
`unit-test` credits.
- **(a) 222**: has C tests, all pass with asserts on. Claims that used them are now verified.
- **(b) 1**: a C test fails with asserts on: **OVL_007F_0FD2** (`u7f_roll_composed_0fd2`, the test defect
  above). Its credit rests on the registered checker `u7f_roll_class_0fd2_cpu_audit.py`, not on this C test, so the
  credit is not withdrawn; the test needs its expectation updated (par-c).
- **(c) 58**: no C test in the evidence (kinds: {'cites-tool': 20, 'audit': 25, 'checker': 13}).

**Credits resting on composed Unicorn or DOSBox-X differentials are NOT in question.** Those differentials are their own
Python harnesses, and they compare full state without `assert()`. Only a claim that leaned on a C test alone could have
been hollow, and every such test in (a) now passes with asserts live.

The C suite was green and blind; it is now green and watching. Of the 58 credits without a C test, 20 rest only on a cited shared tool, the weakest evidence class in the corpus, and are being upgraded. (owner: wR:p1)
