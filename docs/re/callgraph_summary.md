# Fixup-aware call graph summary (candidate call sites)

Sites come from byte scans for E8/9A without instruction-boundary validation. `far_fixup_confirmed` sites carry a linker relocation on their segment word and are real far-call or far-pointer operands; `near_candidate` counts are upper bounds.

| measure | count |
| --- | ---: |
| far_fixup_confirmed | 4009 |
| far_overlay | 1161 |
| far_overlay_exact_prologue | 1157 |
| far_resident | 1061 |
| far_runtime | 1787 |
| far_unrelocated_candidate | 28 |
| near_candidate | 1045 |
| near_candidate_exact_prologue | 966 |
| near_candidate_out_of_unit | 259 |

Top far-call segments (fixup-confirmed sites only): 0xaf8: 1787, 0xba: 515, 0xb0: 311, 0x709: 262, 0x2b: 208, 0x3e0: 171, 0x3c5: 119, 0x464: 110, 0x85: 53, 0x3f1: 50

| fan-in rank | unit | function rel | candidate callers |
| ---: | --- | --- | ---: |
| 1 | 0x9e | 0x160 | 140 |
| 2 | 0x9e | 0xde3 | 127 |
| 3 | 0x25 | 0x0 | 55 |
| 4 | 0x25 | 0x136 | 52 |
| 5 | 0x3c6 | 0xbe7 | 47 |
| 6 | 0x3c6 | 0xc0f | 47 |
| 7 | 0x9e | 0xe52 | 39 |
| 8 | 0x3b0 | 0x3bd | 37 |
| 9 | 0x9e | 0x26 | 36 |
| 10 | 0x25 | 0x6af | 35 |
| 11 | 0x25 | 0xc77 | 35 |
| 12 | 0x6c | 0x56c | 33 |

| src unit | dst unit | fixup-confirmed far edges |
| --- | --- | ---: |
| 0x1c | 0x25 | 205 |
| 0x38 | 0x9e | 89 |
| 0x5e | 0x3c6 | 72 |
| 0x86 | 0x9e | 69 |
| 0x6c | 0x3b0 | 33 |
| 0x38 | 0x3c6 | 24 |
| 0x5e | 0x9e | 23 |
| 0x86 | 0x3c6 | 23 |
| 0x31 | 0x5e | 21 |
| 0x33 | 0x9e | 19 |
| 0x6c | 0x6f | 19 |
| 0x2f | 0x3c6 | 18 |
| 0x31 | 0x9e | 17 |
| 0x6a | 0x3b0 | 16 |
| 0x6c | 0x9e | 16 |
| 0x7f | 0x9e | 16 |
| 0x2f | 0x5e | 15 |
| 0x1c | 0x9e | 14 |
| 0x9e | 0x3c6 | 14 |
| 0x1e | 0x9e | 13 |
