# Reviewed function accounting refresh

The current [machine checkpoint](function_accounting_checkpoint.json) is
**79 strict full-C originals** (70 overlay, nine resident) and **140 verified
game identities** (130 overlay, ten resident). The queue still contains 898
candidate starts: 673 overlay, 145 non-SYSTEM resident, and 80 SYSTEM closure
seeds. There are 89 inferred-library starts and 669 unknown-ownership starts.
The whole-game denominator remains unknown; no completion percentage follows
from these counts. The [battle card](../BATTLE_CARD.md) uses this checkpoint.

## Approved changes

The PM explicitly approved each listed +1 identity/+1 strict-C change after
merge. Passing tests alone never promotes an original.

| Original | Review and exact merge | Complete body | Evidence |
| --- | --- | --- | --- |
| `OVL_0025_0756` | PR #380, `bb77539ed4444ad2bd430ce3ed88d8322902a328`; PM inbox i2886 | 102 bytes, SHA-256 `b307e1dd310b369eeb068ef1e0fd7b84667e59382144bec02e5295b974c3fcb9` | [party-list index](U25_PARTY_INDEX_0756.md) |
| `OVL_005E_0E52` | PR #383, `fb342dbef3b38fcd33c88e907f0c2dda69316948`; PM inbox i2920 | 41 bytes, SHA-256 `c99969f9b7b28b6bdd15297e8b711a8491b605df8055135eb114b6215dfc82a2` | [byte-half helper](U5E_HALF_0E52.md) |
| `OVL_009E_0E2E` | PR #468, `cc3b27f84d91ea7b8aeedea9a915c9d0d49a2dbe`; PM exact-head review | 36 bytes, SHA-256 `08dc2c3413d0fcfa12d26b589873f619761741e3f38c235477e89ed3a7350443` | [roll latch wrapper](U9E_ENTRY9_ROLL_LATCH_0E2E.md) |
| `OVL_0038_31A2` | PR #467, `aca640cf5617d5625e59564b0ed95d807a3bf2a2`; PM exact-head review | 1,799 bytes, SHA-256 `4a60eda9584cb62276f52b14e2ae7af2077857c521f3d5a835a344f2f50b2519` | [effect-table installer](U38_EFFECTS_INSTALL_31A2.md) |
| `OVL_006A_0159` | PR #472, `7d319e2b7a2d73b713ade29069693d9de581d848`; PM exact-head review | 67 bytes, SHA-256 `a130919171e4e4b45af9155e9cd7e16abc1027eb83e11fbdf801fca7a0d3c099` | [record high-byte clearer](U6A_RECORD_CLEAR_HIGH_0159.md) |
| `OVL_007D_02F2` | PR #476, `62e1fb09c8500d6da1192824c737c2a65ba7d476`; PM exact-head review | 177 bytes, SHA-256 `8f39276ad357b6e05367ffc080afedc0df5a5cca2f3cbf54b3d96db18bea3dd9` | [carry-mask helper](U7D_CARRY_02F2.md) |

The first two historical changes produced 67/129 and 68/130. The four latest
advanced the prior 75/136 checkpoint to 76/137, 77/138, 78/139, and 79/140. Each is
credited once by original address, regardless of entry slots, runtime stubs,
aliases, or the number of C symbols. Unit `0x25:+0x1EA6` remains an extracted
core with zero strict credit. PRs #387/#388/#389 receive no additional identity
or strict-C credit in this refresh.

For `+0x0756`, native entry/return parity used the documented controlled
script edit from address `0x6DCA` to `0x6DB1` at PC `0x9B40` after the second
Up. Natural unedited-script reachability remains unmeasured. The `+0x0E52`
native oracle is a synthetic routine harness; saved-game reachability remains
unmeasured. The newly approved U6A +0159 and U7D +02F2 routines have
bounded original-byte and synthetic native evidence; their natural
saved-game calls remain unmeasured. None of
these approvals completes a continuous playable C route.

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
