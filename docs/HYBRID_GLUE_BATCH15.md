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


## t72 non-author replacement receipt

Base `e88e1110488110add9337481633e5a840356886f`, after #1545 merged.
The Codex lane independently checked the retained route artifacts; this is
an artifact re-measure, not a new gameplay run. `launch.log` states
`PR_HYBRID bus mode: route`, `r1-mode.txt` says `dispatch`, and the seed file
and observed pin both identify `12345678`. The route summary reports success.

The four `observer.dispatch` summary counters give arrivals / C returns /
hand-backs of **1/1/0**, **1/1/0**, **1/1/0**, **24/24/0**, in the table's
order. Interruptions and timeouts are zero for all four. This observer logs
these route results in summary counters; the acceptance does not claim
individual route-return trace lines. Observer file SHA-256:
`5cd997e8a59d1bc6c172c68777fba08b74dbedfde33aa7785707d95782961aa6`.

Decoded RGB pixels from `hybrid-b15-r4-route/final.png` and both
`r4-original-a/final.png` and `r4-original-b/final.png` are byte-identical:
**0 / 786,432 differing pixels**, dimensions 1024 x 768, SHA-256
`1b7c8943aee2baadcbe3ee15249bc93f8f541c9f75792445d39e4b20a0e31bb3`.
All **33 observed post-pin RNG values** match both originals. The normalized
sequence (one hex seed per line, trailing newline) hashes to
`37cd6fc6df171ba2f2a9a86a332d57aefcfd3f36c562abbde1a7aee42b9c9ba2`.
The comparison skips observer comments and timing/instruction counters,
starts after the seed-pin marker, and compares every subsequent draw value.
The frame comparison decodes to RGB before comparing every pixel; PNG file
encoding is not used as the equality criterion.

This adds exactly these four bodies to `re/replaced_bodies.json`, moving
**REPLACED 121 -> 125**. Strict credit and identity accounting are unchanged.
The existing 121 evidence entries are preserved. This does not claim
full-state parity, unobserved draws inside routed C, or the unexercised 19DB
unloaded-slot STUB path. All original frames and logs remain under ignored
`resources/`; only counts, addresses and hashes are published.
