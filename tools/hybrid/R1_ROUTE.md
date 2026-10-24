# R1 original and observer route

`r1_driver.py` adds a separate route beside `walk_driver.py`. It loads slot A,
verifies the saved position and one north step, seeks an encounter, uses Quick
combat, then selects loot Pool, Share, Exit, View and Items. Each key requires
an allowed gate on two consecutive observations. Unknown screens receive no
input and expire at the gate or total timeout. Every gate has a visit bound.
The initial implementation stops when the Items menu is recognized; inventory
splitting, money transfer and dropping are not yet included.

Run through the local heavy-job FIFO wrapper, one submission per run:

```sh
/path/to/heavy-wrapper tools/hybrid/run_r1.sh r1-original-a original
/path/to/heavy-wrapper tools/hybrid/run_r1.sh r1-observer-a observer resources/hybrid/registry_all.tsv
python3 tools/hybrid/r1_report.py resources/oracle-runs/r1-observer-a
```

The launcher requires owner-supplied DOS files under `resources/DOS/POOLRAD`
and a pinned emulator under `resources/hybrid/bin/dosbox-x`. It clears every
inherited `PR_HYBRID_*` control. Observer mode sets only the registry and log;
it never dispatches recovered C or pins guest state. Both modes use disposable
copies of the inputs and a private Xvfb display. The route issues no save command.
The executable and overlay hashes are checked by `prepare_run.sh`; the run
records input hashes, emulator/driver/route hashes, mode, a route/driver snapshot,
frames, polls, key events, per-gate timings and shutdown method.

`routes/r1.json` records `gate`, `key`, `expected_next`, `timeout` (seconds), and
`max_visits`. Expected names are ordered: specific confirmation gates precede
broad menu gates. A key list alternates keys across visits. A null key only
records a milestone. An empty expected-next list completes the route. After
a key, stability is measured afresh. OCR geometry and initial controls reuse
`walk_driver.py`; combat animation text is excluded from Quick input.

The 28 candidate IDs are static call-path candidates from the loot/inventory
route design. They are not arrival or replacement claims. The observer report
lists actual counts, zero observations and missing registry IDs separately.
An interrupted or forced-kill run must not establish a completed route or a
zero-arrival conclusion. Reaching a body only qualifies it for subsequent
integration and parity work; no replacement credit follows from this harness.

Run the synthetic transition tests without an emulator:

```sh
python3 tools/hybrid/test_r1_driver.py
```

Native acceptance requires an all-original run reaching treasure and View
Items, followed by an observer-only run with per-gate evidence. Keep captures,
original game text/media and derived evidence under ignored `resources/`.
