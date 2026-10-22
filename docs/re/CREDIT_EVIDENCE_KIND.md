# Credit evidence kind: what each strict credit's evidence lets a reader re-run

**Decision by the credit authority (par-e), VP i20849/i20915, 2026-10-21; revised 2026-10-22 (VP i21177).** Every strict credit carries one label in
[credit_evidence_kind.csv](credit_evidence_kind.csv), from a closed set. The label states what a reader can actually
run for that credit. This **un-credits nothing**: `strict_full_c_addresses.csv` and the accounting pin are untouched.
It makes the corpus say which kind of evidence each credit rests on.

## The labels (closed set)

| Label | Meaning | VP's category (i20915) |
| --- | --- | --- |
| `checker` | The id is in some `CHECKER_ROWS` that `check_checker_builds` discovers (under `tools/overlay` **and** `tests/recovered`), and CI proves that checker builds on current main. | registered checker |
| `audit` | The evidence document cites an existing script whose **source names this body**, but the script is not registered, so it is NOT machine-proven to run on main. | audit that needs `--compile-only` |
| `unit-test` | The document cites a `ctest -R` command for a test registered in `CMakeLists.txt`. These are hand-derived synthetic checks of the C, not an original-byte differential. | doc that names a command (weaker form) |
| `cites-tool` | The document cites only scripts that do not name this body (`callgraph.py`, `ecl_walk.py`, `resident_loader_map.py`, ledger generators). | doc that names a command |
| `prose-only` | The document cites nothing runnable for this body. (Called `document` before 2026-10-22.) | prose only |
| `stale` | Every script the document cites is missing, or a cited `ctest` name is not registered. | (none today) |
| `missing-doc` | The evidence document itself is missing. | (none today) |

## How the table is made, and why it cannot drift

- **Generated, never hand-edited:** `tools/overlay/credit_evidence_kind.py --write` derives every label from the tree:
  CI's own checker registry (`check_checker_builds.discover`), each credit's evidence document, and the `add_test`
  names in `CMakeLists.txt`. `--check` exits 1 when the committed table differs from a fresh run, and CI runs it.
- **`checker` is pinned:** `tools/overlay/test_credit_evidence_kind.py` requires exactly one row per strict credit and
  labels from the closed set, with no `stale` or `missing-doc` rows. It also requires the `checker` set to EQUAL the
  credits in CI's registry, so `checker` is not an opinion, and the committed table to equal a fresh classification. A
  synthetic tree exercises every label, including a `tests/recovered` registration and an unregistered `ctest` name.
- **A label follows the document, not a file that merely exists.** Before 2026-10-22, `unit-test` meant that a C test
  file named the body, whether or not the document said how to run it. It now requires the command in the document.
- **"Names the body"** means the id appears, or the offset appears together with its unit or segment, in the script's
  file name or source. Matching on the offset alone was rejected: `0000` would match unrelated `*_0000_*` files.

## Counts (280 credits)

| Label | `9663d0c8` (#1411) | `ee07bd68` + this change |
| --- | ---: | ---: |
| `checker` | 98 | 104 |
| `audit` | 115 | 117 |
| `unit-test` | 11 | 38 |
| `cites-tool` | 21 | 21 |
| `prose-only` (was `document`) | 35 | 0 |
| `stale` / `missing-doc` | 0 | 0 |

The two columns use different definitions of `unit-test` (see above), so the 11 and the 38 are not the same kind of
count.

**Correction to #1411.** Its registry scanned `tools/overlay` only, but CI's `check_checker_builds` also registers
`CHECKER_ROWS` from `tests/recovered`. That left `OVL_0038_12B2` labelled `audit` and `OVL_0038_25BC` labelled
`unit-test` when both are `checker`. The classifier now takes CI's own `discover()`, and the test pins both rows.
#1411's test compared the classifier with itself, so it could not have caught this.

**What these counts do and do not say.**
- **The table is static.** CI proves only that the `checker` rows' checkers build; it does not run them.
- **Most `audit` rows were not run by me.** Each cites an existing script that names the body. "Cited and present" is
  not "reproduces". The three that this change added (019C, 0918, 0E2E) were run; see below.
- **`unit-test` is weaker than `audit`.** These are hand-derived synthetic checks of the C, with no DOS bytes. CI
  does not run `ctest`.

## The 46 prose-only credits (VP i21177), fixed 2026-10-22

ds4 counted 46 credits whose documents cite no command at all. On `ee07bd68` this table had them as 45 `prose-only`
plus `OVL_0038_25BC`. Every one now has a per-body reproduction command in its evidence document, under a
`## Reproduce` heading. I ran each command on `ee07bd68` before writing it down. None is labelled prose-only, because
every body had a registered `ctest` target that calls its credited C symbol. I checked that call body by body: for the
22 unit 0x38 entries and the four unit 0x3C6 getters, a `pr_*_entryN(` call in the named test.

| Outcome | Credits | How |
| --- | ---: | --- |
| `checker` (new) | 4 | `OVL_0033_01AD`, `OVL_0033_0377`, `OVL_0033_0E9E`, `OVL_0033_1434`. Their sibling's u33 audit already ran the credited C as a direct original-byte target. I ran each audit with `--unicorn` (all passed), added the id to that audit's `CHECKER_ROWS`, and `check_checker_builds` passed for the four. |
| `checker` (relabel) | 1 | `OVL_0038_25BC`, which was always registered (see the correction above). Its parity script passed 720 cases. |
| `audit` (new) | 3 | `OVL_006A_019C` (u6A audit, which has no `--compile-only` and so cannot be registered: 507 cases), `OVL_0038_0918` (96 cases), `OVL_009E_0E2E` (96 + 16 cases). |
| `unit-test` | 38 | `ctest -R` command only. All 24 test executables passed. |

**A broken checker found on the way.** `tests/recovered/check_overlay_u9e_entry9_roll_latch_0e2e.py` did not run on
`ee07bd68`: its library failed to load with `undefined symbol: pr_u9e_entry8_fullstate_compose_0de3`. 08de57fb had added
`pr_u9e_roll_latch_0e2e_cpu` to the credited source file, and the script's `cc` list was never updated. This change adds
that function's eight-file source closure. The credited function is unchanged, and the script passes again.

**Still open.** The 38 `unit-test` credits have no original-byte differential. The 21 `cites-tool` credits are outside
the 46 and still cite only shared tooling.

## Credits other credits depend on: re-examine first

On `9663d0c8`, 56 credits (`cites-tool` + `document`) had no body-specific runnable evidence. An error in one that
other credited bodies compose would propagate, so those come first. Credited callers are counted from the call graph
(near and far edges into the body) on that base. The label column is today's.

| Credit | Label | Credited callers | All callers | Evidence document |
| --- | --- | ---: | ---: | --- |
| `OVL_0025_06AF` | cites-tool | 7 | 24 | `docs/re/FUNCTION_LEDGER.md` |
| `OVL_03C6_0C5F` | unit-test | 6 | 18 | `docs/re/U9E_0160.md` |
| `OVL_03BD_047B` | unit-test | 4 | 9 | `docs/re/U3BD_NIBBLE_ENTRY2.md` |
| `OVL_03C6_0C0F` | unit-test | 3 | 28 | `docs/re/U9E_0160.md` |
| `OVL_03C6_0BE7` | unit-test | 3 | 28 | `docs/re/U9E_0160.md` |
| `OVL_0025_06FB` | cites-tool | 3 | 4 | `docs/re/FUNCTION_LEDGER.md` |
| `OVL_03BD_0350` | unit-test | 2 | 4 | `docs/re/U3BD_FIELD_ENTRY3.md` |
| `OVL_0068_05AB` | unit-test | 2 | 2 | `docs/re/U68_RECORD_SCAN_05AB.md` |
| `RES_00BA_2637` | cites-tool | 1 | 23 | `docs/re/RESIDENT_CHARACTER_TARGET_RESET.md` |
| `RES_00BA_23B7` | cites-tool | 1 | 10 | `docs/re/RESIDENT_CHARACTER_FIELD_ZERO.md` |
| `RES_00BA_2E8B` | cites-tool | 1 | 6 | `docs/re/RESIDENT_CHARACTER_TARGET_SELECT.md` |
| `OVL_03BD_057A` | unit-test | 1 | 5 | `docs/re/U3BD_CELL_ENTRY4.md` |
| `OVL_03C6_0C37` | unit-test | 1 | 2 | `docs/re/U9E_0160.md` |
| `OVL_0068_05EE` | unit-test | 1 | 2 | `docs/re/U68_LIST_SEARCH_05EE.md` |
| `OVL_0038_2B63` | unit-test | 1 | 1 | `docs/re/OVERLAY_UNIT38_EFFECT_ENTRIES.md` |
| `OVL_0038_0C06` | cites-tool | 1 | 1 | `docs/re/FUNCTION_LEDGER.md` |
| `OVL_0025_06D3` | cites-tool | 1 | 1 | `docs/re/FUNCTION_LEDGER.md` |

17 of those 56 have credited callers, and 39 have none. Of the 17, the 11 that were `document` now have a command,
but only `unit-test` strength. The six `cites-tool` rows have nothing body-specific.

The proposed re-examination is the free-model policy's proven shape: an independent static re-derivation from the raw
bytes that reports AGREE or DISAGREE and names the bytes. The review lane accepts each result against the bytes, and any
DISAGREE gets a real audit before anything else.
