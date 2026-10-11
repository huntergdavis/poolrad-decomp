# DOS v1.3 oracle runs

The original DOS installation stays under ignored `resources/DOS/POOLRAD`.
Make one private, verified copy per replay so changes to saves and configuration
cannot alter the source input:

```sh
run_dir=$(tools/oracle/prepare_run.sh menu-001)
```

The script accepts the two executable hashes recorded in
[the binary audit](../FABLE_AUDIT.md), writes a full input-hash manifest inside
the ignored run directory, verifies the copy, and refuses to overwrite an
existing run. It places the copied installation under `RUN_NAME/POOLRAD` so
that `POOL.CFG`'s `C:\POOLRAD\` game and save paths resolve when DOSBox-X mounts
`RUN_NAME` as drive C. It does not launch the game. Older flat private runs
remain historical captures and must be prepared again for replay.

On this Linux host, Ubuntu's DOSBox-X 2026.01.02 package is installed. Its
[documented command-line options](https://dosbox-x.com/wiki/DOSBox%E2%80%90X%E2%80%99s-Command%E2%80%90Line-Options)
support a clean default configuration, explicit DOS commands, a save-state
directory, debugger entry, and file-I/O logging. Launch a prepared run with
Bubblewrap: it disables the network, presents host files read-only, and makes
only that run directory writable.

```sh
tools/oracle/launch.sh "$run_dir"
```

For a timed, headless smoke run, use `xvfb-run -a tools/oracle/launch.sh
"$run_dir" -time-limit 40`. A local smoke on 2026-10-10 reached the game's
loading screen after about seven seconds and credits after about 25 seconds.

An isolated run reaches the main menu. Reproduce it in a fresh run with:

```sh
run_dir=$(tools/oracle/prepare_run.sh fresh-menu-001)
xvfb-run -a tools/oracle/capture_menu.sh "$run_dir"
```

The script schedules Return after 28 seconds using DOSBox-X's
[`AUTOTYPE` command](https://dosbox-x.com/wiki/DOSBox%E2%80%90X%E2%80%99s-Supported-Commands),
captures the screen at 38 seconds, and stops at 45 seconds. It writes only to
the ignored run directory. This internal key path works under Xvfb without
window focus. After the corrected `C:\POOLRAD\` mount, a private run also
reached the saved-game chooser and showed the two local slots A and J when
`AUTOTYPE` sent the menu's highlighted `L` command. A further private run
selected slot A and reached the six-person exploration screen at position
`15,4 N`, game time `10:50`. In a separate run, `AUTOTYPE` sent `up` from that
screen; the displayed position advanced to `14,4 N` at `10:51`. This verifies
the DOS save-load and movement path and supplies the first gameplay oracle
fixture. Slot J is present in the chooser but its load result has not yet been
verified. Screenshots, save bytes, and logs stay private under `resources/`.

Keep local screenshots, emulator saves, traces, and changed game saves in
`resources/`. For each replay, record the run name, executable hashes, emulator
version/configuration, starting save hashes, exact input sequence, observed
area/position/facing, event or text ID, party changes, combat outcome, and
resulting save hashes. The first five scenarios are startup/menu, a repeatable
map movement, one scripted event, one combat, and save/reload. Record uncertainty
where a game state is not yet observable. Public reports contain only hashes,
IDs, counts, offsets, and behavioral summaries.
