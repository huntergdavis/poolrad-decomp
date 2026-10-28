# Free-model share of credited work (t40)

Measured 2026-10-27 from the free-run verdict corpus and the credited-body
registry on main `57764669`. Target: 50% of finished work began from a free
draft. This is a measurement; it grants no credit and makes no whole-game claim.

## Result

- **0 free drafts used out of 0 post-program credit opportunities.** One body
  has been credited since the free program began at 2026-10-24 19:45:31 PDT:
  `OVL_0038_0400` (t83). The free pool never probed it. The pipeline has not yet
  put a free draft in front of a post-program body.
- The 12 probed-and-credited bodies were **all credited before the program**
  (`33f2444a`, 2026-10-24 14:32:37 -0700). They are ineligible by definition,
  not a performance sample.
- Of the **62 committed verdict files**, **0** are whole-body byte-identical
  under the checker. A verdict's "16 bytes" is the length of the byte run the
  checker matched, not the body length; it does not establish a whole body.
- The capability question is therefore **unmeasured here**; the 50% target is
  not assessed, rather than reported as a measured zero.

## Evidence now in the tree

- `docs/re/free_runs/<body>/verdict.txt`: 62 verdict files, copied verbatim
  from `/home/hunter/workspace/vp-of-agentry/data/free_runs` (2026-10-27).
- `docs/re/free_runs/MANIFEST.tsv`: body, model, verdict line and SHA-256 of
  every copied file.
- `tools/free/free_verify_citations.py`
  (`d7c6b9005f81f0aec00f98023b65dceae59c46040c80db4f3b315732318061f4`) and
  `tools/free/free_body_check.py`
  (`7ae74e7abd581e226453b185d059cf35e90934af3ae4802efa6b6e5839745831`),
  copied from `/home/hunter/workspace/vp-of-agentry-wt-themes2/tools/`
  (2026-10-25). The former emits `VERDICT: FABRICATED CITATIONS`; the latter
  emits `BYTES MATCH` / `BYTES DIFFER` over the quoted dump.
- Corpus composition: 59 `nemotron-3-ultra:free`, 3
  `gemini-4-argon-contributor` files. Three of the 65 source directories had no
  `verdict.txt`; the laguna 5 are not in this corpus (scratch gone).

## Credit-side method (verified, unchanged)

- Credited set: `docs/re/strict_full_c_addresses.csv` on main (290 rows). All
  12 bodies in the table are present there (12/12).
- All 12 were in the CSV at `33f2444a`, the last commit before the program
  start; their final C predates the free drafts.
- The only post-program credit-grant commit is `62a7a9d2`
  (2026-10-25 05:01:56 -0700, `OVL_0038_0400`, t83). It is not a free-probed
  body.

## Per-body pre-program credits (ineligible by definition)

| Body | Free verdict line | Final C began from a free draft |
| --- | --- | --- |
| `OVL_0025_0000` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 29907 | no (pre-program) |
| `OVL_0025_0136` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 30217 | no (pre-program) |
| `OVL_0025_01B6` | FABRICATED CITATIONS; BYTES DIFFER at +0 (offset 30345) | no (pre-program) |
| `OVL_0025_0482` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 31061 | no (pre-program) |
| `OVL_0025_06D3` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 31654 | no (pre-program) |
| `OVL_0025_06FB` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 31694 | no (pre-program) |
| `OVL_0025_0756` | FABRICATED CITATIONS; no bytes result | no (pre-program) |
| `OVL_0025_0E0C` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 33503 | no (pre-program) |
| `OVL_0038_01C3` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 60958 | no (pre-program) |
| `OVL_0038_24D9` | FABRICATED CITATIONS; no bytes result | no (pre-program) |
| `OVL_0038_268A` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 70373 | no (pre-program) |
| `OVL_0038_2E37` | FABRICATED CITATIONS; BYTES MATCH 16 bytes at 72338 | no (pre-program) |

## Limits / not verified

- The "22 documented attempts (nemotron 17, laguna 5)" and the "7/20"
  scouting claim come from the t40/t105 task records; they are not re-derived
  from the 62 verdict files (59 nemotron + 3 gemini-argon).
- The laguna verdicts are absent from the corpus (scratch gone).
- The checker scores a quoted dump; it does not compile a C draft, so "compiles"
  is not tested here.
- No free model was rerun; no whole-game, parity or credit claim.
