# Hybrid executor, stage (b): observer on the save-walk route (VP i20735)

Observer only: no body is substituted (`PR_HYBRID_DISPATCH` unset). The numbers come first. The claims, and what they do
not cover, are in their own sections.

## The numbers

**Headline (acceptance item 4).**

- **Credited bodies not reached by the cold start: 261** (277 credited, 16 reached by the cold start).
  - VP i20735 said 258. That was 270 − 12 from stage 0's registry, which was a stale export: `strict_full_c_addresses.csv`
    now has 277 rows. The 7 new rows are all `RES_0802_*`, and the cold start reaches 4 of them.
- **Of those 261, the save walk reaches 32.** Hottest first: `OVL_0025_06D3` 139, `OVL_03BD_047B` 109,
  `OVL_0025_0136` 54, `OVL_0025_0000` 50, `OVL_0025_06FB` 37, `RES_00BA_1037` 30. The full list is below.
- **The walk also reaches 51 uncredited function starts the cold start does not.**

**Top ten by arrival count on the walk, any credit status:**

| Rank | ID | Walk c | Walk d | Cold start | Credit status | Prior work in the repo |
|---|---|---|---|---|---|---|
| 1 | `RES_0A40_02FD` | 11,151,356 | 8,817,672 | 4,992,480 | **not credited** | Crt.KeyPressed (`INT 16h` AH=1). A declared boundary, "NEVER a credit". C exists, compose-only (`CRT_KEYPRESSED_02FD_CPU.md`) |
| 2 | `RES_0709_0C3F` | 131,758 | 131,758 | 131,752 | **not credited** | no C found |
| 3 | `RNEAR_0A6BD` | 14,832 | 14,832 | 14,832 | **not credited** | near-only start, ledger class `linked_crt_inferred`; no C found |
| 4 | `RPRO_04640` (`RES_0464_0000`) | 7,016 | 7,016 | 2,480 | **not credited** | near-only identity work (`NEAR_ONLY_GAME_SEGMENT_IDENTITY.md`) |
| 5 | `RPRO_08AE0` (`RES_08AE_0000`) | 1,578 | 1,745 | 0 | **not credited** | DRIVER_INTERNAL, "NEVER a credit". C exists (`RES_08AE_TIME_0000_CPU.md`) |
| 6 | `OVL_040C_015D` | 1,320 | 1,320 | 452 | **not credited** | 336 B, held under `RES_03F1_0000` (`STRICT_CANDIDATE_QUEUE.md`) |
| 7 | `RES_0709_01DF` | 1,161 | 1,160 | 824 | credited | glyph routine. Already glued (stage (a)) |
| 8 | `RES_0464_1951` | 782 | 782 | 454 | **not credited** | blit. C exists, "readiness work, not credit" (`RES_0464_BLIT_1951_CPU.md`) |
| 9 | `RES_0464_08D4` | 540 | 540 | 0 | **not credited** | listed as unready (1,181 B) in `STRICT_CANDIDATE_QUEUE.md` |
| 10 | `RES_0802_0356` | 192 | 192 | 0 | **not credited** | C exists (`RESIDENT_STRING_0356.md`); credit subject to review |

The hottest credited body is the glyph routine, at rank 7, and it is already glued. The hottest credited body that is not
yet glued is `OVL_0025_06D3`, at rank 11 overall (139 arrivals). It is walk-only.

**Per-ID counts and totals (acceptance item 1).**

| Run | Route | IDs reached (credited) | Arrivals | Credited arrivals | Instructions (real mode) |
|---|---|---|---|---|---|
| `hybrid-stage-b-cold-c` | cold start to the main menu | 66 (16) | 5,143,851 | 1,109 | 264,912,391 |
| `hybrid-stage-b-walk-c` | the save walk | 149 (48) | 11,313,021 | 2,453 | 468,071,763 |
| `hybrid-stage-b-walk-d` | the save walk, repeated | 149 (48) | 8,979,503 | 2,452 | 403,024,224 |

The per-ID table for all 149 reached IDs is written by `tools/hybrid/stage_b_report.py --tsv` (under the ignored
`resources/oracle-runs/hybrid-stage-b-walk-c/stage_b_counts.tsv`, because the registry it is joined to holds game bytes).

**Repeatability, measured.** The walk was run twice (c and d):

- The same 149 IDs were reached.
- **146 of the 149 have identical counts.**
- The 3 that differ:
  - `RES_0A40_02FD`: 11,151,356 against 8,817,672;
  - `RPRO_08AE0`: 1,578 against 1,745;
  - `RES_0709_01DF`: 1,161 against 1,160.

## The route, exactly (acceptance item 2)

- **Build:** DOSBox-X `5a9c7c53` with the cumulative patch `tools/hybrid/dosbox-x-5a9c7c53-dispatch.patch`. The binary's
  sha256 is `4813e8a3570db7a549198594649c89a9aa26955f248e53afb7c31ad960b4bf4c` (the stage (a) build), run with
  `cpu core=normal` and dispatch unset.
- **Repo:** `1576a2cf` plus this PR's tools.
- **Media:** the audited v1.3 game, staged per run by `tools/oracle/prepare_run.sh`, which checks the input hash list.
- **Save:** slot **A** of the game's own *Load saved game* menu, which offers "A J". These are the slot A files (sha256,
  first 16 hex digits):

  | File | sha256 |
  |---|---|
  | `SAVGAMA.DAT` | `abca248371287c43` |
  | `CHRDATA1.SAV` | `d7ba1321e1fca846` |
  | `CHRDATA2.SAV` | `6e82edf77f8c213f` |
  | `CHRDATA3.SAV` | `0dd9737e34c75c0a` |
  | `CHRDATA4.SAV` | `a8596f9be05c439a` |
  | `CHRDATA5.SAV` | `0dd24d3201adc5d5` |
  | `CHRDATA6.SAV` | `51a2a026381993f4` |
  | `CHRDATA1.ITM` | `a1927b99e61de81f` |
  | `CHRDATA2.ITM` | `1068419d97cae13f` |
  | `CHRDATA3.ITM` | `a069154daf5870f1` |
  | `CHRDATA4.ITM` | `2583a54045d43f64` |
  | `CHRDATA5.ITM` | `caf8e0db328867a8` |
  | `CHRDATA6.ITM` | `ab5c48038c4000a6` |
  | `CHRDATA1.SPC` | `5ad1965e13b304ef` |
  | `CHRDATA2.SPC` | `dca13ba560aa8186` |
  | `CHRDATA3.SPC` | `61f7d673eb99a328` |
  | `CHRDATA6.SPC` | `6aee25f3773bc03c` |

- **Keys:** the keys of `tools/oracle/capture_save_walk.sh`, each sent by `xdotool` only once its screen shows. The
  prompt line is read by OCR (`tools/hybrid/walk_driver.py`):

  | Screen shown | Key sent |
  |---|---|
  | `INPUT THE CODE WORD` | **Return** |
  | `CHOOSE A FUNCTION` | **L** |
  | `LOAD WHICH GAME` | **A** |
  | `15,4` on screen | **Up** |
  | `14,4` on screen | stop |

- **Position reached:** **14,4 facing W, game clock 10:51**, from 15,4. This is read from the final frame, not only from OCR.
  Each run keeps every polled frame under `frames/`.
- **Stop:** 3 s after 14,4. DOSBox-X's Ctrl+F9 did nothing in any of the three runs that tried it, so the driver's logged fallback, SIGTERM to the
  emulator's PID, ended each run in under 1 s. The report is the observer's exit report: each arrival total is not a
  multiple of 4096, the interval of the periodic rewrite. No emulator was left running.
- **The cold start** is the same driver stopped at the main-menu gate. It is the stage 0 path (Return on the code-word
  prompt).

## Named exclusions (acceptance item 3)

Nothing is folded into a total. Each class below is either counted in full, or listed by ID.

- **Unobservable: 34 IDs, all uncredited.** Each has a signature that occurs 45 times in game code, so an arrival there
  cannot be attributed, and the observer does not count them:

  `OVL_0017_0809`, `OVL_0019_0359`, `OVL_001C_3508`, `OVL_001E_0F32`, `OVL_0020_1671`, `OVL_0023_0798`, `OVL_0025_2215`,
  `OVL_002F_127A`, `OVL_0031_1922`, `OVL_0033_1F8D`, `OVL_0036_0615`, `OVL_0038_38A9`, `OVL_005E_3C6D`, `OVL_0068_10A7`,
  `OVL_006A_1FD9`, `OVL_006C_4454`, `OVL_006F_23E0`, `OVL_0075_08F6`, `OVL_0077_2B0D`, `OVL_007D_0E60`, `OVL_007F_2256`,
  `OVL_0086_37B0`, `OVL_009B_045E`, `OVL_009E_1932`, `OVL_03B0_1245`, `OVL_03B4_02A0`, `OVL_03B6_0283`, `OVL_03B9_07C0`,
  `OVL_03BD_1206`, `OVL_03C2_0BD9`, `OVL_03C6_12FB`, `OVL_03CE_0640`, `OVL_03D2_00F7`, `OVL_040C_02AD`

  **All 277 credited IDs are observable.**
- **Function starts no ledger enumerates** are invisible: the registry is the 818 starts of
  `overlay_function_ledger.csv` and `resident_callable_ledger.csv`.
- **The 42 near-only resident starts** (`RPRO_`, `RNEAR_`) have only an image offset. Their IP is inferred from the
  highest resident segment base at or below that offset. A wrong inference can only miss arrivals (the signature is still
  checked); it cannot add any.
- **Interrupts are not separated.** The observer counts every arrival, including any reached from a hardware interrupt
  handler. It has no interrupt-context flag, so no ID is called interrupt-reached here, and no arrival is excluded for it.
  (The interrupt exclusion of stages (a) and earlier applies only to a dispatch comparison; none runs here.)
- **The `.seq` arrival order is capped** at 2,000,000 lines and hit the cap in all three runs. The per-ID counts in
  `observer.tsv` are not capped.

## The claims, each with the check that pins it

- **The route was the save walk:** the frames show the slot prompt "LOAD WHICH GAME: A J" and a final "14,4 W 10:51"
  (`frames/` of each run).
- **Route-determined counts:** 146 of the 149 per-ID counts are identical across two runs of the same route (c and d).
- **Wait-determined counts:** `RES_0A40_02FD` and `RPRO_08AE0` differ between c and d.
  - The driver's waits differed: for example, the gap from the code-word gate to the menu gate was 70.5 s in c and 8.0 s in
    d.
  - KeyPressed polls in the game's idle loop, so its count reflects how long the route waited, not the route.
  - **Read neither count as a property of the walk.** The 02FD count is about 98.6% of the walk's arrivals (11,151,356 of
    11,313,021 in c).
- **`RES_0709_01DF` 1,161 against 1,160 is measured and not explained.** One glyph draw differs between the two runs.
- **What these runs do not claim:**
  - That the walk is "most of the game". It is one save, one step, and 149 of the 818 registry IDs.
  - Anything about combat, other menus, other saves, or more than one step.

## Glue order (VP i20735: hottest first)

| Order | Body | Arrivals | Status | Next step |
|---|---|---|---|---|
| 1 | `RES_0A40_02FD` | most arrivals | uncredited by policy (declared boundary) | Not a credit task: the class refuses credit. It is `INT 16h`-bound, so gluing it would be shadow form (the original still runs). Decision for the VP |
| 2 | `RES_0709_0C3F` | 131,758 | uncredited, no C found | credit task first |
| 3 | `OVL_0025_06D3` | 139 | hottest credited body not yet glued | not yet examined for call shape or services |

## Build and run

- **Registry:** `.venv/bin/python tools/hybrid/hybrid_registry.py --all-ledgers --out resources/hybrid/registry_all.tsv`.
  It needs `capstone` (the venv). It gives 818 rows: 784 unique, plus the 34 above. The 270 rows of the stage 0 registry
  are byte-identical in it.
- **Runs:** `tools/hybrid/run_observer_walk.sh NAME resources/hybrid/registry_all.tsv 300 1500 walk|menu`, through
  `poolrad-heavy.sh`. The script's own budgets are 300 s per gate and 1,500 s in total.
- **Report:**
  `tools/hybrid/stage_b_report.py <cold>/observer.tsv <walk>/observer.tsv --top 10 --tsv <walk>/stage_b_counts.tsv`.
- **First attempt:** `hybrid-stage-b-walk-a` used the fixed-time AUTOTYPE keys. On the loaded host they landed on the
  code-word prompt, so that run is off-route and is not used.
- **Second attempt:** `hybrid-stage-b-walk-b` was on-route, but it was ended without the exit report. Its counts are not
  used either.
