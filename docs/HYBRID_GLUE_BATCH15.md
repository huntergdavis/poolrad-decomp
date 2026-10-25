# Hybrid glue, batch 15: par-d2's raw models for R4's four semantic-only bodies (PM i22989, i23084)

| Row | C entry | Kind | PR |
| --- | --- | --- | --- |
| `OVL_007F_0130` | `pr_u7f_div5_0130_cpu` | bus, RETF 2 | #1541 |
| `OVL_007F_0185` | `pr_u7f_div5_0185_cpu` | bus, RETF 2 | #1541 |
| `OVL_0025_19DB` | `pr_u25_move_19db_cpu` | bus; a STUB stop at an unloaded 3BD slot hands back, and the overlay manager loads it | #1542 |
| `OVL_0025_1815` | `pr_u25_compare_strings_1815_cpu` | raw; composes SYSTEM 0B25/16A8/0C10 | #1543 |

**Depends on** #1541, #1542 and #1543, which provide the sources, and **stacks on** #1546 (batches 12/13).

**Build:** `2760a9e8`, with 160 model entries and 171 dispatch rows. It was built from main plus those three PRs'
source and header files. **Runs:** `run_r1.sh` dispatch mode on R4, pinned at `12345678`, compared with codex's
all-original R4 pair (RGBA `008650a6…`, raw-RGB `1b7c8943…`). **These are test-harness runs.**

| Run | Result |
| --- | --- |
| `hybrid-b15-r4-v1` | non-equal 0; final frame and all 33 draws equal. 0130 1/1, 0185 1/1, 19DB 1/1, 1815 24/24 |
| `hybrid-b15-r4-route` | 11,044 routed returns; **0 px**; all 33 draws equal. Routed: 0130 1, 0185 1, 19DB 1, 1815 24 |

- **19DB** returned in both runs: the 3BD slot was loaded at its one arrival, so the STUB path was not exercised.
- **Hand-backs:** the only hand-back is `OVL_006F_1446`'s cold INT 3Fh stub.
