# Hybrid glue, batch 11: the R2 route's bodies, and the hand-back diagnostic (PM i22823, i22862)

codex's R2 route (#1526, `tools/hybrid/R2_ROUTE.md`: camp Magic, Rest, Order) reaches 11 credited bodies that were
not glued. **All 11 are glued**, with `pop 0`:

| Group | Bodies |
| --- | --- |
| The six Rest/Order bodies | `OVL_006A_14A9`, `OVL_006A_1601`, `OVL_007D_03A3`, `OVL_007D_047A`, `OVL_007D_0565`, `OVL_007D_05F0` (bus) |
| R1-shared candidates that R1 never reached | `OVL_0086_028D`, `OVL_0086_059C`, `OVL_03B0_0A08` (bus), `OVL_03B0_0C33` (bus), `OVL_03B0_11F3` |

**Build:** `3b75bd03`, with 147 model entries and 158 dispatch rows. **Runs:** `run_r1.sh` dispatch mode with
`WALK_ROUTE=r2`, pinned at `12345678`, compared with codex's all-original pinned R2 pair, `r2-original-a` and `-b`
(RGBA `4474ef30…`, raw-RGB `d612a49e…`). These runs use the current shared driver, which is the driver the reference
was made with. **They are test-harness runs.**

## The hand-back diagnostic

Every model call now records its **raw status** (`last_raw`). Every hand-back line (`ROUTED_STOP_HANDED_BACK`,
`C_STOPPED_HANDING_BACK`) appends:
- `raw=<status>`;
- the five code bytes at the stop;
- for a far call (`9A`), the first three bytes at its target.

The target bytes show whether the stub is loaded (`EA`, a far jump) or cold (`CD 3F`, INT 3Fh).

**First result:** `OVL_006F_1446`'s hand-back logged `stop at 08D0:0025 raw=2 code=CD 3F 00 00 00`. The model
stopped with `PR_U6F_INSERT_CPU_CODE` at an **INT 3Fh overlay stub**. The "cold stub" reading of i22495 is now
**measured**. The `OVL_001C_00F4` hand-back (batch 10) did not recur on R2, so it is still unmeasured.

## V1, `hybrid-b11-r2-v1`

Driver success. Non-equal 0. Final frame equal to the reference. The single post-pin draw is equal, and R2 makes no
later draw.

| Body | V1 equal / compared |
| --- | --- |
| `03B0_0A08` | 6 / 6 |
| `86_059C` | 3 / 3 |
| `7D_05F0` | 3 / 3 (+1 INTERRUPTED) |
| `7D_03A3`, `03B0_11F3` | 2 / 2 |
| `6A_14A9`, `6A_1601`, `7D_047A` | 1 / 1 |
| `03B0_0C33` | 0 compared: both arrivals were INTERRUPTED |

**0 own arrivals:** `7D_0565` and `86_028D`. They run composed inside their glued parents and are not claimed.

The service body `RES_0802_0716` shows `C_STOPPED_SHADOW status 9`. It is shadow-only by design: the DOS-service replay
keeps the original's result, and it does not drive the game.

## Route mode, `hybrid-b11-r2-route`

Driver success. 1,121 routed returns, 1 hand-back (1446, as above). The final frame is **0 of 786,432 px** from the
reference.

**Routed:** the nine bodies above with own arrivals, plus `OVL_006A_0214` (2) and `OVL_0077_27DF` (1). Both were glued
earlier, and R2 is the first route to run them. #1538 adds all 11 to
[re/replaced_bodies.json](re/replaced_bodies.json) (101 → 112).
