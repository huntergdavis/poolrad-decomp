# Hybrid glue, batch 8: the remaining credited raw/bus bodies route 3 reaches (PM i21974)

**No credit change.**

Route, driver and bar are those of [HYBRID_GLUE_BATCH7.md](HYBRID_GLUE_BATCH7.md): on route 3 the global bar is
COMPLETION, not pixels. Every body is checked by V1 shadow equality on each compared arrival, and pixels are checked on
the reproducible walk regression with the same build.

## Selection

The route-3 observer logs give 44 credited bodies that route 3 reaches and that were not glued. The logs are the 8
runs `hybrid-r3-proto-g`, `b7a`..`b7e`, `seed1-a/b` and `b7d-route`.

**Entry kind.** wR:p5's `all-results.csv` covers 5 of the 44. For the rest, every header was grepped for a
`pr_*<offset>*` prototype in the right unit, and its first parameter was read: `uint8_t *ram` means raw, `pr_bus *`
means bus, an io struct means semantic.

- **29** have a plain raw or bus entry: glued here.
- **3** (`OVL_005E_263D`, `2574`, `17F6`) have raw entries whose layout binds other units' code segments (census
  class (b)). They are glued here with a stub read.
- **12** have no raw or bus entry. They are the raw-model backlog, priced to the PM in i22005 and i22006: 06D3, 0C5F,
  06FB, 0C0F, 0BE7, 18EB, 0C37, and RES_00BA 23B7, 2E03, 23DB, 2E46, 1509.

**Bindings the glue derives, relative to the SYSTEM segment, as for the existing stub guards:**

| Body | Argument | Taken from |
| --- | --- | --- |
| `OVL_006C_3291` | `resident_cs` | RES 0464 (header: "resident_cs is the segment of RES 0464"), plus SYSTEM |
| `OVL_0033_12E1` | `resident_cs` | RES 00BA (its source checks the binding to 00BA:0BB8), plus SYSTEM |
| `OVL_0025_12FB`, `OVL_001C_0111` (limits) | `system_cs` | SYSTEM |
| `OVL_0077_27DF` | RETF frame policy | SS-wrapped (pinned DOSBox-X) |

**The 263D trio stub read.** These models take U3C6's and U3C2's code segments as a layout, and they never read the
stubs. The glue checks the three stubs listed in [re/U5E_DIRECTION_263D.md](re/U5E_DIRECTION_263D.md):
- `03E0:005C` must be a loaded `EA` to +0BE7;
- `03E0:0061` must be a loaded `EA` to +0C0F, into the same segment;
- `03DB:0034` must be a loaded `EA` to +057B.

It takes `slots_cs` and `predicate_cs` from those stubs. If any check fails it declines, so nothing changes and the
original runs. Stub declines were 0 in both runs.

**These sources predate #1445.** The trio was glued against main before #1445, codex's loaded-overlay-stub guards,
which changes `overlay_u5e_direction_gate_17f6.c` and the 263D audit. #1445 was still open when this ran. Until it
lands, the stub-precondition guard lives in this glue, as for 0214 (PM i21977). Once #1445 lands, the trio is re-run
against main.

## Results

Builds: `65521ba5` (the 29) and `e4ece650` (the 29 plus the trio).
- **Route-3 V1:** `hybrid-b8a-v1-r3-b` and `hybrid-b8b-v1-r3-a`, both driver_rc 0, non-equal 0, stub declines 0.
- **Walk V1:** `hybrid-b8a-v1-walk` and `hybrid-b8b-v1-walk`, both 0 px and 0 non-equal.
- **Discarded:** `hybrid-b8a-v1-r3-a`, driver_rc 10. The prompt bar stayed blank for 120 s while the computer acted.
  V1 shadowing slows those turns, so the wait is now `COMBAT_IDLE_S`, default 300.

**Counted from route 3, V1 own / equal** (b8b, with b8a in brackets where it differs). There are 23 bodies:

| Body | b8b | | Body | b8b |
| --- | --- | --- | --- | --- |
| `OVL_005E_263D` | 154 / 154 | | `OVL_002F_122B` | 6 / 6 |
| `OVL_005E_2574` | 74 / 74 | | `OVL_0038_11F9` | 1 / 1 (b8a 0) |
| `OVL_005E_17F6` | 32 / 32 | | `OVL_0025_0F53` | 3 / 3 |
| `RES_00BA_2637` | 72 / 72 (b8a 26) | | `OVL_007F_0F7E` | 2 / 2 |
| `RES_00BA_1168` | 41 / 41 (b8a 22) | | `OVL_0025_0482` | 2 / 2 |
| `RES_0709_0714` | 21 / 21 | | `OVL_001C_0A9F` | 1 / 1 |
| `RES_00BA_2E8B` | 17 / 17 | | `OVL_001C_0111` | 1 / 1 |
| `OVL_009E_0E2E` | 10 / 10 | | `OVL_0033_12E1` | 1 / 1 |
| `OVL_0033_1577` | 12 / 12 | | `OVL_0020_0355` | 1 / 1 |
| `OVL_005E_2879` | 5 / 5 | | `OVL_001C_125D` | 1 / 1 |
| `OVL_0038_0947` | 2 / 2 | | `OVL_0025_1FB1` | 1 / 1 |
| `OVL_006C_3291` | 6 / 6 (b8a: 5 equal, plus 1 C stop at 0C0D:002F whose mid-body hand-back compared equal) | | | |

**Glued but NOT counted:**
- **`RES_0709_0DBE` and `RES_00BA_1693`: delay: not shadow-comparable** (PM i22007). They stay glued and are not
  counted. Every arrival timed out: b8a 17 and 1, b8b 39 and 6. The window is abandoned after 2M instructions. Both are
  timer-delay bodies: the original spins on the BIOS tick, and the shadow C cannot advance time. These are honest
  non-comparisons, not passes.

  **Possible later fix, not built:** feed the shadow C the BIOS tick values the original read, so that its delay loop
  ends at the same point.
- **`OVL_0033_0810`:** its one arrival per run was IRQ-interrupted.
- **0 own arrivals in both runs:** `OVL_006C_0000`, `OVL_0025_12FB`, `OVL_0025_1951`, `OVL_0033_0000`, `OVL_001C_271F`,
  `OVL_0077_27DF`.

## What this does not show

- **No credit change.**
- The limit is the one stated in batch 7: validated per arrival, which is weaker than "the final state matched".
- The single-arrival bodies rest on one compared call each.
