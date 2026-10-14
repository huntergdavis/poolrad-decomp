# Reviewed function accounting refresh

The current [machine checkpoint](function_accounting_checkpoint.json) is
**68 strict full-C originals** (59 overlay, nine resident) and **130 verified
game identities** (120 overlay, ten resident). The queue still contains 898
candidate starts: 673 overlay, 145 non-SYSTEM resident, and 80 SYSTEM closure
seeds. There are 89 inferred-library starts and 679 unknown-ownership starts.
The whole-game denominator remains unknown; no completion percentage follows
from these counts. The [battle card](../BATTLE_CARD.md) uses this checkpoint.

## Approved changes

The PM explicitly approved these two independent +1 identity/+1 strict-C
changes after merge. Passing tests alone never promotes an original.

| Original | Review and exact merge | Complete body | Evidence |
| --- | --- | --- | --- |
| `OVL_0025_0756` | PR #380, `bb77539ed4444ad2bd430ce3ed88d8322902a328`; PM inbox i2886 | 102 bytes, SHA-256 `b307e1dd310b369eeb068ef1e0fd7b84667e59382144bec02e5295b974c3fcb9` | [party-list index](U25_PARTY_INDEX_0756.md) |
| `OVL_005E_0E52` | PR #383, `fb342dbef3b38fcd33c88e907f0c2dda69316948`; PM inbox i2920 | 41 bytes, SHA-256 `c99969f9b7b28b6bdd15297e8b711a8491b605df8055135eb114b6215dfc82a2` | [byte-half helper](U5E_HALF_0E52.md) |

The first change produces 67/129; the second produces 68/130. Each is credited
once by original address, regardless of entry slots, runtime stubs, aliases,
or the number of C symbols. Unit `0x25:+0x1EA6` remains an extracted core with
zero strict credit. PRs #387/#388/#389 receive no additional identity or
strict-C credit in this refresh.

For `+0x0756`, native entry/return parity used the documented controlled
script edit from address `0x6DCA` to `0x6DB1` at PC `0x9B40` after the second
Up. Natural unedited-script reachability remains unmeasured. The `+0x0E52`
native oracle is a synthetic routine harness; saved-game reachability remains
unmeasured. Neither change completes a playable C route or expands the
separate 128-of-64,000-pixel comparison scope.

## Canonical inputs and generated outputs

- [strict_full_c_addresses.csv](strict_full_c_addresses.csv) maps reviewed
  strict-C originals to evidence documents.
- [reviewed_function_identities.csv](reviewed_function_identities.csv) adds
  reviewed overlay identities to the classifier's historical 118-address
  `BASE_VERIFIED` set. Each addition records its PR, exact merge, body length,
  body hash, and evidence path. Duplicate identities fail validation.
- [overlay_classification.csv](overlay_classification.csv) and its
  [generated summary](overlay_classification_summary.md) are regenerated from
  the hash-locked media and identity set, including changed caller signals.
- [function_accounting_checkpoint.json](function_accounting_checkpoint.json)
  pins the resulting queue hash, input hashes, metadata hashes, separate
  counts, domain splits, and most recent reviewed change. The alias audit
  reads this checkpoint instead of assuming 66 strict and 128 verified.

The frozen reports in [FUNCTION_ALIAS_AUDIT.md](FUNCTION_ALIAS_AUDIT.md)
retain their original counts and hash; their reproduction command supplies
explicit historical expectations. They do not describe the current totals.
The queue exporter can bind the reviewed identity CSV with
`--reviewed-identities`; the owner-media audit always binds it.

## Apply the next approved change

Use one PM-approved record per run. `original_id` must be a canonical queue
ID or an exact alias already present in the queue. An example record for the
second change above is:

```json
{
  "status": "approved",
  "approved_by": "PM",
  "approval_reference": "inbox:i2920",
  "review_pr": 383,
  "reviewed_commit": "fb342dbef3b38fcd33c88e907f0c2dda69316948",
  "original_id": "OVL_005E_0E52",
  "evidence_doc": "docs/re/U5E_HALF_0E52.md",
  "body_sha256": "c99969f9b7b28b6bdd15297e8b711a8491b605df8055135eb114b6215dfc82a2",
  "body_bytes": 41,
  "game_identity": true,
  "strict_full_c": true
}
```

That original is already credited in the current checkpoint: replaying this
record against the current queue must fail. To inspect a future approved
change, put its record in ignored `resources/` and supply the exact queue
whose hash matches the current checkpoint:

```sh
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/function_accounting_refresh.py \
  --approval resources/function-accounting-refresh/approval-next.json \
  --baseline-queue resources/function-accounting-current/queue.json \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --out-dir resources/function-accounting-refresh/next
```

Without `--apply`, only ignored staging products are written. After reviewing
them, repeat with `--apply` to publish canonical CSVs, generated summary,
checkpoint, and the marked battle-card paragraph. Optional paired
`--dashboard-snapshot` and `--dashboard-html` paths refresh a local dashboard's
queue identity/credit fields, counters, embedded JSON, and accounting
checkpoint while preserving replay observations and pixel comparisons.
The dashboard files in this workspace are untracked local artifacts and are
outside the public PR.

Before staging, the workflow requires matching baseline manifests and
classifier identities, supported media hashes, an evidence note containing
the body hash, and the exact reviewed merge as an ancestor of HEAD. It derives
the original span from the media inventory and verifies its body digest.
It then regenerates inventory, callgraph, near-call audit, both ledgers,
classification, and two byte-identical queues. Only the approved original
may gain identity or strict credit; the original-address set and unknown
denominator must stay unchanged. Duplicate alias credits and already-applied
approvals fail. Identity-only and strict-only approvals have separate deltas.

Publication follows alias reconciliation, all 41 synthetic test methods,
`git diff --check`, oracle shell syntax checks, prior-research checksums, and
tracked-media hygiene. Individual files are replaced atomically after these
checks; publication is not a transaction across all output files. If an I/O
failure interrupts replacement, restore the previous source checkpoint and
rerun from its matching baseline. Resident promotions, new candidate starts,
demotions, and inferred-library ownership changes require separate reviewed
workflows and are rejected here.

## Reproduce the current checkpoint

With Capstone 5.0.7 and the owner media present:

```sh
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/function_queue_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --out-dir resources/function-accounting-current
```

This compares all three regenerated CSV ledgers byte-for-byte with the
checked-in files. The two queues must agree with each other and with the
SHA-256 in the published checkpoint. Diagnostics and generated products stay
under ignored `resources/`; public evidence contains counts, addresses,
hashes, and synthetic tests only.

The required prior-session query,
`deja "poolrad strict_full_c_addresses classifier accounting regeneration 66 128 reviewed credit"`,
returned no matches. No prior-session result was reused. This workflow reuses
the checked-in [queue audit](FUNCTION_CENSUS_QUEUE.md),
[alias reconciliation](FUNCTION_ALIAS_AUDIT.md), classifier, resident audit,
and the two approved body notes linked above.
