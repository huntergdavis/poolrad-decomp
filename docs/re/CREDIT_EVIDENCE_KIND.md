# Credit evidence kind: what each strict credit's evidence lets a reader re-run

**Decision by the credit authority (par-e), VP i20849/i20915, 2026-10-21.** Every strict credit carries one label in
[credit_evidence_kind.csv](credit_evidence_kind.csv), from a closed set. The label states what a reader can actually
run for that credit. This **un-credits nothing**: `strict_full_c_addresses.csv` and the accounting pin are untouched.
It makes the corpus say which kind of evidence each credit rests on.

## The labels (closed set)

| Label | Meaning | VP's category (i20915) |
| --- | --- | --- |
| `checker` | The id is in some audit's `CHECKER_ROWS`, and `check_checker_builds` (CI) proves it builds on current main. | registered checker |
| `audit` | The evidence document cites an existing script whose **source names this body**, but the script is not registered, so it is NOT machine-proven to run on main. | audit that needs `--compile-only` |
| `unit-test` | The document cites no script, but a C test or native script under `tests/recovered` names the body. These are hand-derived checks, not an original-byte differential. | doc that names a command (weaker form) |
| `cites-tool` | The document cites only scripts that do not name this body (`callgraph.py`, `ecl_walk.py`, `resident_loader_map.py`, ledger generators). | doc that names a command |
| `document` | Nothing runnable names the body. | prose only |
| `stale` | Every script the document cites is missing from the tree. | (none today) |
| `missing-doc` | The evidence document itself is missing. | (none today) |

## How the table is made, and why it cannot drift

- **Generated, never hand-edited:** `tools/overlay/credit_evidence_kind.py --write` derives every label from the tree:
  the `CHECKER_ROWS` tuples, each credit's evidence document, and `tests/recovered`. `--check` exits 1 when the
  committed table differs from a fresh run.
- **`checker` is pinned:** `tools/overlay/test_credit_evidence_kind.py` requires exactly one row per strict credit and
  labels from the closed set, with no `stale` or `missing-doc` rows. It also requires the `checker` set to EQUAL the
  registered credits, so `checker` is not an opinion, and the committed table to equal a fresh classification. A
  synthetic tree exercises every label.
- **"Names the body"** means the id appears, or the offset appears together with its unit or segment, in the script's
  file name or source. Matching on the offset alone was rejected: `0000` would match unrelated `*_0000_*` files.

## Counts on main `9663d0c8` (280 credits)

| Label | Count |
| --- | ---: |
| `checker` | 98 |
| `audit` | 115 |
| `unit-test` | 11 |
| `cites-tool` | 21 |
| `document` | 35 |
| `stale` / `missing-doc` | 0 |

**What these counts do and do not say.**
- **The table is static.** Only the 98 `checker` rows are machine-proven to build on current main.
- **The 115 `audit` rows** cite an existing script that names the body, but I did **not** run them. "Cited and present"
  is not "reproduces". Registering an `audit` row (adding `--compile-only` and `CHECKER_ROWS`, then proving it with
  `check_checker_builds`) is the step that turns it into `checker`, and the table will follow automatically.

**Reconciliation with ds4's earlier static classification** (i20846/i20852, over the 100 credits that then had no audit
file of their own):
- **ds4's 46 "no command"** = this table's `document` 35 + `unit-test` 11. The 11 are split out because a C test that
  names the body is runnable evidence, just weaker.
- **ds4's 54 "cites a command"** fall here into `audit` (cluster audits whose source names the body, such as
  `u38_helper_cluster_audit.py`) or `cites-tool` (21).
- **The `checker` count has risen since** (46 then, 98 now) as registration batches landed.

## Prose-only credits that other credits depend on: re-examine, not just label

The 56 `cites-tool` + `document` credits have no body-specific runnable evidence. An error in one that other credited
bodies compose would propagate, so those come first. Credited callers are counted from the current call graph (near
and far edges into the body).

| Credit | Label | Credited callers | All callers | Evidence document |
| --- | --- | ---: | ---: | --- |
| `OVL_0025_06AF` | cites-tool | 7 | 24 | `docs/re/FUNCTION_LEDGER.md` |
| `OVL_03C6_0C5F` | document | 6 | 18 | `docs/re/U9E_0160.md` |
| `OVL_03BD_047B` | document | 4 | 9 | `docs/re/U3BD_NIBBLE_ENTRY2.md` |
| `OVL_03C6_0C0F` | document | 3 | 28 | `docs/re/U9E_0160.md` |
| `OVL_03C6_0BE7` | document | 3 | 28 | `docs/re/U9E_0160.md` |
| `OVL_0025_06FB` | cites-tool | 3 | 4 | `docs/re/FUNCTION_LEDGER.md` |
| `OVL_03BD_0350` | document | 2 | 4 | `docs/re/U3BD_FIELD_ENTRY3.md` |
| `OVL_0068_05AB` | document | 2 | 2 | `docs/re/U68_RECORD_SCAN_05AB.md` |
| `RES_00BA_2637` | cites-tool | 1 | 23 | `docs/re/RESIDENT_CHARACTER_TARGET_RESET.md` |
| `RES_00BA_23B7` | cites-tool | 1 | 10 | `docs/re/RESIDENT_CHARACTER_FIELD_ZERO.md` |
| `RES_00BA_2E8B` | cites-tool | 1 | 6 | `docs/re/RESIDENT_CHARACTER_TARGET_SELECT.md` |
| `OVL_03BD_057A` | document | 1 | 5 | `docs/re/U3BD_CELL_ENTRY4.md` |
| `OVL_03C6_0C37` | document | 1 | 2 | `docs/re/U9E_0160.md` |
| `OVL_0068_05EE` | document | 1 | 2 | `docs/re/U68_LIST_SEARCH_05EE.md` |
| `OVL_0038_2B63` | document | 1 | 1 | `docs/re/OVERLAY_UNIT38_EFFECT_ENTRIES.md` |
| `OVL_0038_0C06` | cites-tool | 1 | 1 | `docs/re/FUNCTION_LEDGER.md` |
| `OVL_0025_06D3` | cites-tool | 1 | 1 | `docs/re/FUNCTION_LEDGER.md` |

17 of the 56 prose-only credits have credited callers; 39 have none.

The proposed re-examination is the free-model policy's proven shape: an independent static re-derivation from the raw
bytes that reports AGREE or DISAGREE and names the bytes. The review lane accepts each result against the bytes, and any
DISAGREE gets a real audit before anything else.
