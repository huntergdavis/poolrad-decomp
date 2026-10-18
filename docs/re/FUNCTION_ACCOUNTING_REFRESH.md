# Reviewed function accounting refresh

The current [machine checkpoint](function_accounting_checkpoint.json) is
**118 strict full-C originals** (109 overlay, nine resident) and **174 verified
game identities** (164 overlay, ten resident). The queue still contains 898
candidate starts: 673 overlay, 145 non-SYSTEM resident, and 80 SYSTEM closure
seeds. There are 89 inferred-library starts and 666 unknown-ownership starts.
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
| `OVL_002F_122B` | PR #481, `8a03a1b64a8d0e60d8a940d47f6eb47fb54a38ea`; PM exact-head review | 79 bytes, SHA-256 `ef58c3eedf096f21b9b923c9c303dfc629e5f0935d2673dabf93e1bc9749de9a` | [record link clearer](U2F_RECORD_LINK_CLEAR_122B.md) |
| `OVL_006C_3F31` | PR #491, `df9d516cb6862e84a0f749e709f79913dfb042d3`; PM exact-head review | 130 bytes, SHA-256 `9f192cb8fe704324facbbec368d1e24602558b878aa3f7e0031f644862e09767` | [record/table byte sum](U6C_RECORD_TABLE_SUM_3F31.md) |
| `OVL_007F_0F7E` | PR #492, `04d96904fd74bbb7586db3d992cf6fa959072e4c`; PM exact-head review | 84 bytes, SHA-256 `5db57d103e9038273938138f1980a39a0af093ac1b811d8e50fa178f5371c51b` | [state flags](U7F_STATE_FLAGS_0F7E.md) |
| `OVL_002F_0131` … `OVL_005E_2574` (17 bodies) | Batch acceptance PR #870, `967f4277e707065d75ba7d0351a8bc8b7dd4f872`; VP inbox i17223 | pinned body SHA-256 and extents per body in [reviewed_function_identities.csv](reviewed_function_identities.csv) and each evidence note | [PR #870](https://github.com/huntergdavis/poolrad-decomp/pull/870) |
| `OVL_03C2_01A0` | PR #872, `f44e528e894ddbb44f8cd22d8afd8059f0977390`; VP independent review | 300 bytes, SHA-256 `7fbacb8d9d298c72bd886321a58a8d842f2ec6dc350fc83fde6064fe78f27f40` | [line-record initializer](U3C2_LINE_INIT_01A0.md) |
| `OVL_006A_0000`, `OVL_006A_019C`, `OVL_0033_012B`, `OVL_0033_01AD` | PRs #845 (`dc3c4717d7ec121bf51eba65ad0642e1ba189cd9`) and #846 (`eeba7425649b377ce64dfced230e378c77eae927`); independently verified by the ds4 lane | pinned per body in [reviewed_function_identities.csv](reviewed_function_identities.csv) and each evidence note | [record weight](U6A_RECORD_WEIGHT_0000.md), [link clear](U6A_LINK_HIGH_CLEAR_019C.md), [cell lookup](U33_CELL_LOOKUP_012B.md), [cell pair](U33_CELL_PAIR_01AD.md) |
| `OVL_03C2_0422` | PR #875, `d1398522367d3dc4de69b706edb86b008c927e52`; independently verified by the ds4 lane (1,430 cases, 368/368 coverage) | 345 bytes, SHA-256 `f075c6dba0b31e12aea19318958a72f4caee6fe7954c53f3583dd5dd9fa8114d` | [line check](U3C2_LINE_CHECK_0422.md) |
| `OVL_0033_0615` | PR #877, `51eddc9a633b6b86c2caae5cf3e6b3046719055f`; independently verified by the ds4 lane (956 cases, 196/196 caller coverage) | 507 bytes, SHA-256 `c973402de9f97dbf004ece46254273f351e9a0a8600bc1f442dbbd9b1b4371e9` | [grid pattern](U33_GRID_PATTERN_0615.md) |
| `OVL_0033_040F` | PR #880, `74dbfe77ec4fe4e39a8ea8ed75e4c1210bc8e428`; independently verified by the ds4 lane (1,046 cases, 209/209 caller coverage) | 518 bytes, SHA-256 `880652652d8a0222747e0f99b090418fc29d674e547b9809e0d5855a0f4bcc04` | [grid pattern](U33_GRID_PATTERN_040F.md) |
| `OVL_0033_1434`, `OVL_0033_1577` | PR #881, `de5f7fc4e277d98d75f2adf4c0f7ac4870e48a25`; independently verified by the ds4 lane (213 + 170 cases; 138/138 and 469/470 starts with the declared infeasible edges) | 323 B `b01749df…`, 1,150 B `c9418332…` | [cell claim](U33_CELL_CLAIM_1434.md), [place search](U33_PLACE_SEARCH_1577.md) |
| `OVL_0033_02BA`, `OVL_0033_0377` | PR #883, `5ca69bc0759b4ea62956927ea42b83daac5ac391`; independently verified by the ds4 lane (158 cases each; 86/86 and 82/82 starts) | 189 B `a1e4f0b3…`, 152 B `aa813423…` | [stamp block](U33_STAMP_BLOCK_02BA.md), [stamp square](U33_STAMP_SQUARE_0377.md) |
| `OVL_0033_0221` | PR #850, `e518c1314968e775dab77fc04bade7e4b6ee4c05`; dependency credited on its own audit re-run by the ds4 lane (821 cases, 64/64 instructions, 67 CFG edges) | 153 bytes, SHA-256 `9a97dca1163b4aabd6193cea3a81427143709390e7b4aa2dec1a2a727236cd06` | [clipped writer](U33_CLIPPED_WRITER_0221.md) |
| `OVL_006C_4235` | PR #886, `5d7ff51b1f263d07c73d1bdcd899257ee4365ee2`; independently verified by the Claude lane (1,609 cases with 16 declared transfers, 134/134 sites, 17 feasible edges; 620 native cases) | 184 bytes, SHA-256 `fdb467838a6d2067178154c2b0bf3d60f0acff37060f337977e3d66d24391d60` | [record roll sum](U6C_RECORD_ROLL_SUM_4235.md) |
| `OVL_0025_0482` | PR #888, `674a1f6590ad01919491a6108c2f1fcbb52d431d`; independently verified by the Claude lane (2,083 composed cases without masks, 173/173 chain instructions, 16/16 caller outcomes; 157 legacy callback cases) | 143 bytes, SHA-256 `bcfe56989d4733d260e0f85a7146d4cb56de3041cbf20b7de27851f0f513df2b` | [composed step probe](U25_STEP_PROBE_0482_COMPOSED.md) |
| `OVL_0033_0810` | PR #887, `8f456eaa3e8c4682737c56265fe9a8ecfc636c7b`; independently re-verified by the ds4/pi lane at head 22a774b7 (68 original-byte cases, full 1 MiB RAM and all 14 CPU words, 71/71 instruction starts, 4/4 outcomes; 24 native DOSBox-X normal-core/386 cases, no flag mask) | 148 bytes, SHA-256 `73d7b42bd2b006650d6856afd73118648ac17bd55a7c7eceafe2bf539e5569ac` | [neighbourhood sweep](U33_NEIGHBOURHOOD_0810.md) |
| `OVL_006A_0214` | PR #892, `e88dde1fa37da739ba9dbd4322c59a6a3098cd1b`; independently re-verified by the ds4/pi lane at head 99ed89ca (125 returning original-byte cases, full 1 MiB RAM and all 14 CPU words, 27/27 starts, 2/2 outcomes, 2 cyclic originals capped by the host LIMIT guard; 30 native DOSBox-X normal-core/386 cases, no flag mask) | 72 bytes, SHA-256 `35ee0bd09567048899c218e1874c5884fa7eaad1c08ccdc2198b2b85eb2fe939` | [record walk](U6A_RECORD_WALK_0214.md) |
| `OVL_03C6_0068` | PR #894, `ce519c85cc8cbc7e6a91902ced4b4996ba996adb`; independently re-verified by the ds4/pi lane at head 47b0034 (188 returning original-byte cases, full 1 MiB RAM and all 14 CPU words, 51/51 starts, 4/4 outcomes, 5 host-cap stops equal to the original at +0080; 36 native DOSBox-X normal-core/386 cases, no flag mask) | 123 bytes, SHA-256 `c429a91a0c99d1b512cbf00f5ab9b7423070f03114a0f8d3de89fbd1784a3acb` | [slot offsets](U3C6_SLOT_OFFSETS_0068.md) |
| `OVL_0038_2002` | PR #896, `194fb33d5a6be55f9607813afab0e0c2573bc04d`; independently re-verified by the ds4/pi lane at head 2c6fde3c (2,651 returning original-byte cases, exact full 1 MiB RAM and 14 CPU words, 75/75 instructions, 16/16 outcomes; 8 named alias fixtures excluded at the 20,000-instruction budget; 4 native cases, selected RAM) | 221 bytes, SHA-256 `cdf3d522f6de4b8f37ccc47e3267e244f26cab4866883306b9dcfe1525dd2af4` | [leaf selector](U38_LEAF_2002.md) |

The first two historical changes produced 67/129 and 68/130. The seven latest
advanced the prior 75/136 checkpoint to 76/137, 77/138, 78/139, 79/140,
80/141, 81/142, and 82/143. Each is
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
saved-game calls remain unmeasured. U2F +122B has original-byte and native
party-caller parity; natural saved-game reach remains unmeasured. U6C +3F31
has 2,522 original-byte full-RAM/all-CPU cases, 1,282 native original-caller
cases, and three aligned game-evidenced callers. Its natural saved-game reach
remains unmeasured. U7F +0F7E has 389 returning original-byte full-RAM/all-CPU
cases, 256 native copied-leaf cases, and seven aligned fixup-backed game
callers. One alias fixture exceeded the original return budget and is excluded
from returning-path parity; natural saved-game reach remains unmeasured. For
U33 +0810, native parity uses the DOSBox-X normal-core 386 profile
(`core=normal`, `cputype=386`, `cycles=max`); the synthetic harness relocates
the five far-call segment words to one stub paragraph and copies the unit
0x33/0x3BD/0x9E spans plus the SYSTEM generator to absolute `0AF8:11C0`. The
native comparison covers 14 CPU words, a 256-byte frame window, the
1,264-byte grid and the seed (not all RAM); the original-byte Unicorn
differential covers the whole 1 MiB RAM and all 14 CPU words. Neither proves
natural saved-game reach. None of these approvals completes a continuous
playable C route.

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
