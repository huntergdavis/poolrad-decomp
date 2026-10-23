# Hybrid glue, batch 7: route 3 (combat) and the credited bodies it reaches (PM i21589)

## Route 3

The walk and route 2 never reach combat, and random encounters kept cutting route 2 short (exit 8). Route 3 makes the
encounter the route itself.

**Keys** are sent by `tools/hybrid/walk_driver.py` with `WALK_COMBAT=1`, starting from the save walk at 14,4:

1. **Seek.** Pace Down, Up, Down, … until an encounter. The budget is `SEEK_STEPS` steps, default 60; exit 9 means no
   encounter came.
2. **Start.** The encounter menu, `COMBAT WAIT FLEE PARLAY`, gets `c`. An ambush skips that menu: `BATTLE BEGINS...`
   is followed directly by the combat screen.
3. **Fight.** Each character's combat menu, `MOVE VIEW AIM USE CAST TURN QUICK DONE`, gets `q`, so the computer
   fights for that character.
4. **End.** `CONTINUE BATTLE: YES NO` gets `n`. The game asks this when no enemy is left in reach. In
   `hybrid-r3-proto-e`, answering `y` left the party guarding for ever, and that run was discarded.
5. **Aftermath.** `PRESS ENTER` gets Return. The treasure menu `VIEW TAKE POOL SHARE EXIT` gets `e`.
   `THERE IS STILL TREASURE LEFT. ... GO BACK AND CLAIM YOUR TREASURE? YES NO` gets `n`.
6. **Done.** The area menu `AREA CAST VIEW ENCAMP SEARCH LOOK` ends the route.

**Every key waits for its screen.** The screen is the OCR'd prompt line read twice the same. Any other settled prompt
stops the run with exit 10, so the route never presses a key for a screen it does not know.

The empty prompt bar while monsters act, which OCR reads as noise, is waited out, up to 120 s on one screen. The combat
menu's two-colour hot letters OCR badly (`NHOVE WIEN AIN USE adich DOE`), so the fragments that survive are matched.

**The screens were learned from six observer-only prototypes** (`hybrid-r3-proto-a`..`g`). Prototype `g` completed:
encounter at seek step 4, six Quick presses, back on the area menu. Each earlier prototype stopped, by design, on a
screen it did not yet know.

**Route 3 is not reproducible from run to run.** The encounter comes at a different seek step, the fight follows the
game's timer-seeded dice, and the post-combat screen shows party hit points. So the pixel-identical final-frame bar is
measured on the **walk** with the same build, which is reproducible: 0 px in every build below. Route 3 runs count only
when the route completes (driver_rc 0); a run that stops on an unknown screen or finds no encounter is discarded.
Choosing between this and making route 3 deterministic was put to the PM (i21644).

## Reach (observer-only `hybrid-r3-proto-g`)

- **126 of the 281 credited bodies are reached; 59 of them were already glued.**
- **67 credited bodies are reached and were not glued.** Ranked by arrivals, the 24 with a raw-CPU or bus C entry at
  the top of that list are glued here.
- **Correction.** My first entry survey missed five raw entries: 0DE3's full-state composer, 071A, 04BE, 06A7 and
  0CB9. Searching every header for the body's offset found them. They are group 3 below.

## Glued (24), all shadow-equal on route 3; one (0E52) failed and is pulled

Builds: group 1 `d0953359`, group 2 `203a380e`, group 3 `cd314c3d`. The V1 counts are from route 3 with the final
build (`hybrid-b7c-v1-r3`) unless marked otherwise.

| Body | C entry | V1 c_equal / compared | Route-mode returns (`hybrid-b7d-route-r3-a`) |
| --- | --- | --- | --- |
| `OVL_03C2_057B` | raw | 831/831 (b7a: 4,509/4,509) | 345 |
| `OVL_0033_0221` | raw | 0 arrivals this run; **b7a: 2,282/2,282** | 0 |
| `OVL_03C2_01A0` | raw | 7/7 | 5 |
| `OVL_0033_012B` | raw, reads its 0x3BD stubs | **0 own arrivals in any run**: every call came inside `01AD`, which composes it | 0 |
| `OVL_03C2_0422` | raw, composed line check | 29/29 (b7a: 1,257/1,257) | 18 |
| `OVL_03C6_06E9` | raw | 533/533 | 219 |
| `OVL_0033_01AD` | raw | 206/206 (+1 IRQ-interrupted) | 207 |
| `OVL_0033_0615` | raw | 64/64 (+1) | 65 |
| `OVL_0033_040F` | raw | 65/65 | 65 |
| `OVL_0033_0377` | raw | 65/65 | 65 |
| `OVL_0033_02BA` | raw | 65/65 | 65 |
| `OVL_03C2_002E` | raw | **0 own arrivals in any run**: every call came inside `0909` | 0 |
| `OVL_03C6_03A0` | raw, takes SYSTEM CS | 47/47 (+3) | 26 |
| `OVL_0031_128D` | raw | 22/22 | 14 |
| `OVL_0033_1434` | raw | 42/42 | 42 |
| `OVL_0031_0675` | raw, reads its 03E0:0070 stub | 74/74 | 12 |
| `OVL_002F_0131` | raw | 38/38 | 17 |
| `OVL_03C6_0068` | raw | 25/25 | 16 |
| `OVL_03C2_0909` | raw; **stub-guarded in the glue** | 159/159 (+41) | 69 |
| `OVL_009E_0DE3` | raw full-state composer | 928/928 | 841 |
| `OVL_03C6_071A` | raw | 766/766 (+2) | 350 |
| `OVL_03C6_04BE` | raw, SS-wrapped RETF policy | 404/404 | 174 |
| `RES_0709_06A7` | bus | 315/315 (+11) | 167 |
| `OVL_03C6_0CB9` | raw, SS-wrapped RETF policy | 60/60 (+1) | 22 |
| **`OVL_005E_0E52`** | raw | **0/269: all 269 differ in AF only. STOP reported (i21765); pulled from the dispatch list** | — |

**The 0909 stub guard.** `0909` far-calls U3C6 +0000 through stub `03E0:0025` and takes U3C6's segment as an argument,
`pair_cs`. It is census class (c), on par-e's i21391 list. The glue runs the C only when that stub is a loaded far `JMP`
to +0000, and it takes `pair_cs` from the stub. Stub declines: 0 in every run.

**The 0E52 defect.** The body ends with `SHR AX`. The model clears AF there, but pinned DOSBox-X sets AF for a
non-zero count, the #1433 rule. The flags reach the `RETF`, so every arrival differs. This is a credited-model defect.
Per i21416 the fix belongs to the flag bundle, so 0E52 stays pulled until it lands.

**Totals, final build `cd314c3d`.** In all three runs: non-equal 0 (0E52 pulled), stub declines 0.
- **Route-3 V1:** driver_rc 0.
- **Route-3 route mode:** driver_rc 0, 7,522 routed returns, 1 routed stop (`OVL_006F_1446`, handed back at
  08D0:0025, as in earlier batches).
- **Walk V1:** driver_rc 0, 0 px.

**Two notes on runs:**
- **The earlier route-mode attempt with build `203a380e` stalled on the credits screen** (`hybrid-b7b-route-r3`,
  driver_rc 6). By then only the RES_0802 file bodies had routed, all `SERVICES_REPLAYED_STATE_EQUAL`. No batch-7 body
  arrives before that screen: the walk shows 0 of them. The stall did not reproduce in the two later route-mode runs,
  which passed the code gate. **The cause is not known.**
- **`01A0` and `0422` declare "SHL AF chosen zero".** No divergence appeared in 7 + 29 compared arrivals here, or 5 +
  1,257 in b7a, so that AF is dead on these paths. This is not a general claim.

## Reached credited bodies NOT glued (PM i21589: listed separately, with arrivals from `hybrid-r3-proto-g`)

**No raw-CPU or bus C entry** (semantic-only, or no standalone entry), for pricing a raw model:

| Body | Arrivals | What exists |
| --- | --- | --- |
| `OVL_0025_06D3` | 1,433 | no standalone entry (batch 5) |
| `OVL_03C6_0C5F` | 1,283 | no standalone entry; modelled only inside the `263D` chain |
| `OVL_0025_06FB` | 361 | no standalone entry (batch 5) |
| `RES_00BA_23B7` | 345 | semantic (`resident_character_field_zero.h`) |
| `OVL_03C6_0C0F` | 335 | none found |
| `OVL_03C6_0BE7` | 335 | none found |
| `OVL_0025_18EB` | 134 | no standalone entry (batch 5) |
| `RES_00BA_2E03` | 84 | semantic (`resident_character_link_check.h`) |
| `OVL_03C6_0C37` | 84 | none found |
| `RES_00BA_1168` | 20 | none found |
| `RES_00BA_2E46` | 7 | semantic (`resident_character_link_flags.h`) |
| `RES_00BA_23DB` | 5 | semantic (`resident_party_count.h`) |
| `RES_00BA_1509` | 1 | semantic (`resident_sidecar_remove.h`) |

**Raw entry exists but not glued in this batch** (below the top 24, or needing a layout binding):
- `OVL_005E_263D` 76, `2574` 34 and `17F6` 14: their layout argument binds other units' segments (census class (b)),
  so each needs a stub-read glue first.
- `RES_00BA_2637` 26, `RES_0709_0DBE` 24, `OVL_0033_1577` 12, `RES_0709_0714` 12.
- `OVL_009E_0E2E`, `OVL_006C_3291`, `OVL_006C_0000` and `OVL_002F_122B` at 6 each; `OVL_0025_12FB` 5.
- `RES_00BA_1693`, `OVL_005E_2879` and `OVL_0025_0F53` at 3 each; `OVL_007F_0F7E` and `OVL_0025_0482` at 2 each.
- `OVL_0038_11F9`, `0947`, `OVL_0033_12E1`, `0810`, `0000`, `OVL_0025_1FB1`, `1951`, `OVL_0020_0355` and
  `OVL_001C_125D`, `0A9F`, `0111` at 1 each.

**`RES_00BA_2E8B`** (13) has `resident_character_target_select.h`. Whether that header is a raw-CPU entry is
unverified.

## What this does not show

- **No credit change.** Gluing proves only the bodies' equality on the arrivals counted above.
- **012B and 002E were not compared standalone.** Every arrival came inside a glued parent that composes them.
- **The pixel bar is not measured on route 3**; see the route-3 section.
