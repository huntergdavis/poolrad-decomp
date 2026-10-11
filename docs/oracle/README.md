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
existing run. It does not launch the game.

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
The menu and gameplay replay remain to be checked. Screenshots and logs from
that smoke are kept only in the ignored run directory.

Keep local screenshots, emulator saves, traces, and changed game saves in
`resources/`. For each replay, record the run name, executable hashes, emulator
version/configuration, starting save hashes, exact input sequence, observed
area/position/facing, event or text ID, party changes, combat outcome, and
resulting save hashes. The first five scenarios are startup/menu, a repeatable
map movement, one scripted event, one combat, and save/reload. Record uncertainty
where a game state is not yet observable. Public reports contain only hashes,
IDs, counts, offsets, and behavioral summaries.
