# Fixup-aware call graph summary (candidate call sites)

Sites come from byte scans for E8/9A without instruction-boundary validation. `far_fixup_confirmed` sites carry a linker relocation on their segment word and are real far-call or far-pointer operands; `near_candidate` counts are upper bounds.

| measure | count |
| --- | ---: |
| far_fixup_confirmed | 4009 |
| far_overlay | 202 |
| far_overlay_exact_prologue | 200 |
| far_resident | 1850 |
| far_runtime | 1787 |
| far_stub_misaligned | 170 |
| far_unrelocated_candidate | 28 |
| near_candidate | 1045 |
| near_candidate_exact_prologue | 966 |
| near_candidate_out_of_unit | 259 |

Top far-call segments (fixup-confirmed sites only): 0xaf8: 1787, 0xba: 515, 0xb0: 311, 0x709: 262, 0x2b: 208, 0x3e0: 171, 0x3c5: 119, 0x464: 110, 0x85: 53, 0x3f1: 50

| fan-in rank | unit | function rel | candidate callers |
| ---: | --- | --- | ---: |
| 1 | 0x9e | 0x160 | 140 |
| 2 | 0x25 | 0x1cc3 | 52 |
| 3 | 0x25 | 0x1df0 | 42 |
| 4 | 0x6c | 0x56c | 33 |
| 5 | 0x86 | 0x8c3 | 33 |
| 6 | 0x3c6 | 0xe3 | 26 |
| 7 | 0x33 | 0x221 | 22 |
| 8 | 0x3c6 | 0x0 | 21 |
| 9 | 0x3bd | 0x47b | 21 |
| 10 | 0x38 | 0x0 | 20 |
| 11 | 0x3c6 | 0x12fb | 19 |
| 12 | 0x3bd | 0x211 | 18 |

| src unit | dst unit | fixup-confirmed far edges |
| --- | --- | ---: |
| 0x1c | 0x25 | 99 |
| 0x6c | 0x3c6 | 24 |
| 0x1c | 0x3c6 | 10 |
| 0x6a | 0x3c6 | 10 |
| 0x77 | 0x3c6 | 9 |
| 0x1e | 0x3c6 | 5 |
| 0x25 | 0x3c6 | 5 |
| 0x6f | 0x3c6 | 5 |
| 0x86 | 0x3c6 | 5 |
| 0x1c | 0x2f | 4 |
| 0x2f | 0x3c6 | 4 |
| 0x1c | 0x6f | 3 |
| 0x25 | 0x3d2 | 2 |
| 0x36 | 0x3c6 | 2 |
| 0x5e | 0x3c6 | 2 |
| 0x6c | 0x7f | 2 |
| 0x7d | 0x3c6 | 2 |
| 0x7f | 0x3c6 | 2 |
| 0x1c | 0x7f | 1 |
| 0x20 | 0x3c6 | 1 |
