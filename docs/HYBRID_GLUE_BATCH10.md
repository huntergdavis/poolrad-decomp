# Hybrid glue, batch 10: the R1 route's 14 bodies (PM i22647)

codex's R1 route (#1517, `tools/hybrid/R1_ROUTE.md`: loot Pool/Share, then View Items) reaches 14 credited bodies that no
earlier route reached. All 14 are glued, with `pop 0` (the return is detected dynamically):

| Unit | Bodies |
| --- | --- |
| `OVL_007F` | `0000`, `003A`, `0054` (bus), `0508`, `05E0` |
| `OVL_03B0` | `0000`, `0067`, `0711`, `07A4` (bus), `08D7`, `0923`, `0969` (bus), `0B03`, `0CC5` |

**Build:** `76f56e4d`, on main, with 133 model entries and 144 dispatch rows.

**Harness:** `tools/hybrid/run_r1.sh` gains a `dispatch` mode. `R1_DISPATCH` names a file with the comma-separated body
list, which is copied into the run and hashed. `R1_BUS_MODE` unset means V1; `R1_BUS_MODE=route` means route mode.
`r1_report.py` counts arrivals in dispatch runs as it does in observer runs. The other modes are unchanged, and dispatch
mode refuses `R1_DISPATCH` or `R1_BUS_MODE` without a list.

**Both runs are pinned** (`12345678`) and compared with codex's all-original pinned R1 pair, `r1-original-c` and `-d`
(final RGBA `0dc77c31…`, raw-RGB `7fb30554…`). **They are test-harness runs.** Both used the R1 driver as it was
before #1526, which is the driver the reference was made with.

## V1, `hybrid-b10-r1-v1`

R1 driver success. Non-equal 0, stub declines 0. Final frame equal to the reference, and all 1,447 post-pin RandSeed
draws equal (`ad42d362`).

| Body | V1 equal / compared |
| --- | --- |
| `OVL_007F_003A` | 47 / 47 |
| `OVL_007F_0054` | 45 / 45 (+2 INTERRUPTED) |
| `OVL_007F_0508`, `OVL_007F_05E0`, `OVL_03B0_0067`, `OVL_03B0_0969`, `OVL_03B0_0CC5` | 1 / 1 each |
| `OVL_03B0_07A4` | 0 compared: its one arrival was INTERRUPTED |

**0 own arrivals:** `OVL_007F_0000` (composed inside `0054`), `OVL_03B0_0000` (inside `0B03`/`0CC5`), `0711`
(inside `07A4`), `08D7`/`0923` (inside `0969`), and `0B03` (inside `0CC5`). They run as C inside their glued parents.
They are not claimed from their own arrivals.

## Route mode, `hybrid-b10-r1-route`

R1 driver success. 27,613 routed returns. The final frame is **0 of 786,432 px** from the reference. The 60 visible
post-pin draws equal the reference prefix; the other draws are made inside routed C.

**Routed:** the eight bodies above with own arrivals (47 + 47 + 1 × 6), plus `OVL_0077_0901` (6) and `RES_0709_0CAB`
(2), which R1 routes for the first time. #1531 adds them to [re/replaced_bodies.json](re/replaced_bodies.json).

**Two arrivals were handed back to the original:**
- `OVL_006F_1446`, the known cold overlay stub (i22495).
- **New:** `OVL_001C_00F4` arrival 122, stopped at `34DD:00FA`, before its LCALL through the stub to unit 0x25
  `+0000`. The model checks the stub before the call (`PR_U1C_GOTO_CODE`) or stops in the callee. A cold, unloaded
  unit 0x25 after loot/Items is the likely cause, the same class as 1446, but it is **not proven**: the dispatch logs
  only a generic stop. The PM's suggested diagnostic, logging the raw status and the stub bytes on hand-back, would
  settle it.
