# t106 bounded free-model bake-off

## Result: the 2026-10-27 run is INVALID as a model score

Run dir:
`/home/hunter/workspace/vp-of-agentry-wt-themes2/data/bakeoff/20261027-180647`
(results.tsv SHA-256 `a5a6f9e4a15b9915afeb777edd38da707f59b2f10c47cfb397b22dd073965b0a`,
plus 10 `.md`/`.err` pairs and six byte-check receipts). Five candidate models,
two verified-C bodies (`OVL_0025_0136` at GAME.OVR 30217/128 B and
`OVL_0025_06AF` at 31618/36 B), 10 runs, parallel 3. The systemd free pool was
not restarted (`poolrad-free-pool` and `poolrad-free-trial` are inactive).

The wrapper labels are operational observations only. The four EMPTY runs never
invoked the byte checker (the subprocess stdout was empty after a 429), so their
labels are not decoding outcomes. The raw files show two tool/provider failures,
not a model result:

- **EMPTY (flint 2, xing 2): upstream 429.** The report `.md` is 0 bytes and
  `.err` carries the OpenRouter JSON: "temporarily rate-limited upstream"
  (providers Recursive and Phala).
- **OFFSET UNRESOLVED / DIFFERS (inkling 2, north 2, nemotron 2): broken
  media access, and in three reports also a genuinely wrong or missing offset.**
  All six non-empty reports state that they could not access the game media,
  and none produced a quoted dump. Five name a missing/broken
  `resources/DOS/POOLRAD/GAME.OVR` or symlink (per-report assertions, not
  independently proven here); north `0136` reports only that its tools/read
  operations failed and that the media "appears to be inaccessible", which is
  a report of inability to access, not proof of absence. The invocation used
  `POOLRAD_REPO=/tmp/opencode/t40-wt`, whose private `resources` link is not
  visible inside the free sandbox; that is the mechanism, established by the
  reviewer's sandbox reproduction rather than by the reports.

  Separately from the access failure, the offsets claimed in the six reports
  are: inkling `06AF` **1755 (`0x6DB`, wrong)**; north `06AF` **11439
  (`0x2CEF`, wrong)**, and it falsely equates `06AF` with that offset; north `0136`
  **no offset at all**; inkling `0136` and nemotron `0136` **30217 (`0x7609`,
  correct)**; nemotron `06AF` **31618 (`0x7B82`, correct)** but it explicitly
  reconstructs the bytes from the recovered C and its addresses are
  discontinuous. That is three wrong/missing, two correct and one
  correct-but-reconstructed — not four wrong. The model errors are not excused
  by the media failure, and the media failure is not a model score.

Therefore the current judge's 0/10 is **not** evidence that the models cannot
decode these bodies, and `0/10` must not be quoted as a capability score of
any model. The run must be repeated with a media-accessible repo and 429
retries before any model comparison is drawn.

## The ten runs, from `results.tsv`

| Model | Body | Wrapper label | Seconds |
| --- | --- | --- | ---: |
| `recursive/flint-v1.0:free` | `OVL_0025_06AF` | EMPTY (upstream 429) | 102 |
| `recursive/flint-v1.0:free` | `OVL_0025_0136` | EMPTY (upstream 429) | 128 |
| `thinkingmachines/inkling:free` | `OVL_0025_0136` | OFFSET UNRESOLVED (30217 claimed) | 162 |
| `cohere/north-mini-code:free` | `OVL_0025_0136` | OFFSET UNRESOLVED (no offset) | 177 |
| `thinkingmachines/inkling:free` | `OVL_0025_06AF` | OFFSET UNRESOLVED (1755 claimed) | 215 |
| `cohere/north-mini-code:free` | `OVL_0025_06AF` | OFFSET UNRESOLVED (11439 claimed) | 376 |
| `xingchen-agi/xing4.0-29b-a4b:free` | `OVL_0025_0136` | EMPTY (upstream 429) | 26 |
| `nvidia/nemotron-3.5-super-vl:free` | `OVL_0025_0136` | OFFSET UNRESOLVED (30217 claimed) | 96 |
| `nvidia/nemotron-3.5-super-vl:free` | `OVL_0025_06AF` | DIFFERS (31618 reconstructed) | 299 |
| `xingchen-agi/xing4.0-29b-a4b:free` | `OVL_0025_06AF` | EMPTY (upstream 429) | 51 |

`results.tsv` is the authority for these labels; the table adds only what the
report files themselves say about the offset.

## Earlier receipts with media accessible (not re-run)

`/home/hunter/workspace/vp-of-agentry/data/bakeoff/20261024-213551/results.tsv`
and its per-run reports:

| Model | `OVL_0025_06AF` (36 B) | `OVL_0025_0136` (128 B) |
| --- | --- | --- |
| `recursive/flint-v1.0:free` | VERIFIED 16 B (prefix) | VERIFIED 128 B (full) |
| `thinkingmachines/inkling:free` | VERIFIED 36 B (full) | VERIFIED 6 B (prefix) |
| `cohere/north-mini-code:free` | VERIFIED 36 B (full) | VERIFIED 16 B (prefix) |
| `nvidia/nemotron-3.5-super-vl:free` | VERIFIED 16 B (prefix) | NO DUMP |
| `xingchen-agi/xing4.0-29b-a4b:free` | EMPTY | EMPTY |

These came from the 2026-10-24 checker and are the reason the 2026-10-27 run
cannot be read as a model regression. Explicitly: Flint `0136` 128/128 B is a
full-body match; Flint `06AF` 16/36 B is a prefix, not a full body. Current
Flint runs are EMPTY after 429, so the current byte checker was never invoked
for them.

## Harness-format control

With the exact judge SHA-256 `7ae74e7abd581e226453b185d059cf35e90934af3ae4802efa6b6e5839745831`,
a known-good complete 36-byte `06AF` dump at 31618 passes with exit 0 /
`BYTES MATCH`. Prefixing only a repository-style timestamp makes the same dump
exit 1 / `BYTES DIFFER`: the timestamp is parsed as a malformed address token.
That is an additional judge-format false rejection; it does not validate any
unavailable-media output from this run.

## Harness and evidence

- `free_bakeoff.sh` SHA-256 `3230c0b6d3cd706ada5cf15177a401f6da642ab3c7b67f41fde92522f602d1d9`
  (prompt: `PROMPT_FOR`, lines 20-30)
- `free_sandbox.sh` SHA-256 `a2dd6085fab12cc909a61c35bf4f536dd646d5d632c5257540af2cadd5323167`
- current strict judge `free_body_check.py` SHA-256
  `7ae74e7abd581e226453b185d059cf35e90934af3ae4802efa6b6e5839745831`,
  at `/home/hunter/workspace/vp-of-agentry-wt-themes2/tools/free_body_check.py`
  for this run. A byte-identical copy is committed in the poolrad tree as
  `tools/free/free_body_check.py` (reviewed with #1603). The judge is **not**
  in this branch of the tree; the hash is of the harness file actually run.
- citation judge `free_verify_citations.py` SHA-256
  `d7c6b9005f81f0aec00f98023b65dceae59c46040c80db4f3b315732318061f4`
  (**the run's** copy at
  `/home/hunter/workspace/vp-of-agentry-wt-themes2/tools/free_verify_citations.py`).
  The committed poolrad copy is now `c9880a32…` after the #1603 review fixed
  its exit contract; the two differ, and this report describes the run's copy.
- raw root: `/home/hunter/workspace/vp-of-agentry-wt-themes2/data/bakeoff/20261027-180647`
- invocation: `POOLRAD_REPO=/tmp/opencode/t40-wt tools/free_bakeoff.sh "<5 models>" "OVL_0025_0136:30217:128 OVL_0025_06AF:31618:36" 3`
- receipts: 10 report/stderr pairs and six `.check` files under that root; original media and any derived dumps stay private.

## Limits / not verified

- The 2026-10-27 run is invalid for model scoring; no valid re-run was done.
- The 2026-10-24 receipts were not re-run under the current strict checker.
- The harness scores a quoted dump; it does not compile a C draft, so
  "compiles" is not tested.
- No pool restart, no credit, no whole-body/native/parity claim.
