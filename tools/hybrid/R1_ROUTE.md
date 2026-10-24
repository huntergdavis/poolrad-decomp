# R1 original and observer route

`WALK_ROUTE=r1` selects `tools/hybrid/routes/r1.json` in the separate
`r1_driver.py` harness. Existing `walk_driver.py` routes are untouched.
The JSON uses the driver owner's state format: `name`, `gate` (any-of settled
prompt substrings), `key` (xdotool name, null at a terminal milestone),
`expect_next`, `timeout_s`, and `max_visits`. The outer object names the initial
and terminal states and lists static arrival candidates.

The canonical startup remains in the harness: CODE/Return, FUNCTION/l, slot A,
15,4/SAVE LOADED plus `observer.pin`, Up, then 14,4. The JSON starts there,
paces Down/Up, handles encounters or ambushes with Quick combat, selects loot
Pool/Share/Exit, then View/Items. It ends in the named `final` state and retains
`final.png` plus a hash of its raw RGBA pixels. Split/Join, trade and drop are
not exercised by this initial route.

Keys require two identical observations and an allowed next state. Known
computer-action status and unreadable prompt bars wait within the state's
idle allowance (`COMBAT_IDLE_S`, default 300 seconds). A settled recognizable
but unexpected menu fails with status 10; the total budget fails with status 7.
Visit bounds prevent loops. Seeking allows 150 steps (the retained pinned
control first encounters combat on pace 116); the total default is 4800 seconds. OCR alternatives include measured hot-letter
misreads. OCR runs with one thread and a bounded 120-second subprocess budget
to tolerate contention on the shared host; screenshots/key commands are also
bounded. No input is sent to another X display.

Submit each native run once through the local heavy-job FIFO wrapper:

```sh
WALK_ROUTE=r1 R1_SEED_PIN=12345678 /path/to/heavy-wrapper tools/hybrid/run_r1.sh r1-original-a original
WALK_ROUTE=r1 R1_SEED_PIN=12345678 /path/to/heavy-wrapper tools/hybrid/run_r1.sh r1-original-b original
WALK_ROUTE=r1 R1_SEED_PIN=12345678 /path/to/heavy-wrapper tools/hybrid/run_r1.sh r1-observer-a observer resources/hybrid/registry_r1_pin.tsv
```

The launcher requires owner-supplied DOS files under `resources/DOS/POOLRAD`
and a pinned emulator under `resources/hybrid/bin/dosbox-x`. Seeded originals
use `resources/hybrid/registry_pin.tsv`, containing the audited unique
`DRAW_SYS_120F` row. The observer registry contains the requested candidate
rows plus that draw row. Copy unchanged unique rows from the audited registry;
keep signatures and all captures under ignored `resources/`.

Every inherited `PR_HYBRID_*` setting is cleared. The launcher then installs
only the explicitly selected registry/log and optional `R1_SEED_PIN`. Dispatch
is always unset: guest routines execute original code. A seed pin deliberately
sets the guest RNG at the first draw after the canonical marker. Unseeded mode
is available by omitting `R1_SEED_PIN`; those runs do not establish a pinned
parity reference. Successful seeded runs must contain the actual pin event.

Each run uses a disposable input copy and private Xvfb display. It retains
input hashes, mode/seed, emulator/driver/route hashes, immutable route/driver
snapshots, frames, polls, keys, per-gate timings and shutdown method. The route
issues no save command. START.EXE and GAME.OVR hashes are checked by the existing
`prepare_run.sh`. The launcher propagates failures and cleans up its own
emulator process group only.

Native acceptance requires two pinned original runs with equal final pixel
hashes, then an observer-only pass listing measured arrivals. The 28 candidates
are static possibilities, not arrival or replacement claims. `r1_report.py`
distinguishes arrived, unobserved and missing candidates. Forced termination
may leave a periodic observer snapshot; zero then proves nothing. Parity and
route-mode replacement work remains separate from this arrival harness.

```sh
python3 tools/hybrid/test_r1_driver.py
python3 tools/hybrid/r1_report.py resources/oracle-runs/r1-observer-a
```

## Measured native runs (2026-10-23)

Two pinned originals (`r1-original-c`, `r1-original-d`) and one original-code
observer (`r1-observer-a`) completed through the shared FIFO with dispatch unset.
All three used identical input, driver, route and emulator hashes. Their final
raw-RGBA SHA256 is
`0dc77c31eaa137b9538d9a6b55a94ea752d0c4dc7157321b0e11c59746acf5bb`.

| Gate | Original C (s) | Original D (s) | Observer A (s) |
| --- | ---: | ---: | ---: |
| treasure_pool | 1918.408 | 2053.710 | 1970.786 |
| treasure_share | 1926.527 | 2062.263 | 1980.839 |
| treasure_exit | 1939.589 | 2070.135 | 1988.604 |
| area_view | 1949.288 | 2078.727 | 1997.170 |
| view_items | 1960.344 | 2088.331 | 2005.362 |
| final | 1970.443 | 2097.832 | 2013.983 |

Each run issued six Quick inputs. C and observer A issued 116 pacing inputs;
D issued 117. The extra Down in D occurred while encounter text was still
drawing above the old area prompt. The input/draw timelines agree through
move 116 and reach the first Quick input at the same post-pin draw count.
All three complete post-pin RNG traces contain the same 1,447 values.
**Exact key-sequence validation fails for D.** Prompt-only stability can
accept an obsolete area menu during a transition; route-owner review of this
limitation remains pending. Equal final pixels do not establish arbitrary
guest-state equivalence or C-replacement parity.

All runs exited successfully with SIGTERM cleanup, without SIGKILL. Final
per-ID report counts agree exactly with the complete observer sequence.
Both originals contain 31,979 draw arrivals. Observer A has 31,978 draw
arrivals (its pin occurs one draw earlier) plus 154 candidate arrivals.
All 28 candidates are present in the report: 14 arrived and 14 were not
observed. These counts cover the whole run, not only the loot/items segment.
Zero does not prove unreachability. No replacement credit is claimed.

| Candidate | Whole-run arrivals |
| --- | ---: |
| `OVL_0077_17ED` | 0 |
| `OVL_0077_275B` | 0 |
| `OVL_0077_28BE` | 0 |
| `OVL_007F_0000` | 47 |
| `OVL_007F_003A` | 47 |
| `OVL_007F_0054` | 47 |
| `OVL_007F_00A8` | 0 |
| `OVL_007F_044C` | 0 |
| `OVL_007F_0508` | 1 |
| `OVL_007F_05E0` | 1 |
| `OVL_007F_0A01` | 0 |
| `OVL_007F_0B93` | 0 |
| `OVL_0086_028D` | 0 |
| `OVL_0086_044E` | 0 |
| `OVL_0086_051C` | 0 |
| `OVL_0086_059C` | 0 |
| `OVL_03B0_0000` | 3 |
| `OVL_03B0_0067` | 1 |
| `OVL_03B0_0711` | 1 |
| `OVL_03B0_07A4` | 1 |
| `OVL_03B0_08D7` | 1 |
| `OVL_03B0_0923` | 1 |
| `OVL_03B0_0969` | 1 |
| `OVL_03B0_0A08` | 0 |
| `OVL_03B0_0B03` | 1 |
| `OVL_03B0_0C33` | 0 |
| `OVL_03B0_0CC5` | 1 |
| `OVL_03B0_11F3` | 0 |
