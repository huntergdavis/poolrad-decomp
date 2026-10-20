# Identity sweep, class (a): the media-decidable set, with callers named

**Method.** Every identity-blocked row whose queue status is `main_model`
(**52 rows**) was tested mechanically: decode every unit, take `lcall` sites whose
**segment word carries a relocation**, resolve the target through the unit
descriptor (`memory_segment` + entry slot `0x20+5i`), map the site back to its
enclosing ledger function, and ask whether that caller is game code.

**Two bars, two answers - and the difference is the point.**

| test | requirement on the caller | rows |
| --- | --- | ---: |
| **A1** (strict) | enclosing caller is **credited** (in the strict table) | **0** |
| **A1'** (the bar actually used) | enclosing caller is **game code** (`game_verified` or `game_evidenced`) | **23** |

A1 returns **zero**, which is itself a finding: no identity-blocked body in this
corpus has a *credited* caller. Every accepted credit this lane argued tonight -
including `OVL_0077_275B`, whose own ownership argument rests on a
`game_evidenced` caller - was accepted on the A1' bar. So **A1' is the operative
test**, and the earlier estimate that "about a third are creditable without
translating anything" is now measured rather than guessed: **23 of 52**.

## The 23 (class (a))

| row (identity-blocked, modelled) | first game-owned caller | unit | fixup site | callers |
| --- | --- | ---: | ---: | ---: |
| `OVL_0025_0411` | `OVL_001C_0450` | `0x1c` | `0x04aa` | 1 |
| `OVL_0025_0511` | `OVL_001C_0856` | `0x1c` | `0x090c` | 1 |
| `OVL_0025_1684` | `OVL_001C_10D1` | `0x1c` | `0x1232` | 6 |
| `OVL_0025_1788` | `OVL_001C_0F08` | `0x1c` | `0x1053` | 1 |
| `OVL_0025_1CC3` | `OVL_001C_1DB1` | `0x1c` | `0x1e1f` | 2 |
| `OVL_005E_0123` | `OVL_0031_10D7` | `0x31` | `0x1204` | 1 |
| `OVL_006F_0010` | `OVL_006C_306D` | `0x6c` | `0x308a` | 1 |
| `OVL_006F_01B8` | `OVL_006C_306D` | `0x6c` | `0x327e` | 1 |
| `OVL_006F_0482` | `OVL_006C_306D` | `0x6c` | `0x3192` | 9 |
| `OVL_006F_1262` | `OVL_001C_2B7E` | `0x1c` | `0x2b9b` | 1 |
| `OVL_006F_22D5` | `OVL_001C_0BD1` | `0x1c` | `0x0c47` | 1 |
| `OVL_0077_299C` | `OVL_005E_2414` | `0x5e` | `0x2453` | 9 |
| `OVL_009E_0026` | `OVL_001E_0249` | `0x1e` | `0x02c6` | 31 |
| `OVL_009E_02FA` | `OVL_002F_01F6` | `0x2f` | `0x022c` | 21 |
| `OVL_009E_1081` | `OVL_0086_136E` | `0x86` | `0x139d` | 9 |
| `OVL_03B0_06FB` | `OVL_0025_0341` | `0x25` | `0x035f` | 19 |
| `OVL_03B0_1140` | `OVL_001C_0F08` | `0x1c` | `0x0f73` | 6 |
| `OVL_03B0_11F3` | `OVL_001C_0F08` | `0x1c` | `0x106b` | 13 |
| `OVL_03B9_0708` | `OVL_0025_054E` | `0x25` | `0x05d5` | 2 |
| `OVL_03C2_02CC` | `OVL_0086_295B` | `0x86` | `0x2a71` | 2 |
| `OVL_03C6_0105` | `OVL_002F_0000` | `0x2f` | `0x0064` | 3 |
| `OVL_03C6_1298` | `OVL_002F_01F6` | `0x2f` | `0x0278` | 9 |
| `OVL_03CE_01C8` | `OVL_001C_0450` | `0x1c` | `0x0532` | 15 |

Each of these has a **relocated call site** in a game-owned function whose
enclosing row the ledger resolves. That is the same argument that cleared
`U33 +12E1` (four fixup-backed callers) and `OVL_0077_275B` (one).

**A2 (dispatch ownership)** applies to a subset: only `OVL_009E_0026` and
`OVL_009E_0160` carry `indirect_unresolved`. `0026` is already in the 23 above
via `OVL_001E_0249`; `0160` has no fixup-backed caller and its table ownership
has not been established.

## The remaining 29 (class (b)/(c))

No relocated call site in any game-owned function. These cannot be converted by
a sweep; they need a capture of a reachable path, or an explicit decision that
behaviour plus documented fields counts as ownership - the case
`OVL_0068_0294` illustrates (three credited callees, a complete differential, and
zero fixup-backed callers).

## Worked examples

* **(a) ordinary:** `OVL_03B0_06FB` <- `OVL_0025_0341`, unit `0x25` site
  `0x035F`, **19** relocated call sites. Caller is game code; the test is caller
  resolution alone.
* **(a) with dispatch:** `OVL_009E_0026` <- `OVL_001E_0249`, **31** call sites,
  and the row also carries `indirect_unresolved` - so it needs A1' *and* the
  table-ownership argument before its identity is settled.
* **(a) highest fan-in:** `OVL_0025_1684` <- six sites in `OVL_001C_10D1`;
  `OVL_03CE_01C8` <- 15 sites in `OVL_001C_0450`.
* **(b) counterexample:** `OVL_0068_0294` - **zero** relocated callers. Full
  evidence, no ownership evidence.

## What this converts, and what it does not

A sweep converts the **23** mechanically, in one reviewer pass, with no
translation. It does not convert the other **29**: those are behaviour-only rows,
and converting them would need a policy decision, not a test.

Nothing here decides an identity or touches a classification. The caller table is
the evidence a reviewer would cite, not the verdict.

## Reproduce

    python3 tools/overlay/identity_sweep_class_a.py --json /tmp/sweep.json

**Inputs.** `resources/strict-candidate-queue/queue.json` (derived, gitignored;
regenerate with the recipe in `STRICT_CANDIDATE_QUEUE.md` - it needs the owner
media), plus the checked-in `strict_full_c_addresses.csv`,
`overlay_classification.csv` and `overlay_function_ledger.csv`, plus
`resources/DOS/POOLRAD/{START.EXE,GAME.OVR}` read-only. The queue **digest is
printed on every run**. A digest change is **not** by itself a credit change: par-e
found that the candidate-queue digest moves with git refs (branch pushes), not only
with credits, so "comparable only at the same digest" was stricter than reality. par-e
reproduced the counts below (49 / 0 / 23 / 26) at doc digest `54e1c7c3`, at PR base
`c7526b6f` and at main `6b855b2d`. Read a different digest as "re-read the counts"
and compare the **counts and the checkpoint**; only a change in the counts is news.

**Expected output**, at queue sha256 `54e1c7c3922d…03e4c2`:

    identity-blocked modelled rows: 49
    CLASS (a) by A1  (credited caller):    0
    CLASS (a) by A1' (game-owned caller): 23
    remaining (A1' failed): 26

**These move.** The blocked set shrank from 52 to 49 in the hours between the
first run and this note as other lanes landed credits, while **A1' stayed 23** -
so the blocked count is not the number to watch; the A1' list is. Re-run after
each batch of credits and refresh the table rather than quoting 23 as a constant.

**What the script does not do:** it decides no identity, touches no
classification and credits nothing. It resolves callers; a reviewer still has to
accept the result, and A2 (dispatch-table ownership) is only flagged, never
implemented.
