# Function conversion backlog, step 0 (measured at main `95218080e03299a8d2b91a0ab8a1ceb3add82f8d`)

Exact list of identified-but-uncredited game function bodies: **45 identities** (40 mechanically-checkable, 5 interpretation-heavy), published as blocks of ten, mechanically-checkable first. Owner/checker slots are filled by the leader when a block is assigned; a pre-filled slot is an assignment nobody made.

**Authoritative artefact:** `function_backlog_step0_95218080.csv` (this document is GENERATED from it by `render_function_backlog_step0.py`; edit the CSV and regenerate, never hand-edit this page).

## Derivation (measured)

1. `function_backlog_reconciliation.csv` rows with `game_ownership == game_verified` = 335 (287 overlay + 48 resident), matching `function_accounting_checkpoint.json` (`verified_game_identities: 335`).
2. Rows with `strict_full_c == 1` = 290, matching `strict_full_c_originals: 290` and the 290 ids in `strict_full_c_addresses.csv`.
3. Gap = **45**: rows with `game_ownership == game_verified`
   AND `strict_full_c == 0` (44 overlay + 1 resident), equal to 335 - 290.

**Do NOT derive this list by string-subtracting the two ID files.** Naive subtraction gives 92, not 45: the 47 credited resident ids in `strict_full_c_addresses.csv` use `RES_<seg>_<off>` naming (`RES_00BA_0000`) while the reconciliation uses `RES_IMAGE_<imgoff>` (`RES_IMAGE_000F6`), so they never string-match. The per-row `strict_full_c` flag is the only consistent join without a segment:offset to image-offset mapping.

**Ledger join key** (for joining `OVL_` ids to `overlay_function_ledger.csv`): the ledger's `id` is `0x<unit unpadded hex>:0x<offset padded to 4>` (e.g. `OVL_001C_0001` is `0x1c:0x0001`). The `OVL_` form does not string-match the ledger id; join via the reconciliation's `original_address` column.

## Classification rule (proxy, not a conversion claim)

- **mechanically-checkable**: body fully decoded in `overlay_function_ledger.csv` (`decoded == bound`), grade A or B (own caller contract: entry site or a specific near-call site), no alias entry.
- **interpretation-heavy**: partial decode (`decoded < bound`), grade D (alias entry, semantics inferred), or contract is fallthrough rather than a call site.

This is a classification from decode/ledger metadata only. `recorded_body_bytes` is empty on all 45 rows and no compiler was run: "mechanically-checkable" means the byte-identical-body + caller-contract check is *well-posed from recorded evidence*, not that any body is verified convertible. Reviewer verification note: the resident row's fallthrough characterisation is the author's read of the resident ledger, not independently reviewed.

## Blocks of ten

### Block 1 (10 identities, mechanically-checkable) — owner: — | checker: —

| id | addr | grade | ledger class | decoded/bound | insns | ret | callers | checkability |
|---|---|---|---|---|---|---|---|---|
| OVL_001C_0001 | 0x1c:0x0001 | B | state | 243/243 | 91 | retf | entry=0;far=0;near=2 | mechanically-checkable |
| OVL_001C_0126 | 0x1c:0x0126 | B | state | 125/125 | 50 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_01A3 | 0x1c:0x01a3 | B | state | 145/145 | 57 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0234 | 0x1c:0x0234 | B | state | 94/94 | 35 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0292 | 0x1c:0x0292 | B | state | 78/78 | 30 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_02E0 | 0x1c:0x02e0 | B | state | 212/212 | 70 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_03B4 | 0x1c:0x03b4 | B | state | 156/156 | 56 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0450 | 0x1c:0x0450 | B | state | 967/967 | 306 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0817 | 0x1c:0x0817 | B | state | 63/63 | 25 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0856 | 0x1c:0x0856 | B | state | 251/251 | 91 | retf | entry=0;far=0;near=1 | mechanically-checkable |

### Block 2 (10 identities, mechanically-checkable) — owner: — | checker: —

| id | addr | grade | ledger class | decoded/bound | insns | ret | callers | checkability |
|---|---|---|---|---|---|---|---|---|
| OVL_001C_0951 | 0x1c:0x0951 | B | state | 67/67 | 27 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0994 | 0x1c:0x0994 | B | state | 117/117 | 49 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0A09 | 0x1c:0x0a09 | B | state | 150/150 | 65 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0BD1 | 0x1c:0x0bd1 | B | state | 163/163 | 66 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0C74 | 0x1c:0x0c74 | B | state | 379/379 | 129 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0DEF | 0x1c:0x0def | B | state | 111/111 | 43 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0E5E | 0x1c:0x0e5e | B | state | 94/94 | 35 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_0EBC | 0x1c:0x0ebc | B | state | 76/76 | 29 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_10D1 | 0x1c:0x10d1 | B | state | 396/396 | 161 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_12C6 | 0x1c:0x12c6 | B | state | 273/273 | 100 | retf | entry=0;far=0;near=1 | mechanically-checkable |

### Block 3 (10 identities, mechanically-checkable) — owner: — | checker: —

| id | addr | grade | ledger class | decoded/bound | insns | ret | callers | checkability |
|---|---|---|---|---|---|---|---|---|
| OVL_001C_1435 | 0x1c:0x1435 | B | state | 500/500 | 177 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_1629 | 0x1c:0x1629 | B | state | 160/160 | 55 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_16C9 | 0x1c:0x16c9 | B | glue | 210/210 | 95 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_179B | 0x1c:0x179b | B | state | 349/349 | 121 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_19B0 | 0x1c:0x19b0 | B | state | 1025/1025 | 405 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_1EF8 | 0x1c:0x1ef8 | B | state | 1747/1747 | 653 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_25CB | 0x1c:0x25cb | B | state | 159/159 | 66 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_2775 | 0x1c:0x2775 | B | state | 813/813 | 296 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_2AA2 | 0x1c:0x2aa2 | B | state | 31/31 | 10 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_2AC1 | 0x1c:0x2ac1 | B | glue | 42/42 | 18 | retf | entry=0;far=0;near=1 | mechanically-checkable |

### Block 4 (10 identities, mechanically-checkable) — owner: — | checker: —

| id | addr | grade | ledger class | decoded/bound | insns | ret | callers | checkability |
|---|---|---|---|---|---|---|---|---|
| OVL_001C_2B33 | 0x1c:0x2b33 | B | glue | 75/75 | 36 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_2B7E | 0x1c:0x2b7e | B | state | 133/133 | 43 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_2C03 | 0x1c:0x2c03 | B | state | 204/204 | 74 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_2CCF | 0x1c:0x2ccf | B | state | 260/260 | 88 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_2E10 | 0x1c:0x2e10 | B | state | 109/109 | 40 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_001C_2F2D | 0x1c:0x2f2d | B | state | 580/580 | 273 | retf | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_0025_0A74 | 0x25:0x0a74 | B | state | 515/515 | 163 | retf 4 | entry=0;far=0;near=1 | mechanically-checkable |
| OVL_0025_0C77 | 0x25:0x0c77 | A | state | 405/405 | 141 | retf 4 | entry=1;far=35;near=0 | mechanically-checkable |
| OVL_03BD_0E08 | 0x3bd:0x0e08 | A | state | 703/703 | 278 | retf 4 | entry=1;far=8;near=0 | mechanically-checkable |
| OVL_03BD_10C7 | 0x3bd:0x10c7 | A | state | 319/319 | 124 | retf 2 | entry=1;far=2;near=0 | mechanically-checkable |

### Block 5 (5 identities, interpretation-heavy) — owner: — | checker: —

| id | addr | grade | ledger class | decoded/bound | insns | ret | callers | checkability |
|---|---|---|---|---|---|---|---|---|
| OVL_001C_0F08 | 0x1c:0x0f08 | B | state | 456/457 | 201 | retf | entry=0;far=0;near=1 | interpretation-heavy |
| OVL_001C_1DB1 | 0x1c:0x1db1 | B | state | 324/327 | 133 | retf | entry=0;far=0;near=1 | interpretation-heavy |
| OVL_001C_3171 | 0x1c:0x3171 | B | state | 66/67 | 22 | retf 2 | entry=0;far=0;near=9 | interpretation-heavy |
| OVL_0036_002D | 0x36:0x002d | D | state | 1512/1512 | 585 | retf | entry=0;far=0;near=0 | interpretation-heavy |
| RES_IMAGE_000F6 | image:0x000F6 | A |  | n/a |  |  | init_fallthrough=1 | interpretation-heavy |

## Limits

- The count (45) is a fact measured at main `95218080e03299a8d2b91a0ab8a1ceb3add82f8d`; it moves as PRs merge. Re-run the derivation before quoting it against a different head.
- Classification is proxy metadata; no body conversion or compilation was attempted (step 0 is list-only).
- The 92-vs-45 discrepancy is a naming artifact, not a finding of 92 uncredited bodies; the 47 mismatched ids are all credited residents.
- Owner/checker slots are deliberately empty; the leader fills them at assignment time.
