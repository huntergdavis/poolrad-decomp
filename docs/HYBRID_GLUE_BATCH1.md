# Hybrid glue, batch 1: all 16 credited bodies the cold start reaches now run as our C (VP i20767)

The owner's standing rule (VP i20767) is to glue every credited body. This batch adds the four `RES_0802` bodies that the
cold start reaches but that were not glued yet: 025D, 050F, 05CD and 06E7.

- **16 of the 16 credited bodies the cold start reaches now run as our C: 11 C-only, 5 shadow.**
- The route is the stage (b) save walk ([HYBRID_STAGE_B_OBSERVER.md](HYBRID_STAGE_B_OBSERVER.md)), which starts with
  that cold start.
- **Not covered:** this is not "every credited body". 261 credited bodies are not on the cold start, and the walk
  reaches 32 of them; none of those 32 are glued yet.

## The numbers

**Shadow (V1), run `hybrid-sixteen-v1-walk-b`: every checked arrival of all 16 bodies is equal, with 0 differences.**
Each arrival runs the original first and then our C from the same snapshot. Every arrival compares the registers, the
flags and all 1 MiB; bus bodies also compare their device stream, and service bodies compare their DOS and BIOS calls.

| Body | Kind | Checked equal | Excluded (named) |
|---|---|---|---|
| `RES_0802_025D` | DOS open/close replay | 17 | 1 interrupted |
| `RES_0802_050F` | RAM-only, arrival budget 2^24 | 62 | 9 interrupted |
| `RES_0802_05CD` | DOS open/close/read/seek replay | 71 | — |
| `RES_0802_06E7` | DOS close replay | 71 | — |

The other 12 bodies are unchanged from stage (a), with 0 differences:

| Body | Result |
|---|---|
| `RES_0899_0104` | 101 equal, plus 11 forced-argument reruns, all equal |
| `RES_0464_2595` | 16 equal, plus 15 mid-body hand-backs, all equal |
| `RES_0709_0626` | 87 equal |
| `RES_0802_02EB` | 35 equal |
| `RES_03F1_0137` | 24 equal |
| `RES_0709_05EB` | 5 equal |
| `OVL_03B0_01DA` | 4 equal |
| `RES_0899_0059` | 4 equal |
| `OVL_03B0_00CB` | 4 equal |
| `RES_0709_01DF` | 3 equal |
| `OVL_0086_3413` | 1 equal |
| `OVL_0038_31A2` | 1 equal |

Interrupted arrivals of these bodies are excluded and named: `RES_0709_0626` 9, `RES_03F1_0137` 2, `RES_0709_05EB` 2,
`OVL_03B0_01DA` 2 and `RES_0802_02EB` 1.

One arrival is excluded as a TIMEOUT: `RES_0899_0059`'s last ReadKey, after Up, which waits for a key that never comes.
The window was abandoned after 2,000,000 instructions, and the original was kept.

**Route, run `hybrid-sixteen-route-walk-b`: the original's executions, body by body.**

| Body | Mode | Arrivals | Our C returned | Original executed |
|---|---|---|---|---|
| `RES_0709_01DF` (glyph) | C-only, real VGA | 1,161 unsubstituted → 3 | 3 | **0** (was 1,161) |
| `RES_0899_0104` | C-only | 112 | 112 | **0** |
| `RES_0709_0626` | C-only, real VGA | 96 | 96 | **0** |
| `RES_0802_050F` | C-only | 71 | 71 | **0** |
| `RES_0464_2595` | C-only | 31 | 31 | **0** |
| `RES_03F1_0137` | C-only, real VGA | 26 | 26 | **0** |
| `RES_0709_05EB` | C-only, real VGA | 7 | 7 | **0** |
| `OVL_03B0_01DA` | C-only, real VGA | 6 | 6 | **0** |
| `OVL_03B0_00CB` | C-only | 4 | 4 | **0** |
| `OVL_0086_3413` | C-only | 1 | 1 | **0** |
| `OVL_0038_31A2` | C-only | 1 | 1 | **0** |
| `RES_0802_02EB` | shadow, DOS replay | 125 | 35 checked equal | 125 (by design) |
| `RES_0802_025D` | shadow, DOS replay | 89 | 18 checked equal | 89 (by design) |
| `RES_0802_05CD` | shadow, DOS replay | 71 | 71 checked equal | 71 (by design) |
| `RES_0802_06E7` | shadow, DOS replay | 71 | 71 checked equal | 71 (by design) |
| `RES_0899_0059` | shadow, BIOS replay | 5 | 4 checked equal | 5 (by design) |

How to read the table:

- **Arrivals count calls made inside another shadow body's window.** For example, the 025D calls that 05CD makes are
  original executions inside 05CD's window. A shadow body's checked count can therefore be smaller than its arrivals.
- **"Original executed" is arrivals minus our C's routed returns,** from the dispatch summary. The observer counts at
  the entry, *before* the C runs, so a C-only body's own observer count does not fall. The glyph routine's does fall,
  because its calls are made from inside the composed C of 0626, 05EB and 01DA.

**Credited bodies' original executions on the save walk: 2,453 unsubstituted, 937 here (−62%).**

- **Where the drop comes from (1,516 executions):**
  - 1,161 are the glyph routine;
  - 355 are the other ten C-only bodies.
- **Where the 937 come from:**
  - 361 are the five shadow bodies, which by design still run the original;
  - 576 are credited bodies not glued yet.

**Composed callees that are not credited also stopped running.** Each runs inside a credited body's C now:

| Callee | Unsubstituted | Route |
|---|---|---|
| `RES_0709_00B7` (rect fill) | 54 | 2 |
| `RPRO_07090` | 54 | 2 |
| `RPRO_07130` | 109 | 5 |

**The rest of the game:**

- **Path.** The route run reached the same 149 IDs as the unsubstituted walk. The only counts that moved, other than
  the glued bodies and their composed callees, were three timing-driven ones:
  - `RES_0A40_02FD`, the KeyPressed poll;
  - `RES_0709_0C3F`, the clock read inside the credits wait;
  - `RPRO_08AE0`.

  02FD and 08AE0 already differ between the two unsubstituted walks c and d (see the stage (b) doc). 0C3F is 131,758 in
  both of those and 132,161 here (+403).
- **Screen.** The final frame (14,4 W 10:51) has **0 differing pixels** against the unsubstituted walk c.

## What went wrong on the way (kept, per the standing rule)

**My glue bug, caught by the comparison.**

- **Symptom:** in the first V1 run (`hybrid-sixteen-v1-walk`), `RES_0802_05CD` stopped on all 70 checked arrivals. Its
  fifth DOS call, a Seek, asked for an offset that was usually 2 higher than the original's (`DX=00EE` against `00EC`).
- **Cause:** it was the glue, not the model. BlockRead (`0AF8:14FB`) copies the destination into a *staging* buffer,
  hands that buffer to the DOS service, and then writes the buffer back over RAM. My read callback applied the recorded
  RAM delta but left the staging buffer stale. The write-back then restored the old header bytes, and the C computed its
  seek from them.
- **Fix:** the callback now fills the staging buffer from the replayed RAM. After the fix the result is 71 of 71 equal.
- **Two lessons:**
  - The service comparison can fail on 05CD, and it caught a real wrong value 70 times.
  - Rule out the glue before calling a shadow difference a model defect.

**A route run diverged once, and the same configuration did not diverge again.**

- **What happened:** in the first 16-body route run (`hybrid-sixteen-route-walk`), the step north produced a random
  encounter ("You have surprised a party of kobolds"). The final frame differed by 34,888 pixels, and 170 IDs were
  reached instead of 149.
- **The repeat did not diverge:** the identical configuration (`-b`) reached 14,4 with the same 149 IDs and 0 differing
  pixels. So did a 12-body route run (`hybrid-twelve-route-walk`) and a 0104-only route run (`hybrid-route-0104-walk-b`).
- **Where the first run's arrival order first split:** inside `RES_0709_0C3F`, the clock read of a timed wait loop. In
  route mode our C takes no emulated time, so wait loops spin a different number of times.
- **Reading:**
  - One configuration gave two outcomes, so the encounter roll depends on run timing, not on a body's result.
  - This is the "timing is not covered" limit showing up in play. It is **not proven** that no body is involved, because
  route arrivals have no per-arrival check. The V1 runs, which do check every arrival, never diverged.

**One unexplained stall.** A 0104-only route run (`hybrid-route-0104-walk`) sat on the credits screen for its whole
400 s budget, and its rerun reached 14,4 in 101 s.

- `RES_0899_0104` is a 30-byte routine that executes 7 instructions from its entry.
- The stall did not reproduce.

## What this does not show

- **Shadow is not authority.** For the five service bodies the original still runs, and the game would behave the same
  if their C were wrong. Only the equality log says the C agreed. The hybrid still cannot drive DOS itself.
- **No per-body mutant for 025D, 050F or 06E7.** The comparison code is the same as for the bodies that have mutants,
  and 05CD's comparison failed for real (see above).
- **One route, one save, one step.** Timing is not covered, and it measurably changes random outcomes (see above).
- **Batch 1 of the standing rule:**
  - 32 credited bodies on the walk are still unglued, and 9 of them are semantic-only models that need an adapter first;
  - 229 credited bodies are on no measured route;
  - 71 credited IDs have no C entry symbol my resolver can find.

## Build and run

- **Models object:**
  `python3 tools/hybrid/build_models.py --entries <the 16 entry symbols>`. It links the closure of the entry symbols
  (49 sources) from a per-source compile cache. On the 12 stage (a) entries it reproduces the hand-built 41-source object
  exactly.
- **Patch:** the cumulative `tools/hybrid/dosbox-x-5a9c7c53-dispatch.patch`. It reverse-applies to the tree that ran;
  the binary's sha256 prefix is `8c0e8dde5dffa1c4`.
- **Compile-time check:** every model CPU type the dispatch casts to the 14-word layout has a `static_assert`, so a
  mismatched layout fails the build.
- **Runs:**
  `tools/hybrid/run_observer_walk.sh NAME resources/hybrid/registry_all.tsv 400 2000 walk`, with:

  ```
  PR_HYBRID_DISPATCH=RES_0899_0104,RES_0464_2595,RES_0709_01DF,RES_0709_05EB,RES_0709_0626,RES_03F1_0137,OVL_03B0_01DA,OVL_0086_3413,OVL_0038_31A2,OVL_03B0_00CB,RES_0802_02EB,RES_0899_0059,RES_0802_025D,RES_0802_050F,RES_0802_05CD,RES_0802_06E7
  ```

  Add `PR_HYBRID_BUS_MODE=route` for route mode.
