# U19 selected-PC overflow and return negative controls

This is a synthetic, source-only gate for the merged opt-in U19 candidate
recorder. It does not prove native instruction coverage, a guest writer,
four-plane change, page selection, RGB, C parity, or a complete frame. A
bounded `deja "Poolrad U19 selected PC recorder candidate 2305 overflow early
return duplicate return negative controls"` search found no indexed match.
The tests reuse the merged [2,304-candidate capacity
gate](U19_ROW14_CANDIDATE_COVERAGE_GATE.md) and
[version-1 recorder](U19_ROW14_OPTIN_RECORDER_FEASIBILITY.md).

The four candidate PCs are `0F2D:+0412/+0438/+0445/+0451`. On the
conditional fixed-row C route, the first possible `A000:118F` selected
candidate is 897 or 898 according to page order. The immediate successor
does not increment the candidate count. The raw frame cap is 2,307:
caller, 2,304 candidate frames, one selected successor, and first U19
`+02DD` return.

The separate synthetic controls exercise:

| Supplied sequence | Required fail-closed result |
| --- | --- |
| Return after 0, 896, 897, or 2,303 candidates | `STOP_INCOMPLETE_ROUTE`; no auditable selected event. The 897 case has already supplied its selected successor. |
| All 2,304 candidates and successor, with no return | `STOP_TRUNCATED`; no selected event. |
| Full return followed by another return or candidate | `STOP_AFTER_END`; final transcript cannot audit as selection-bearing. |
| Accepted selected prefix followed by a return supplied after terminal | `STOP_AFTER_END`; prior prefix cannot remain an accepted final transcript. |
| Candidate 2,305 with either page order | `STOP_CANDIDATE_OVERFLOW` and no selected event; the overflow frame is not appended. A forged lower count is rejected. |

These tests intentionally check the terminal transcript as well as the
immediate `push` result. In particular, a later invalid input cannot leave
behind an accepted selected-event manifest. The two possible first-target
ordinals remain conditional source values, not observed native visits.

Reproduce with synthetic inputs only:

```sh
python3 -m unittest \
  tests.replay.test_cold_start_codeword_row14_selected_pc_negative_controls \
  tests.replay.test_cold_start_codeword_row14_selected_pc_recorder
```

Original game media and any private capture stay under ignored
`resources/`. The native host slot remains held.
