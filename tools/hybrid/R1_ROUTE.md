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
Visit bounds prevent loops. OCR alternatives include measured hot-letter
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
