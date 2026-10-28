# Free-model share of credited work (t40)

Measured 2026-10-26 from the free-run verdicts and the credited-body registry on
main `57764669`. Target: 50% of finished work began from a free draft. This is a
measurement; it grants no credit and makes no whole-game claim.

## Result

- **0 of 12** credited bodies that the free pool probed began their final C from
  a free draft (**0%**).
- **0 of 1** body credited since the free program began
  (`OVL_0038_0400`, the t83 grant) began from a free draft.
- **0 whole-body usable free drafts** in the 65 verdict directories present
  (the documented trials are 22 attempts: nemotron 17, laguna 5). The 7/20
  "byte-identical" scouting claims are prefix matches, not whole-body C.
- The 50% target is not met.

## Method

- Free verdicts: `data/free_runs/<body>/verdict.txt` (65 body directories).
- Credited set: `docs/re/strict_full_c_addresses.csv` on main.
- Free program start: 2026-10-24 19:45:31 PDT.
- All 12 intersecting bodies were already in the credited CSV at `33f2444a`,
  the last commit before the program started; their final C therefore predates
  the free drafts.
- The only post-program credit-grant commit in the accounting history is
  `62a7a9d2` (`OVL_0038_0400`, t83). That body was not probed by the free pool;
  its final C is the codex lane's.

## Per-body evidence (12 probed and credited)

Every row is "no": the verdict has fabricated citations and, at best, a 16-byte
prefix match; the credit predates the free run.

| Body | Free verdict line | Final C began from a free draft |
| --- | --- | --- |
| `OVL_0025_0000` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 29907 | no |
| `OVL_0025_0136` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 30217 | no |
| `OVL_0025_01B6` | FABRICATED CITATIONS; BYTES DIFFER at +0 (offset 30345) | no |
| `OVL_0025_0482` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 31061 | no |
| `OVL_0025_06D3` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 31654 | no |
| `OVL_0025_06FB` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 31694 | no |
| `OVL_0025_0756` | FABRICATED CITATIONS; no bytes result | no |
| `OVL_0025_0E0C` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 33503 | no |
| `OVL_0038_01C3` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 60958 | no |
| `OVL_0038_24D9` | FABRICATED CITATIONS; no bytes result | no |
| `OVL_0038_268A` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 70373 | no |
| `OVL_0038_2E37` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 72338 | no |

## Limits / not verified

- The laguna verdict directory is no longer present under scratch; its 0/5
  result is taken from the task record, not re-read here.
- The other 53 probed bodies have no credited final C, so they have no share
  entry.
- Prefix matches do not establish a whole body; no free model was rerun.
- The 12-body denominator is "probed and credited", not every credited body in
  the game; the post-program grant denominator is 1.
