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

On this host, Ubuntu's DOSBox-X 2026.01.02 package is installed. Its
[documented command-line options](https://dosbox-x.com/wiki/DOSBox%E2%80%90X%E2%80%99s-Command%E2%80%90Line-Options)
support a clean default configuration, explicit DOS commands, a save-state
directory, debugger entry, and file-I/O logging. A first manual launch from a
prepared run can use:

```sh
dosbox-x -defaultconf -defaultmapper -nogui \
  -savedir "$run_dir/.dosbox-saves" \
  -c "mount c $run_dir" -c 'c:' -c 'START'
```

Keep local screenshots, emulator saves, traces, and changed game saves in
`resources/`. For each replay, record the run name, executable hashes, emulator
version/configuration, starting save hashes, exact input sequence, observed
area/position/facing, event or text ID, party changes, combat outcome, and
resulting save hashes. The first five scenarios are startup/menu, a repeatable
map movement, one scripted event, one combat, and save/reload. Record uncertainty
where a game state is not yet observable. Public reports contain only hashes,
IDs, counts, offsets, and behavioral summaries.
