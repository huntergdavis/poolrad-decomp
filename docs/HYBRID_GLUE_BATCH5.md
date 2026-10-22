# Hybrid glue, batch 5: route 2 and the 18 credited bodies it reaches (VP i21301)

## Why batch 5 needed a new route

Batch 4 glued every credited body on the save walk that has a raw-CPU or bus C entry. The 8 credited bodies the walk
still reached cannot be glued as they are:

- **5 are semantic-only.** Their C maps inputs to outputs and does not model registers, flags or stack slots:
  `OVL_03BD_047B`, `0155`, `057A`, `0350`, and `OVL_0068_068D`.
- **3 have no standalone C entry:** `OVL_0025_06D3`, `06FB` and `18EB`.

Every other credited body had 0 arrivals on any measured route, so "next by arrival count" needed a new route.

## Route 2

- **Keys.** The save walk of [HYBRID_STAGE_B_OBSERVER.md](HYBRID_STAGE_B_OBSERVER.md) to 14,4, then the game's own command
  bar, using `tools/hybrid/walk_driver.py` with `WALK_EXTRA="v Escape e Escape l s a"`. Each key waits for the prompt line
  to settle before the next. The screens are:
  - **V:** the VIEW screen, "ITEMS TRADE DROP EXIT", then Escape;
  - **E:** the CAMP menu, "SAVE VIEW MAGIC REST ALTER EXIT", then Escape;
  - **L, S, A:** LOOK, SEARCH and AREA.
- **Reproducible.** Two unsubstituted runs, `hybrid-route2-explore-a` and `-b`, reach the same 180 IDs, have pixel-identical
  final frames, and have equal credited arrival totals (4,687).
- **Encounter guard.** The driver now stops with exit 8, "route diverged", if a random-encounter prompt (COMBAT) appears.
  Such runs are not the route. One V1 run (`hybrid-b5b-v1-route2`) hit an encounter after the step north and was
  discarded.
  - That run is evidence about timing. In V1 the game only ever continues from a C state that compared equal to the
    original's, so the encounter came from timing (V1 runs slower), not from any C result. The same goes for the earlier
    kobold encounter.

**The 18 credited bodies route 2 reaches that the walk does not.** All 18 are glued here:

| Body | Route-2 arrivals | C entry |
|---|---|---|
| `OVL_0086_323D` | 28 | raw |
| `RES_00BA_2199` | 14 | raw |
| `OVL_006A_0159` | 12 | raw |
| `OVL_006A_019C` | 12 | raw (guarded, see below) |
| `OVL_0077_0901` | 6 | bus |
| `RES_00BA_11EA` | 3 | raw (chain helper) |
| `OVL_006A_0214` | 2 | raw (guarded) |
| `RES_00BA_01FB`, `0282` | 2 each | raw (chain core) |
| `RES_00BA_0000`, `03A0`, `0BB8` (chain core); `10E6`, `1282`, `1325`, `13AB` (chain helpers) | 1 each | raw |
| `RES_00BA_17E3` | 1 | bus |
| `RES_0709_0CAB` | 1 | bus |

The ten `RES_00BA` character-chain bodies run through the chain's own entry-IP dispatchers, `pr_res00ba_core_cpu` and
`pr_res00ba_helpers_cpu`.

## The numbers, from the final binary (sha256 prefix `56bb27cda5da00b0`)

**Shadow (V1), run `hybrid-b5f-v1-route2`, all 59 bodies dispatched, on route: 0 non-equal results.** The final frame has
0 differing pixels against the unsubstituted route-2 run.

| Body | Checked |
|---|---|
| `OVL_001C_0B52` (batch 3; re-checked here, now guarded) | 24 equal |
| `RES_00BA_2199` | 14 equal |
| `OVL_0077_0901` | 6 equal (device stream and state) |
| `OVL_006A_0214` | 1 equal; 1 declined by the stub guard, whose original rerun was equal |
| `RES_00BA_0BB8`, `RES_00BA_17E3` | 1 equal each |
| `RES_0709_0CAB` | 0 checked: its one arrival was interrupted |
| `323D`, `0159`, `019C`, and the other nine `RES_00BA` chain bodies | no standalone arrival: reached only inside `0214`'s or `0BB8`'s window, checked inside those |

**Route, run `hybrid-b5f-route-route2`: did not diverge.**

- **IDs:** no new ID was reached.
- **Screen:** the final frame has 0 differing pixels.
- **Returns:** 864 routed returns. The one routed stop is `1446`'s callee hand-back.
- **Stub guard:** declined 2 arrivals, and the original ran for both.
- **Uncredited counts that moved:**
  - the timing counters (`RES_0A40_02FD`, `RES_0709_0C3F`, `RPRO_08AE0`);
  - `OVL_03B9_0000`, `RES_0464_0D71` and `RES_0464_1B52`. These already differ between the two unsubstituted runs
    (411 vs 245), so they count as timing too;
  - callees now inside our C: `OVL_0025_07BC` 10 → 0, `RES_0709_00B7` 138 → 0, `RPRO_07090` 138 → 0, `RPRO_07130`
    277 → 1.

**Credited bodies' original executions on route 2 (today's 280-row list): 4,687 unsubstituted, 1,217 with all 59 glued.**

For the batch-5 bodies, every one goes to 0 except `OVL_006A_019C` and `OVL_006A_0214`, at 1 each. Those are the arrivals
the stub guard declined, where the original ran with its overlay load.

## Finding: two models assume an overlay stub is loaded

**What V1 caught.** In the first V1 run (`hybrid-b5-v1-route2`), `OVL_006A_0214` was equal on its first arrival and
`C_DIFFERS` on its second.

- The first differing byte was linear `0816E`: the C left `D8`, the original wrote `68`. That address is `PSP:002E`, where
  DOS saves the caller's SP on every `INT 21h`. So the original made DOS calls inside the window, and the C did not.
- That V1 run then diverged (195 IDs), because a RAM-only body's check continues from the C's state.

**The cause.**

- 0214 calls `+019C`. The 019C model (`call_323d`) calls the unit 0x86 `+323D` model directly, "through its loaded stub",
  and never reads the stub at `0096:003E`.
- On that arrival unit 0x86 was not resident. The original went through the overlay manager, which loaded the unit with
  DOS reads, and the C skipped that.

**The same class elsewhere.**

- An audit of every linked model source found one more: `OVL_001C_0B52`. It takes U25's CS as an argument and assumes
  stub `002B:008E` is loaded.
- `OVL_001C_00F4`, `OVL_001C_18F8` and `OVL_006F_1446` already read their stubs (`EAh` plus the expected entry).

**The glue fix.**

- `0214` and `019C` run as C only when the stub at `0096:003E` is a far `JMP` (`EA`) to `+323D`.
- `0B52` runs only when the stub at `002B:008E` is a far `JMP` to `+1EA6`, and it takes U25's CS from the stub.
- Otherwise the C declines with ARGUMENT, the original runs, and the dispatch log counts the decline.

**What it means for the corpus.**

- The models' arithmetic is not shown wrong.
- But `019C`, `0214` and `0B52` are valid only while the called unit is resident, and their own code does not check that.
- End-state differentials always run with the unit present, so they cannot see it. Reported to par-a.

## What this does not show

- **Composed only:** 12 of the 18 bodies have no standalone checked arrival on route 2. They run, and are compared, only
  inside `0214`'s and `0BB8`'s windows.
- **No checked arrival** for `RES_0709_0CAB` (interrupted).
- **Two routes, one save.** Timing is not covered: route 2 hit a random encounter in one V1 run out of three.
- **Not glued:** the 5 semantic-only and 3 entryless walk bodies.

## Build

- **Models object:** 52 entries, 95 sources.
- **Dispatch list:** the batch-4 list, plus the 18 above.
- **Route skip:** `PR_HYBRID_ROUTE_SKIP=RES_0802_0716` in route mode.
- **Driver:** `WALK_EXTRA` for route 2.

## After the rebase onto main `132e1ea1` (VP i21372)

**Same binary.** The rebased tree builds a byte-identical binary (sha256 prefix `56bb27cda5da00b0`). The models
object's 95-source closure is unchanged, and the sources main added are not in it.

**V1 re-run on the rebased tree (`hybrid-b5rb-v1-r1`):**
- on route, 180 IDs;
- final frame with 0 differing pixels;
- **0 non-equal results** (860 equal result lines);
- the stub guard declined 1 arrival.

**Route re-runs.** Four route runs on the rebased tree (`hybrid-b5rb-route-r1..r4`) all hit a random kobold encounter, so
none is quoted:
- **How each was caught:** r1 was caught only by its final frame (28,092 px), because the prompt read "PRESS ENTER TO
  CONTINUE", not COMBAT. That run showed the COMBAT check was not enough, so `WALK_EXPECT` now requires each extra step to
  settle on its expected screen, and r2 to r4 stopped at LOOK.
- **Why this is timing, not a C result:**
  - The same binary ran route 2 on route before the rebase (`hybrid-b5f-route-route2`), so the route numbers above stand
    for these bytes.
  - V1, which compares every arrival, passed on route.
  - A wrong C result would diverge every time; these runs diverge only sometimes. The encounter roll depends on timing,
    and route mode's C takes no emulated time.
