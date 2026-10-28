# t106 bounded free-model bake-off

## Result: the 2026-10-27 run is INVALID as a model score

Run dir:
`/home/hunter/workspace/vp-of-agentry-wt-themes2/data/bakeoff/20261027-180647`
(`results.tsv` plus 10 `.md`/`.err`/`.check` triples). Five candidate models,
two verified-C bodies (`OVL_0025_0136` at GAME.OVR 30217/128 B and
`OVL_0025_06AF` at 31618/36 B), 10 runs, parallel 3. The systemd free pool was
not restarted (`poolrad-free-pool` and `poolrad-free-trial` are inactive).

The raw files show two tool/provider failures, not a model result:

- **EMPTY (flint 2, xing 2): upstream 429.** The report `.md` is 0 bytes and
  `.err` carries the OpenRouter JSON: "temporarily rate-limited upstream"
  (providers Recursive and Phala).
- **OFFSET UNRESOLVED / DIFFERS (inkling 2, north 2, nemotron 2): broken
  media access, and in four reports also a genuinely wrong/missing offset.**
  All six non-empty reports state that `resources/DOS/POOLRAD/GAME.OVR` is
  missing/the symlink dangles, so none produced a quoted dump. The invocation
  used `POOLRAD_REPO=/tmp/opencode/t40-wt`, whose private `resources` link is
  not visible inside the free sandbox. Separately from the access failure:
  inkling `06AF` gives 1755 (`0x6DB`, wrong); north `06AF` gives 11439
  (`0x2CEF`, wrong) and falsely equates `06AF` with it; north `0136` gives no
  offset. Inkling `0136` and nemotron `0136` give the correct 30217;
  nemotron `06AF` gives 31618, but explicitly reconstructs the bytes from the
  recovered C and its addresses are discontinuous. The model errors are not
  excused by the media failure, and the media failure is not a model score.

Therefore the current judge's 0/10 is **not** evidence that the models cannot
decode these bodies. The run must be repeated with a media-accessible repo and
429 retries before any model comparison is drawn.

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
cannot be read as a model regression.

## Harness and evidence

- `free_bakeoff.sh` SHA-256 `3230c0b6d3cd706ada5cf15177a401f6da642ab3c7b67f41fde92522f602d1d9`
  (prompt: `PROMPT_FOR`, lines 20-30)
- `free_sandbox.sh` SHA-256 `a2dd6085fab12cc909a61c35bf4f536dd646d5d632c5257540af2cadd5323167`
- current strict judge `free_body_check.py` SHA-256
  `7ae74e7abd581e226453b185d059cf35e90934af3ae4802efa6b6e5839745831`
- citation judge `free_verify_citations.py` SHA-256
  `d7c6b9005f81f0aec00f98023b65dceae59c46040c80db4f3b315732318061f4`
- invocation: `POOLRAD_REPO=/tmp/opencode/t40-wt tools/free_bakeoff.sh "<5 models>" "OVL_0025_0136:30217:128 OVL_0025_06AF:31618:36" 3`

## Limits / not verified

- The 2026-10-27 run is invalid for model scoring; no valid re-run was done.
- The 2026-10-24 receipts were not re-run under the current strict checker.
- The harness scores a quoted dump; it does not compile a C draft, so
  "compiles" is not tested.
- No pool restart, no credit, no whole-body/native/parity claim.
