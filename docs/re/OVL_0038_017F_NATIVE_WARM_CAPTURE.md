# OVL_0038_017F (U38 slot 4): native warm capture, run ovl38-017f-native-003

**Verdict: native capture COMPLETE; store replay PASSES on the full warm trace (0 unattributed); the C composition matches the native end state with declared callee effects (end-state equal; two of three intermediate-value mutants are end-state equivalent, so the composition claim is limited). No parity, function identity, count or credit claim.**

Clearance: the eb5d0b5d controller hold was cleared by owner decision (via the VP, 2026-11-23) for this native forced-call
capture through the t4492 chain. This note records only that capture.

Pin: branch `evidence/ovl-0038-017f`, measured at `9e10392c84c21db746c8637eaeee059b4e2ec237`; base `origin/main`
`d68e6aff3f4abc01f6cb12f17065baafda3cc03a`. No rebase.

## Live addresses in this session (differ from the static estimates)

| Item | Static estimate (inventory) | Live in run 003 |
| --- | --- | --- |
| Unit 9E entry-0 stub (dispatcher) | 00B0:0025 | 08D4:0025 (CD 3F before, EA after the cold call) |
| Unit 38 slot-4 pointer, DS:6838h | 0041:0039 | 0865:0039 (CD 3F before, EA after) |
| Unit 38 slot-4 far target | U38 +017F | 30FF:017F |
| Dispatcher code after load | unit 9E +0000 | 2F6B:0000 |
| Return point used as breakpoint | 9FE0:0100 | 9FE0:0100 |

The stub segments were the same in runs 001 and 003 (08D4 and 0865). The owner's 00B0:0025 and 0041:0039 are not used.

## Runs

- `ovl38-017f-native-001`: aborted. The first traced attempt hit the LOGL limit of 2,097,152 lines inside the cold loader
  (1264:02C0 loop), before the handler. Kept, not used as evidence.
- `ovl38-017f-native-002`: aborted before ARMED. The controller raised FileExistsError. Kept, with an ABORTED note.
- `ovl38-017f-native-003`: the capture. Cold call untraced, then a traced warm call. Artifacts are under the ignored
  `resources/oracle-runs/ovl38-017f-native-003/`.

## Forced frame (chosen by the operator, not a natural caller)

At the stopped debugger (after the owner's frame was written with SR and SM, bytes read back and matched): SS:SP = 1D16:3DE0.
Words at SS:SP: return IP 0100, return CS 9FE0, forwarded A (byte word) 0000, B 0000, C 0000, D 0100, E 14A0 (the game's
DS), then the index word 0004. CS:IP = 08D4:0025, DS = 14A0. The five forwarded words and the index follow the dispatcher
frame cited in `docs/re/OVL_009E_0000_HANDLERS.md`. The values of A to E are operator choices, not measured callers.

## Cold call (untraced)

BP at 9FE0:0100, no LOGL armed. It returned to 9FE0:0100 in seconds with SP = 3DF0 (3DE0 + 4 + 12). Both stubs then read EA,
so units 9E and 38 are loaded.

## Warm call (traced)

The frame was reapplied. The entry state is in `warm/` (registers `CPU-WARM-ENTRY.raw.txt`, RAM `warm/S0..S9.BIN`,
640 KiB). The LOGL trace was armed as 200000 hex lines per part, with the breakpoint at 9FE0:0100.

| Item | Value |
| --- | --- |
| Entry registers (13 + flags) | `a8ff 454a 0008 4549 4549 0008 3e98 3de0 08d4 0025 14a0 4549 1d16 flags 7246` |
| Entry RAM (S0..S9 concatenated) SHA-256 | `dc4ad5a394959840149c42a8a302b8d5309319fe27858f47b8882654289629bd` |
| Exit registers | `a8ff 454b 0008 454a 454a 0008 3e98 3df0 9fe0 0100 14a0 454a 1d16 flags 7246` |
| Exit RAM (S0..S9 concatenated) SHA-256 | `69de1a43e81067735d825f454e7551208f7f53ccf2b12d9eaff1abbab5a7faaa` |
| Trace part 1 | 2,097,152 lines (LOGL limit stop at 1264:02C1) |
| Trace part 2 | 2,097,152 lines (LOGL limit stop at 1264:02C1) |
| Trace part 3 | 466,700 lines (breakpoint stop at 9FE0:0100) |
| Total | 4,660,004 lines, including the stop lines of parts 1 and 2 |

Stop-duplicate rule: the last row of each part is the stop line, and its executed copy is the next part's first row, so each
part drops its last row. The final row (9FE0:0100) is the breakpoint stop and does not execute.

Host-event counts by grep (per part 1, 2, 3): BIOS-segment rows 195, 195, 45; `int` 13, 13, 3; `iret` 26, 26, 6; port
`in`/`out` 5065, 13, 47; `rep` 2548, 0, 802; `retf` 672, 0, 97. The delay loop at 1264:02BD to 02C9 repeats in parts 1 and 2.
The trace contains the handler's far calls (`0AF8:0B0B`, `00BA:16EC`, `00B0:0052`) through the live stubs, and the dispatcher
RETF 0Ch at 2F6B:0023.

## Replay and composition: results (run 003)

See the section 'Replay and composition results' below; this section is superseded.

## Limits

- The warm call's traced state depends on the untraced cold call, which ran first and changed the game state.
- The forced frame values A to E are operator choices. They are not a natural caller's values.
- The chain ended when its background job finished. The controller was killed (KeyboardInterrupt in controller-launch.err),
  so the runbook's scoped cleanup records do not exist for run 003. This needs an owner decision on whether run 003 is
  acceptable evidence or must be repeated with a proper cleanup.
- The callee bodies are not modelled; their effects are to be declared from the trace in the composition.
- Hashes above cover only the RAM images and register files. The large trace parts are recorded by size and line count and
  are not hashed.
- No parity, function identity or count, credit or acceptance claim. ASSIST label not applicable; this is native evidence
  pending replay.
- The local commit carries the repository's configured identity, which is a placeholder and not publishable.

## Run-003 validity: decision

**Decision: run `ovl38-017f-native-003` COUNTS as native evidence, with the missing cleanup records stated as a LIMIT.**
The entry and exit states were recorded before the controller was interrupted, and no process from this run survives.

(a) The controller's interrupt. `controller-launch.err` ends in a `KeyboardInterrupt` raised inside the interpreter's
shutdown (threading `_shutdown`). Its last write is 15:10:43 PST. The controller therefore ended by interrupt when its
background job finished (the job's own 30-minute kill point was not the cause; the job ran to completion), not through the
runbook's scoped cleanup path. `session.err` was last written at 15:10:48 PST.

(b) Process sweep, identity by the run-003 path and by the chain's process names (dosbox-x, t4492 controller, supervisor,
observer, wait-recorder, owned-child, Xvfb, socat, gdb). Sweeps at 15:35 PST and 16:08 PST found no such process. The only
process referencing the run-003 path at 16:08 PST was the replay analysis itself, not a chain process.

(c) Timeline (PST, from the run's action log and file times):

| Event | Time |
| --- | --- |
| Warm entry registers and RAM captured (`capstart`/`capdone WARM-ENTRY`) | 14:45:43 to 14:45:57 |
| Warm traced call armed and resumed (`BP+LOGL 200000`, `RUN warm traced`) | 14:46:07 |
| Trace part 3 final (breakpoint at 9FE0:0100) | 15:10:06 |
| Exit registers written (`CPU-EXIT.raw.txt`) | 15:10:10 |
| Exit RAM captured (`capstart`/`capdone WARM-EXIT`; last file `S9.BIN`) | 15:10:10 to 15:10:23 |
| Controller `KeyboardInterrupt` (`controller-launch.err`) | 15:10:43 |
| Session error file last written | 15:10:48 |

Both states were captured before the interrupt, so the interrupt cannot have altered them.

LIMIT: the runbook's cleanup records (`inner-status` and `observer-status` wait records, and the scoped SIGTERM path) are
absent for run 003. The DOSBox-X, controller and supervisor processes ended with the background job. This does not change
the recorded states, but a reviewer may require a repeat with a proper cleanup before any credit is considered. No credit,
parity or function-identity claim follows from this capture.

## Replay and composition results (run 003)

### Store replay (full warm trace, 4,660,004 rows, stop-duplicate rule applied)

| Check | Result |
| --- | ---: |
| Memory annotations matched by replayed memory | 482,581 of 482,581; 0 mismatches |
| Interrupt frames: `IRET` popped IP, CS and FLAGS equal the next row | 58 of 58 |
| Hardware interrupt landings replayed (frame written) | 29 |
| BIOS tick callbacks (tick word +1) | 29 |
| Callee windows recorded (the three far calls) | 3 |
| String rows (REP chunks) processed | 3,354 |
| Bytes changed from entry to exit (below A0000) | 39 |
| Replayed image vs native exit image, below A0000 | 0 differing bytes |
| Differing bytes without a journaled writer | 0 |
| Display stores (A0000 and above), counted, not compared | 15,520 |

Journal bytes by class: game instruction writes, hardware frames (174), `INT` frames (174), BIOS tick (116). The replay
summary is `warm/composition/replay-summary.json` in the ignored run directory.

Two defects found during this pass were fixed before the full run, and are recorded here. The first replay attempt failed on a
register-field name in the snapshot writer (IP was not in the register dictionary), which lost that run's outputs; it was
re-run. The earlier landing model missed interrupts after jump, call, return and REP rows, and gave 23 `IRET` mismatches and
one unexplained byte. The corrected landing model replaces it: the hardware frame is the resolved successor of the previous
instruction, with the landing row's flags and IF set.

### Replay mutants (each must be caught)

Prefix runs of 500,000 rows (part 1 onward); the baseline prefix has no bad entries.

| Mutant | Catcher | Result |
| --- | --- | --- |
| no-hw-frame | `IRET` check | caught: 3 `IRET` mismatches |
| int-frame-skip | annotation and `IRET` checks | caught: 6 annotation, 3 `IRET` |
| stos-skip | annotation check | caught: 28 annotation mismatches |
| tick-skip | final-state comparison | not caught in the prefix; caught in the full run: 1 byte at 0046C differs (native 194, replayed 165) |

### C composition (body of 31 instructions; callee effects declared from the trace)

| Check | Result |
| --- | --- |
| RAM below A0000 (655,360 bytes), C model vs native post-body state | 0 differing bytes |
| Registers (14 fields), C model vs native post-body state | 0 differing fields |

Composition mutants (each must be caught by the comparator):

| Mutant | Result |
| --- | --- |
| return address of call 2 changed (0x01A5 to 0x01A4) | not detected (end state equal); cause not established |
| `RETF 0Ah` pop changed to 0Ch | detected: SP differs |
| argument byte of call 2 changed (AL 0Ah to 0Bh) | not detected (end state equal); cause not established |

So the end-state comparison detects one of three mutants. The composition establishes the end state with declared callee
effects; it does not yet establish each intermediate boundary value. A boundary-level comparison (the callee arguments and
return addresses at each call, from the native trace) is the check that would catch the two undetected mutants.

### Verdict for this pass

Store replay: PASS on the full warm trace (0 unattributed bytes). Composition: end state equal with declared callee effects;
mutant triage detects 1 of 3, so the composition claim is limited to the end state. No parity, function-identity, count or
credit claim.
