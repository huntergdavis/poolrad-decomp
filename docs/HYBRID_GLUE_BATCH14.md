# R3 batch 14: adapter implementation and validation status (t73)

**Native validation is pending. This is not a replacement receipt.**
The parked R3 route and nine adapters from `9727cbe8` are ported onto
`ceb665018d911d61891e7f39df778a5e6de1a020`. The tenth target, `OVL_0038_010D`,
now has a direct entry to the existing dispatcher slot-1 adapter. No credit
or replacement ledger changes are included.

| Target | C entry |
| --- | --- |
| `OVL_0038_0000` | `pr_u38_helper_0000_cpu` |
| `OVL_0038_010D` | `pr_u38_entry2_010d_cpu` |
| `OVL_0038_133F` | `pr_u38_target_reset_133f_cpu` |
| `OVL_0038_2992` | `pr_u38_entry91_2992_cpu` |
| `OVL_0038_2DE4` | `pr_u38_roll_clear_2de4_cpu` |
| `OVL_005E_32E6` | `pr_u5e_caller_32e6` |
| `OVL_0086_0729` | `pr_u86_entry3_0729` |
| `OVL_0086_07D6` | `pr_u86_roll_value_07d6` |
| `OVL_009E_0E52` | `pr_u9e_list_append_0e52_cpu` |
| `RES_00BA_26C2` | `pr_res_00ba_26c2_cpu` |

The cumulative patch adds only these ten rows. Their CPU layouts are checked
at compile time. `005E:32E6` retains the parked adapter's requirement for
observed SYSTEM, U3C6 and U3C2 segment bindings; missing bindings decline to
the original, and stale bindings remain subject to the model's code checks.

## Direct-entry boundary

The new `010D` function calls the unchanged `h_010d` adapter in
`overlay_u9e_dispatch_0000_cpu.c`. It requires exactly 1 MiB of A20-off RAM,
IP `010D`, and entry SP in `[0200,FFEE]`. Invalid arguments leave state
unchanged. Loaded instruction bytes must remain unchanged by stack/data
writes. It preserves the existing semantic model and flag reconstruction.

`tools/hybrid/check_r3_010d.py` compares the original 16-byte, six-instruction
body against this entry: **545 cases, 14 CPU words and all 1,048,576 RAM bytes,
zero masks**. Cases cover all byte values for both operands, arithmetic
edges, stack/data aliases, odd stacks, and A20 wrapping. Seven argument
refusals leave state unchanged. Six deliberately broken variants are
detected: argument cleanup, ADD carry, INC zero flag, state addition,
saved-BP write, and entry-IP gate.

Body SHA-256: `870dfc9d737ed044d6ff9eae323534b19716dc36b040f6e126296cabcf5cb5ac`.
Media pins are START.EXE
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and GAME.OVR
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The local JSON receipt records the tested head and source hashes.

## Route and evidence gates

The parked route has 41 states and 29 visual gates. Actor, combat actor,
spell and cursor observations must match before an input is selected.
The gray-background glyph threshold is opt-in for R3. Candidate metadata
names the ten targets so the shared route reporter can process dispatch
runs. The synthetic hybrid suite currently contains 49 tests, including
wrong actor/spell/cursor controls and incomplete-trace refusal controls.

Fresh validation must use one frozen binary, driver, route, registry and
game-input set for two pinned original runs before either dispatch run.
`r3_report.py` requires matching original-pair hashes, actions, post-pin RNG
values and final pixels. V1 additionally requires matching RNG observations
and zero dispatch state differences. Routed C RNG calls are absent from
the original guest trace, so that trace is reported without claiming full
RNG equality. Route actions and final pixels must still match, and every
interruption, timeout, argument refusal and stop remains named.

The report counts original credited-body executions with and without C.
For routed runs it subtracts actual C returns from observed entries;
observing a target at entry does not mean its original body executed.
These are execution counts, not new recovery or replacement credits.

## Reproduction and remaining work

All heavy commands go through `/home/hunter/workspace/poolrad-heavy.sh`.
Private files are under ignored `resources/`; the resource symlink is not
tracked. The isolated emulator uses pinned DOSBox-X
`5a9c7c53481c2e933948039c33256776653f4faf` and the cumulative dispatch patch.

```sh
/home/hunter/workspace/poolrad-heavy.sh python3 tools/hybrid/check_r3_010d.py --out resources/checks/010d-final
/home/hunter/workspace/poolrad-heavy.sh python3 -m unittest discover -s tools/hybrid -p 'test_*.py'
tools/run_media_checks.sh
WALK_ROUTE=r3 R1_SEED_PIN=12345678 R1_TOTAL_S=7200 /home/hunter/workspace/poolrad-heavy.sh tools/hybrid/run_r1.sh t73-original-a original resources/hybrid/registry-r3.tsv
WALK_ROUTE=r3 R1_SEED_PIN=12345678 R1_TOTAL_S=7200 /home/hunter/workspace/poolrad-heavy.sh tools/hybrid/run_r1.sh t73-original-b original resources/hybrid/registry-r3.tsv
/home/hunter/workspace/poolrad-heavy.sh python3 tools/hybrid/r3_report.py resources/oracle-runs/t73-original-a resources/oracle-runs/t73-original-b --out resources/checks/original-pair.json
```

After that pair passes, use dispatch mode with
`R1_DISPATCH=resources/hybrid/dispatch-r3.txt`; leave `R1_BUS_MODE` unset for
V1 and set it to `route` for the routed run. Pass both resulting directories
to `r3_report.py` through `--v1` and `--route`.

**Not yet verified:** fresh native original pair, V1, routed comparison,
per-target original-execution reductions, and native behavior of this
combined patch. Earlier R3x receipts are historical, not substitutes.
The route does not establish Bakshi's Sleep or off-route coverage.

The first native build caught an author porting error: the new adapter
block had been inserted into the logging function instead of `run_c`.
Compilation refused it. The block was moved to `run_c`; the initial failure
and corrected build logs remain in `resources/hybrid/`.
