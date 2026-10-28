# Free-run verdict corpus

62 `verdict.txt` files copied verbatim on 2026-10-27 from
`/home/hunter/workspace/vp-of-agentry/data/free_runs`. `MANIFEST.tsv` records
the body, model, first verdict line and SHA-256 of each file.

The checkers are copied from
`/home/hunter/workspace/vp-of-agentry-wt-themes2/tools/` (2026-10-25):

- `tools/free/free_verify_citations.py` — SHA-256
  `c9880a32fd5e3552685c372e0a74f189d553bf2f87ec9fe03a4773d1b088c116`
- `tools/free/free_body_check.py` — SHA-256
  `7ae74e7abd581e226453b185d059cf35e90934af3ae4802efa6b6e5839745831`

`free_body_check.py` reports the length of the byte run it matched ("16 bytes"),
not the body length. Three of the 65 source directories had no `verdict.txt`;
the laguna five are not in this corpus.
