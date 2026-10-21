# RES_0802: identity arguments for the nine side (i) bodies (t30, proposal for acceptance)

**Proposal text only.** No identity, strict, accounting or manifest row is written here, and the review columns below are
blank for par-e. Each body already has its raw-CPU evidence document on main; this page adds only the two-factor argument
(#1195) in the format of the U3B0 four (`CORPUS_STATE_2026-10-20.md` §1a). Bodies are listed in the order the build
needs them: **0040 first**, because `0175` (side ii, never-credit) calls it and cannot join `resident_runtime` until
0040 is ready (#1336).

Basis: main `37d7c6c8`; caller classifications from a fresh export regenerated on that tree (`function_queue_audit.py`,
GAME.OVR `872964a3...`). Caller sites are **asserted** by `tools/overlay/res0802_identity_census.py` (segment-wide raw
E8/E9, relocated 9A/EA, overlay fixups; it also asserts each body hash is in its evidence document).

## Two kinds of factor 1, stated plainly

- **Direct (D):** the entry has overlay call sites inside `game_verified` / `game_evidenced` functions: **0716, 025D, 02EB**.
  This is the U3B0 basis.
- **Derived through the entry (C):** the other six are near-called only from other bodies of this cluster (table below),
  which are `unresolved` residents, so a literal reading of the 0x7F verdict calls those callers circular. These six rest on
  **reachability**: every path to them starts at a direct entry (0716, 025D or 02EB) whose factor 1 is direct and
  non-circular. This is the "t30 chain ruling" the evidence documents cite. **It is the one step I ask par-a to rule on
  rather than assume.** A chain body takes identity only with its role line (factor 2), never from the chain alone.

## The asserted caller census (all transfers into each entry)

| Body | Near sites (site in containing body) | Overlay far | Loader far | Basis |
| --- | --- | ---: | ---: | --- |
| 0716 | none | 8 | 2 | D |
| 025D | 0659 in 05CD | 5 | 0 | D |
| 02EB | 028F in 025D; **0228 in the unmodelled resident 0213** | 15 | 0 | D |
| 05CD | 0775 in 0716 | 0 | 0 | C |
| 050F | 0935 in 0716 | 0 | 0 | C |
| 06E7 | 08AD, 091D in 0716 | 0 | 0 | C |
| 0193 | 02AA in 025D; 084A in 0716 | 0 | 0 | C |
| 0105 | 01CC in 0193 | 0 | 0 | C |
| 0040 | 015F in 0105; 018C in 0175 (side ii) | 0 | 0 | C |

**New finding:** a tenth resident, `0802:[0213,025D)` (74 B, a loop that calls 02EB, then the SYSTEM write calls and
ReadKey `0899:0059` while the file is missing), is **not in the cluster, has no model, and nothing calls it** (no near, far
or overlay transfer, asserted). It is the only 02EB caller outside the cluster; it does not affect 02EB's identity (the
overlay callers carry it) but the cluster is not closed over 02EB's callers. Reported, not modelled.

## Overlay callers (direct bases), with classification

| Entry | Caller function (sites) | Classification |
| --- | --- | --- |
| 0716 | `OVL_001C_19B0` (1), `OVL_03BD_0E08` (1), `OVL_03BD_10C7` (1) | **game_verified** |
| 0716 | `OVL_0025_0341` (1), `OVL_006F_0EC4` (3), `OVL_03B9_00A2` (1) | game_evidenced |
| 025D | `OVL_0036_002D` (1) | **game_verified** |
| 025D | `OVL_006F_0AEC` (3), `OVL_006F_1602` (1) | game_evidenced |
| 02EB | `OVL_006F_0010` (1), `OVL_006F_069E` (2), `OVL_006F_0AEC` (2), `OVL_006F_1602` (6), `OVL_006F_1D58` (3), `OVL_006F_22D5` (1) | game_evidenced (**none verified**) |

None of these callers is strict-credited. Per #1195 a call site inside a `game_evidenced`/`game_verified` function
satisfies factor 1.

## The nine arguments (columns follow `reviewed_function_identities.csv`; review fields blank)

| # | id | evidence_doc | review_pr | reviewed_commit | body_sha256 | body_bytes |
| ---: | --- | --- | --- | --- | --- | ---: |
| 1 | RES_0802_0040 | docs/re/RES_0802_0040_CPU.md | | | 371aba1b1f82444f7021284640b7eed635a7d0a9f97b25333fb9723071d798cd | 197 |
| 2 | RES_0802_0105 | docs/re/RES_0802_0105_CPU.md | | | 3c9775696f677e8b4711764021cf10172ab0bfb86e4aac681f88c3533a57b842 | 112 |
| 3 | RES_0802_0193 | docs/re/RES_0802_0193_CPU.md | | | 03865df7d3a85b7cb3f9d71f8c0a7fb6a82fb8db3a0beea9568dd3cf92de11fc | 90 |
| 4 | RES_0802_06E7 | docs/re/RES_0802_06E7_CPU.md | | | ed4938d30a0049177fe39d10417d6fc40b42c851dd3f826d31d08a58a675324f | 37 |
| 5 | RES_0802_02EB | docs/re/RES_0802_02EB_CPU.md | | | 141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474 | 107 |
| 6 | RES_0802_025D | docs/re/RES_0802_025D_CPU.md | | | eeea6613012d3db99c7ad85788fdd3ff5b9888d7cc6701e9ce7a612195210dc6 | 142 |
| 7 | RES_0802_05CD | docs/re/RES_0802_05CD_CPU.md | | | d8067fb72bfede3fb56aa146e941d7bc9524cb23715f8f63e60f4918c65449b5 | 282 |
| 8 | RES_0802_050F | docs/re/RES_0802_050F_CPU.md | | | 98eb2079e2b8e02ee5cccbbfae1080308ee04142ea1d806328edfcf78e28c9b6 | 188 |
| 9 | RES_0802_0716 | docs/re/RES_0802_0716_CPU.md | | | d14db7945a8e717bc536422426c5df4d35913bdd4c5f4fa10f8a51f5103b34d2 | 564 |

Total 1,719 B (the cluster's 1,813 B less 0000/0020/0175, which are side ii).

| # | Factor 1 | Factor 2 (role line, game state) | Limit |
| ---: | --- | --- | --- |
| 1 | C: 0105 and 0175 only, both under 0193 | draws a character run on the game's display pages through INT 10h, reading the display-state record `DS:4A16..4A1E` (pages `4A1A/4A1B`, EGA flag `4A1E`, documented in the COLD_START_CODEWORD_ROW14 docs) and the attribute map `DS:25D5`, writing the Dos register record `DS:4A02` | **The closest to the refused 06FB.** Unlike 06FB it has memory operands on documented game display state; it is still a display primitive. Par-a to rule. Text-mode path only (not the EGA graphics path) |
| 2 | C: 0193 only | writes the caller's status-line message through 0040's display state | argument-borne subject; same display limit as 1 |
| 3 | C: 025D (02AA) and 0716 (084A), both direct entries | the game's status-line prompt: shows the message, waits for a key through ReadKey, clears row 24 | prompt path in text mode; ReadKey may not return (EXIT/BREAK) |
| 4 | C: 0716 only (2 sites) | closes the two FileRecs of the game's ECL*.DAX archive that 0716 opened (`ECL_BLOCK_LOAD.md`) | none beyond the chain basis |
| 5 | **D**: 15 overlay sites in `game_evidenced` `OVL_006F_*`; plus 028F in 025D | tests whether the game's data file exists, storing the IOResult copy at `DS:49CE` (`RESIDENT_DAX_SELECTOR_02EB.md`, `SLOT_A_FILE_PROBE_02EB_ABI.md`) | no `game_verified` caller; all callers are one unit (006F) |
| 6 | **D**: `game_verified` `OVL_0036_002D` (1) and `game_evidenced` `OVL_006F` (4); plus 0659 in 05CD | opens the game data file named by its caller into the caller's FileRec; prompts for the disk while `[557C]` is 0 (`RESIDENT_DAX_FILE_OPEN.md`) | the prompt path reaches 0193 (text mode) |
| 7 | C: 0716 only | opens the game's block archive (base + extension) as two FileRecs, reads the 2-byte header, positions past it (`RESIDENT_DAX_FILE_OPEN.md`, `ECL_BLOCK_LOAD.md`) | chain basis |
| 8 | C: 0716 only | expands the packed ECL script block into the buffer 0716 hands to the interpreter (`ECL_BLOCK_LOAD.md`) | **STALLED** (a hang) on a data-dependent 7Fh/80h control; chain basis |
| 9 | **D**: 3 `game_verified` + 5 `game_evidenced` overlay callers, 2 loader sites | reads the game's ECL*.DAX block archives by name and block id into the game's script buffer; 29 real blocks decode byte-exactly (`ECL_BLOCK_LOAD.md`) | composed receipt with real leaves is par-b's re-run |

## Order and dependencies

1. **0040 first** (unblocks 0175 as a `resident_runtime` member; its row is then valid on the real image).
2. 0105, 0193 (0193 composes 0175, so 0175's membership also helps its readiness, not its identity).
3. 06E7, 02EB, 025D, 05CD, 050F, then 0716, which is credited last because its readiness composes all of them.

Each acceptance is one commit per row (identity + strict + accounting + pin), per the U3B0 precedent. Landing a body also
means its callees on the path must be ready: that is a readiness question for the acceptance, not decided here.

## Reproduction

    /tmp/gate-venv/bin/python tools/overlay/res0802_identity_census.py

It prints the nine spans and hashes and exits non-zero if any census count or hash changes.
