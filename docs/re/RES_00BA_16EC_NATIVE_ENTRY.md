# 16EC same-entry original/C comparison from a native checkpoint (t24, NC3 evidence, first capture)

**One native capture, two offline comparisons, both equal up to a cold-stub
boundary; nothing is discharged.** This records the first same-entry
original/C comparison of `RES_00BA_16EC` from a state the real game produced,
and the tooling that rebuilds it. NC3's removal test asks for same-entry
evidence of the selected clear/title/printer and common-tail effects; this
first entry stops before them, at the overlay boundary, because the game had
not yet loaded the tile unit. A second capture is proposed, not run. The
[closure record](RES_00BA_16EC_CLOSURE_STATUS.md) keeps NC3 open; the reviewer
decides what the evidence is worth.

## The emulator (owner-authorized build, no binary committed)

| Item | Value |
| --- | --- |
| Source | DOSBox-X tag `dosbox-x-v2026.10.01`, commit `b6abbd5980a885f5f310a4088c59a8688d1b116c`, shallow https clone |
| Patches | `tools/hybrid/dosbox-x-b6abbd59-dispatch.patch` (`ebb5775b…`), `install_parity_checkpoint.py` with `parity_checkpoint.h` (`becb1b87…`) |
| Models object | 159 entry symbols, 213 sources from this repository's C (main `0255aa70` C tree), linked through `LIBS`; no dispatch requested at run time |
| Configure | `--enable-debug=heavy --enable-sdl2 --disable-dynamic-core`, this checkout's `include/` in `CPPFLAGS`; `C_DEBUG 1`, `C_HEAVY_DEBUG 1` |
| Binary | SHA-256 `d85a8563ba98a0a144032eb4569ba6e7753f01db1026c516054e4c144afcf973`, banner `DOSBox-X version 2026.10.01 SDL2`, kept under ignored `resources/hybrid/bin/` |
| Inertness | a prepared run with every `PR_*` variable cleared booted and exited under its 45-second time limit with no observer or checkpoint output |

The build receipt is `resources/hybrid/t24/build-receipt.json` (private).

## The capture (one run under a pre-registered packet)

The packet (`resources/t24-nc3-capture/packet.json`, SHA-256
`15b2e922…`) pinned the emulator, registry, route, game and save hashes, the
exact command and environment, the bounds (1,700-second timeout plus kill
grace, driver total 1,500 s, emulator time limit 1,560 s, gate 300 s, one
run, no retry), the first-entry `SIGSTOP`, the cleanup and the frozen-input
check, before the owner approved it. The run `t24-nc3-capture-001` loaded
slot A through route r3 (observer mode, all-ledger registry; `RES_00BA_16EC`
is unique at 32 bytes) with the checkpoint armed from the start.

| Observed | Value |
| --- | --- |
| Arrival | `RES_00BA_16EC` arrival 1, instruction counter 407,882,699, routed 0/0 |
| CPU at entry | `CS:IP 08DE:16EC` (load base `0824h`), `SS:SP 1D16:3D62`, `DS 14A0`, `ES 1D16`, `BP 3DA2`, `FLAGS 7293` |
| Game context | camp, MAGIC, CAST Detect Magic: the spell message window, not combat |
| Caller | return `08DE:2193` = the resident near site `00BA:218F` (`RES_00BA_1FEB`); wait 1, row `0Ah`, message on the stack (`1D16:3D79`, length 11), title `44BB:0008` (length 6). This matches the pinned entry-contract row for `218F` (bound 40) |
| Selectors | `DS:49F3 = 2`, `DS:6DDA = 0`: the **box** path |
| Tile stub | `0C4C:002A` = `CD 3F`: **cold**, unit 40C not loaded at this arrival |
| Driver | exit 10 at 453 s (the screen froze after the `SIGSTOP`, as the packet predicted), run ended at 464 s inside the bound |
| Frozen inputs | `sha256sum -c` over the copied `POOLRAD` files: unchanged (no save writes) |
| Cleanup deviation | the stopped emulator ignored the driver's `SIGTERM` (a stopped process does); the author sent `SIGCONT` then `SIGTERM` after the run; zero emulators left |
| Files (private) | `checkpoint.ram` 1,048,576 B `f3ed62d9…`, `checkpoint.vram` 2,097,152 B `e9c34df3…`, `checkpoint.json` `b6e53cdd…` |

## The offline comparison (declared before it ran)

`tools/overlay/res16ec_native_entry.py` loads the triple, maps the sixteen
captured registers onto the model's CPU, splits DOSBox-X's interleaved planar
VRAM into the four EGA planes (plane *p* of address *a* is `linear[4a+p]`),
rebinds the #1558 comparator's synthetic layout to the live one (load base,
return frame, loaded overlay segment when the stub is loaded) and runs
`compare()` unchanged: CPU14 under the pinned flag rules, the whole 1 MiB, the
environment event log, the ordered EGA OUT events and VRAM write operands,
the packed EGA state, the stop and the remaining budget.

**Declared mask:** the EGA sequencer, graphics-controller and latch registers
are not part of a checkpoint. Both sides start from the device's reset
registers (`seq [0,0,15,0,0]`, `gc [0,0,0,0,0,0,0,15,255]`) with the captured
plane contents, so their equality is a same-initial-state result, not a claim
about the live register state. The environment (INT 21h time, port 61h, keys)
is the declared synthetic one on both sides.

| Tree | Result |
| --- | --- |
| main `02461ec4` (16EC model as merged) | both sides stop `OVERLAY` at the box call `177B` (callee 6, box status OVERLAY) after 28 own instructions / 27 edges; every compared item equal |
| #1643 guards head `bc778b27` (frame and target guards) | identical outcome; no guard fired |

Receipts: `resources/t24-nc3-capture/offline-main.json`,
`offline-guards.json`, `offline-tracked-main.json` (private; hashes, counts
and offsets only).

What this covers: the prologue, StrStore of the live message, both selector
reads on live data, the colour byte, the box argument pushes and the cold-stub
refusal, from a state the game produced. What it does not cover: the box,
the clear, the title, the printer and the common tail, because this arrival
found the tile unit unloaded; on the original machine that call then loads
unit 40C through the overlay manager, which the port does not model.

## Proposed second capture (not run)

`resources/t24-nc3-capture/packet-2.json` (SHA-256 `6d7ebdab…`) is the same
packet with one change: the checkpoint is armed only after the observer has
logged the first `RES_00BA_16EC` arrival, so the second arrival, with the
stub loaded by the first call, is captured and the comparison can reach the
tail. It waits for the owner's decision.

## Controls and reproduction

`tools/overlay/test_res16ec_native_entry.py` needs no private capture: it
writes a synthetic checkpoint from the #1558 world (cold-stub box entry and
mode-5 entry), runs the comparator on it and requires the known outcomes
(`OVERLAY` through callee 6; `RETURNED`), checks the plane interleave against
a constructed pattern, the rebind/restore of the comparator layout, and the
refusals of a wrong entry IP or a wrong arrival id. It is discovered by
`tools/run_media_checks.sh`.

```sh
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh \
  python3 tools/overlay/res16ec_native_entry.py --run resources/oracle-runs/t24-nc3-capture-001 --output resources/t24-nc3-capture/offline.json
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh \
  python3 -m unittest -v tools/overlay/test_res16ec_native_entry.py
```

## Limits

- One arrival, one branch prefix, one route; no frequency, no all-arrival
  invariant, no claim about other selectors or about a loaded stub.
- The EGA register mask and the synthetic environment are stated above; the
  live register state of the EGA is unknown.
- The checkpoint is captured RAM and VRAM plus registers, not a machine save
  state; the stopped emulator was not resumed.
- No NC3 discharge is recorded by the author; no accounting, identity,
  readiness or credit change. The [entry contract](RES_00BA_16EC_ENTRY_CONTRACT.md)
  and the [guards](RES_00BA_16EC_GUARDS.md) are separate evidence.
