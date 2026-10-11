# tools/overlay

Python 3 (standard library only) tools for the Turbo Pascal overlay system of the DOS build.
They read the owner-supplied `START.EXE` and `GAME.OVR` from the ignored `resources/` tree and
write addresses, sizes, and counts only. Keep JSON outputs outside the repository.

```
G=resources/DOS/POOLRAD; O=/path/outside/repo
python3 tools/overlay/overlay_inventory.py --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/overlay_inventory.json --md docs/re/overlay_inventory.md
python3 tools/overlay/callgraph.py --inventory $O/overlay_inventory.json --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/callgraph.json --md docs/re/callgraph_summary.md
python3 tools/overlay/function_evidence.py --inventory $O/overlay_inventory.json --callgraph $O/callgraph.json --json $O/function_evidence.json
# Optional: requires Capstone 5 with 16-bit x86 support.
python3 tools/overlay/validate_near_calls.py --inventory $O/overlay_inventory.json --callgraph $O/callgraph.json --ovr $G/GAME.OVR --json $O/near_call_evidence.json
python3 tools/overlay/function_ledger.py --inventory $O/overlay_inventory.json --callgraph $O/callgraph.json --near-calls $O/near_call_evidence.json --ovr $G/GAME.OVR --exe $G/START.EXE --csv docs/re/overlay_function_ledger.csv --md docs/re/overlay_function_ledger_summary.md
python3 tools/overlay/ecl_walk.py --dir $G --json $O/ecl_walk.json
python3 tools/overlay/ecl_walk.py --dir $G --sweep --json $O/ecl_sweep.json
```

| tool | purpose |
| --- | --- |
| `overlay_inventory.py` | Parse the MZ header, the FBOV header, every overlay unit descriptor in the resident image (file offset, code size, fixup size, entry stubs, run-time stub segment normally from the next unit's header), decode each unit's fixup table, and list prologue functions per unit. The final unit's segment is an explicitly marked inference corroborated by relocated call sites. |
| `callgraph.py` | Scan code bytes for candidate E8/9A call sites (no instruction-boundary validation), resolve near candidates to functions and fixup-confirmed far sites through the fixup table and the run-time stub addressing `segment:(0x20+5*j)`; classify the rest (runtime, resident, unrelocated candidate). |
| `function_evidence.py` | Reconcile overlay prologue hits, public entry slots, fixup-confirmed far targets, and provisional raw near-call targets; report anomalies without claiming a final game-function count. |
| `function_ledger.py` | Per-address overlay routine ledger (issue #26): grades each prologue candidate by public entry, entry alias (word = offset + 0x800), fixup-confirmed far call, decoded near call and procedure-pointer constant evidence; adds bounded-decode feature columns and measured behaviour classes. Needs Capstone 5 for features. |
| `validate_near_calls.py` | Optional Capstone 5 audit: recursively decode from candidate prologues, validate direct near-call instruction boundaries, and compare with the raw call scan. The starting prologues remain candidates. |
| `ecl_walk.py` | Static control-flow walk (default) or linear sweep (`--sweep`) of the 29 ECL script blocks with the wire-format operand table; `--engine-prefetch-counts` switches to the engine's prefetch table for comparison; reports coverage, stop reasons, and errors per block. |
| `ghidra/` | Headless Ghidra spike: `spike_inputs.py` prepares the resident image, one unit (`SPIKE_UNIT_OVR`), fixups and the run-time stub map; `Spike.java` builds the program, patches non-call fixups, and decompiles the `SPIKE_TARGETS` functions; `run_spike.sh` ties them together (`GHIDRA_HOME`, `JAVA_HOME` required). Decompiler output goes to the scratch directory only. |

Format facts the tools rely on are documented in `docs/re/`. The ECL operand table follows
Gold Box Explorer (bsimser, MIT, rev `eac30aba`), checked against this build.
