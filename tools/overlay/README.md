# tools/overlay

Python 3 (standard library only) tools for the Turbo Pascal overlay system of the DOS build.
They read the owner-supplied `START.EXE` and `GAME.OVR` from the ignored `resources/` tree and
write addresses, sizes, and counts only. Keep JSON outputs outside the repository.

```
G=resources/DOS/POOLRAD; O=/path/outside/repo
python3 tools/overlay/overlay_inventory.py --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/overlay_inventory.json --md docs/re/overlay_inventory.md
python3 tools/overlay/callgraph.py --inventory $O/overlay_inventory.json --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/callgraph.json --md docs/re/callgraph_summary.md
python3 tools/overlay/ecl_walk.py --dir $G --json $O/ecl_walk.json
python3 tools/overlay/ecl_walk.py --dir $G --sweep --json $O/ecl_sweep.json
```

| tool | purpose |
| --- | --- |
| `overlay_inventory.py` | Parse the MZ header, the FBOV header, every overlay unit descriptor in the resident image (file offset, code size, fixup size, entry stubs), decode each unit's fixup table, and list prologue functions per unit. |
| `callgraph.py` | Scan code bytes for candidate E8/9A call sites (no instruction-boundary validation), resolve near candidates to functions and fixup-confirmed far sites through the fixup table and the entry-stub map; classify the rest (runtime, resident, misaligned, unrelocated candidate). |
| `ecl_walk.py` | Static control-flow walk (default) or linear sweep (`--sweep`) of the 29 ECL script blocks with the verified operand grammar; reports coverage, stop reasons, and errors per block. |
| `ghidra/` | Headless Ghidra spike: `spike_inputs.py` prepares the resident image, one unit, fixups and stub map; `Spike.java` builds the program and decompiles four functions; `run_spike.sh` ties them together (`GHIDRA_HOME`, `JAVA_HOME` required). Decompiler output goes to the scratch directory only. |

Format facts the tools rely on are documented in `docs/re/`. The ECL operand table follows
Gold Box Explorer (bsimser, MIT, rev `eac30aba`), checked against this build.
