# OVL_0038_017F (U38 slot 4): native warm capture, run ovl38-017f-native-003

**Verdict: native capture COMPLETE; store replay PASSES on the full warm trace (0 unattributed bytes). The C composition reproduces the replayed post-body snapshot at 2F6B:0022 (RAM below A0000 and the 14 register fields equal) with declared callee effects. Of three intermediate-value mutants, one is detected by this comparator; the other two are not observed by it, so the composition claim is limited to the end state and the final return. No parity, function identity, count or credit claim.**

Clearance: the eb5d0b5d controller hold was cleared by owner decision (via the VP, 2026-11-23) for this native forced-call
capture through the t4492 chain. This note records only that capture.

Pin: branch `evidence/ovl-0038-017f`. The native capture was recorded at `5e60f84a`. The replay, composition and mutant
results were measured at `d441dd23cf95e6d172a756f7815fecd94eb6de92`. The commit that carries this text changes no measured
value. Base `origin/main` `d68e6aff3f4abc01f6cb12f17065baafda3cc03a`. No rebase.

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
- `ovl38-017f-native-003`: the capture. Cold call untraced, then a traced warm call. Both states were captured before the
  chain was ended by the supervisor's 30-minute deadline (see Run-003 validity). Artifacts are under the ignored
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
640 KiB). The LOGL trace was armed as `200000` hex lines per part (2,097,152 lines), with the breakpoint at 9FE0:0100.

| Item | Value |
| --- | --- |
| Entry registers (13 + flags) | `a8ff 454a 0008 4549 4549 0008 3e98 3de0 08d4 0025 14a0 4549 1d16 flags 7246` |
| Entry RAM (S0..S9 concatenated) SHA-256 | `dc4ad5a394959840149c42a8a302b8d5309319fe27858f47b8882654289629bd` |
| Exit registers | `a8ff 454b 0008 454a 454a 0008 3e98 3df0 9fe0 0100 14a0 454a 1d16 flags 7246` |
| Exit RAM (S0..S9 concatenated) SHA-256 | `69de1a43e81067735d825f454e7551208f7f53ccf2b12d9eaff1abbab5a7faaa` |
| Trace part 1 | 2,097,152 lines (LOGL limit stop at 1264:02C1) |
| Trace part 2 | 2,097,152 lines (LOGL limit stop at 1264:02C1) |
| Trace part 3 | 466,700 lines (breakpoint stop at 9FE0:0100) |
| Total | 4,661,004 lines; 4,661,001 rows executed (see the stop-duplicate rule) |

Stop-duplicate rule: the last row of parts 1 and 2 is the LOGL stop line, and the next part's first row is the executed
copy of that instruction, so each of those parts drops its last row. The last row of part 3 is the 9FE0:0100 breakpoint
stop, which does not execute, so it is dropped. 4,661,004 − 3 = 4,661,001 rows executed. This equals the sum of
`rows_executed_by_segment` in the replay summary.

Host-event counts by grep (per part 1, 2, 3): BIOS-segment rows 195, 195, 45; `int` 13, 13, 3; `iret` 26, 26, 6; port
`in`/`out` 5065, 13, 47; `rep` 2548, 0, 802; `retf` 672, 0, 97. The delay loop at 1264:02BD to 02C9 repeats in parts 1 and 2.
The trace contains the handler's far calls (`0AF8:0B0B`, `00BA:16EC`, `00B0:0052`) through the live stubs, and the dispatcher
RETF 0Ch at 2F6B:0023.

### Trace files and the canonical copy (M2)

- Canonical trace: `warm/parts/LOGCPU-part1.txt`, `warm/parts/LOGCPU-part2.txt` and `warm/parts/LOGCPU-part3.txt`. These are
  the only trace inputs the replay reads.
- The top-level `LOGCPU.TXT` in the run directory is the DOSBox-X LOGL target named in the action log
  (`BP+LOGL 200000 ... -> LOGCPU.TXT (warm)`). The replay does not read it. Its content is byte-identical to part 3
  (sha256 `b571f74499d0ef5b3c363a9bc525322072fa69a762c429640096fb737b22b297`), it has a different inode, and its last write is
  15:06:55 PST. The part-3 entry in `warm-chunks.log` is logged at 15:10:06 PST.
- Origin: `warm-chunks.log` records the part-1, part-2 and part-3 stops, and the parts were taken from the LOGL output at
  those stops. The command that made each copy is not recorded in the run directory, and it was not located in the scratch
  trees. How the parts and the top-level copy were produced is therefore not established. The identical bytes show that
  the copies agree; they do not verify the copy mechanism.
- Hashes: only part 3 and the top-level copy are hashed here. Parts 1 and 2 are not hashed; their agreement with the
  recorded summary is the evidence for them (see Limits).

## Replay and composition: results (run 003)

See the section 'Replay and composition results' below; this section is superseded.

## Replay and composition results (run 003)

### Store replay (full warm trace: 4,661,004 lines, 4,661,001 rows executed, stop-duplicate rule applied)

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
| tick-skip | final-state comparison | not caught in the prefix; caught in the full run: 1 byte at 0046C (the BIOS tick word 0040:006C; native 194, replayed 165) |

Mutant prefix and full runs write under `warm/mutant-<name>/`. The baseline prefix run wrote to the canonical effects path; see
Artifact record.

### C composition (body of 31 instructions; callee effects declared from the same trace)

End state (M5): the post-body snapshot. It is the replayed machine state when the handler's final `RETF 0Ah` returns to the
dispatcher at 2F6B:0022. Its 14 register fields are `A8FF 454B 0008 454A 454A 0008 3DDE 3DDE 2F6B 0022 14A0 454A 1D16 7246`
(`post-body-regs14.txt`), and its RAM below A0000 is `post-body-ram.bin`. It is not the warm exit at 9FE0:0100, which is the
capture's breakpoint stop. The replay reproduces the native exit image separately (see Store replay).

| Check | Result |
| --- | --- |
| RAM below A0000 (655,360 bytes), C model vs replayed post-body snapshot | 0 differing bytes |
| Registers (14 fields), C model vs replayed post-body snapshot | 0 differing fields |

Composition mutants (each must be caught by the comparator):

| Mutant | Result |
| --- | --- |
| return address of call 2 changed (0x01A5 to 0x01A4) | not observed by this comparator (end state equal) |
| `RETF 0Ah` pop changed to 0Ch | detected: SP differs (final return: IP, CS and SP after RETF) |
| argument byte of call 2 changed (AL 0Ah to 0Bh) | not observed by this comparator (end state equal) |

What this comparator observes: RAM below A0000 and the 14 register fields of the post-body snapshot. It does not observe the
call targets, call arguments or return addresses at each call. It applies declared callee effects from the same trace, and it
does not read the words that call 2 pushed. Its only control-flow evidence is the final return (IP, CS and SP after RETF).
The two undetected mutants are therefore "not observed by this comparator". They are not shown to be equivalent.

Mechanism, as reported by an independent review at this pin (reproduced there; not re-derived in this document): the two
mutated call-2 stack words (SS:3DAE and SS:3DB4) are not written by the declared callee-1 windows, and the body's own call-3
pushes overwrite them (SS:3DAE to SS:3DBD), so the end-state comparison cannot see them.

The call constants in the C model (return IPs 0x019A, 0x01A5 and 0x01BD; call-2 argument bytes 0x0A and 0x01; final
`RETF 0Ah`) agree with the same review's decode of the pinned body. This composition does not verify them, and it is not a
call-site check. Do not cite it for call-site correctness.

### Verdict for this pass

Store replay: PASS on the full warm trace (0 unattributed bytes). Composition: end state equal with declared callee effects,
and the final return only; mutant triage detects 1 of 3. The two undetected mutants are an accepted stated limit: they change
call-boundary words this composition does not observe, and nothing here indicates a model error. A boundary-level check (the
native stack words at each callee entry against the C model's pushes, or a static constant-versus-decode check) would close
the gap; neither exists at this pin. No parity, function-identity, count or credit claim.

## Run-003 validity: decision

**Decision: run `ovl38-017f-native-003` COUNTS as native evidence, with the missing scoped-cleanup path stated as a LIMIT.
The run did not run to completion. Its chain was ended by the supervisor's 30-minute deadline, after both states had been
captured.**

(a) Termination. The supervisor (pid 818573, `supervisor-identity.txt`) started its owned child at 14:40:42 PST (first line of
`supervisor.log`). At 15:10:42 PST its deadline fired with the child still running (`primary reason=timeout status=None`). It
then sent TERM to the child's process group (`group TERM`, 15:10:42.877 PST). The controller's `KeyboardInterrupt` is the last
write of `controller-launch.err`, at 15:10:43 PST. The supervisor's audit ended at 15:10:48 PST with `group_alive=False`. Its
status record is pid 818573 with `returncode=124` (`inner-status`). Its source raises exit 124 only when the reason is
`timeout` (`t4492-supervisor.py`, in the chain's scratch tree outside this repository). A leftover loop and tail were stopped
at 15:18:59 PST, as recorded by `actions.log` (`stopped leftover loop and tail`). The earlier statement that the job ran to completion, and that the 30-minute
deadline was not the cause, was wrong and is withdrawn.

(b) Process sweep, identity by the run-003 path and by the chain's process names (dosbox-x, t4492 controller, supervisor,
observer, wait-recorder, owned-child, Xvfb, socat, gdb). Sweeps at 15:35 PST and 16:08 PST found no such process. The only
process referencing the run-003 path at 16:08 PST was the replay analysis itself, not a chain process.

(c) Timeline (PST, from the run's action log, supervisor log and file times):

| Event | Time |
| --- | --- |
| Supervisor starts its owned child | 14:40:42 |
| Warm entry registers and RAM captured (`capstart`/`capdone WARM-ENTRY`) | 14:45:43 to 14:45:57 |
| Warm traced call armed and resumed (`BP+LOGL 200000`, `RUN warm traced`) | 14:46:07 |
| Trace part 3 final (breakpoint at 9FE0:0100), logged in `warm-chunks.log` | 15:10:06 |
| Exit registers written (`CPU-EXIT.raw.txt`) | 15:10:10 |
| Exit RAM captured (`capstart`/`capdone WARM-EXIT`; last file `S9.BIN`) | 15:10:10 to 15:10:23 |
| Supervisor deadline fires with child running (`primary reason=timeout`) | 15:10:42 |
| Supervisor `group TERM` to the child's process group | 15:10:42.877 |
| Controller `KeyboardInterrupt` (`controller-launch.err`) | 15:10:43 |
| Supervisor audit ends, `group_alive=False`; status records written | 15:10:48 |
| Leftover loop and tail stopped by identity (`actions.log`) | 15:18:59 |

Both states were captured before the deadline, so the termination cannot have altered them.

LIMIT: the scoped cleanup path is absent for run 003. The run was ended by the supervisor's deadline and group TERM, not by a
scoped cleanup. The wait records do exist: `outer-status` (pid 817451, returncode 125), `observer-status` (pid 819059,
returncode 0) and `inner-status` (pid 818573, returncode 124), all written at 15:10:48 PST. An earlier version of this note
said those wait records were absent; that was wrong. This does not change the recorded states, but a reviewer may require a
repeat with a proper scoped cleanup before any credit is considered. No credit, parity or function-identity claim follows from
this capture.

## Artifact record (M1, M2)

- Effects file. `warm/effects-017f.txt` held the full-run composition input: three callee windows, 622,108 bytes as recorded
  before the overwrite. A 500,000-row baseline prefix run at 17:06 PST overwrote it with one window (243 bytes, `CALL 0` only).
  The driver writes baseline effects to that path for any `--max-rows` value, so a prefix run overwrites the full-run file.
  The same prefix run rewrote `warm/composition/handler-entry-ram.bin` and `handler-entry-regs14.txt` with identical bytes.
- Restore. A full-run replay was re-run at 2026-11-24 02:45:55 UTC (2026-11-23 18:45:55 PST) on a hard-linked copy of the run directory
  (`scratch/m1-regen/`: untracked, not in `.gitignore`, not committed; it must stay out of any commit). The copy's writes went only to the copy's own unlinked outputs. Its replay returned rc 0.
  Its replay summary, handler-entry snapshots, post-body snapshots and composition result are byte-identical to the canonical
  files (`cmp`). Its C composition gives EQUAL (0 RAM bytes and 0 register fields differing). The copy's effects file was installed as the canonical
  `warm/effects-017f.txt`: 622,108 bytes, sha256 23e69778d62f339cd00d40e8f361002c0b52f3af4116e18f787158c50dbd2d99. The regenerated size matches the size recorded before the overwrite.
- Canonical composition inputs: `warm/effects-017f.txt` (full run) and `warm/composition/handler-entry-{ram.bin,regs14.txt}`.
  The post-body snapshot and the replay summary are in `warm/composition/`. Mutant outputs are in `warm/mutant-<name>/` and
  are not composition inputs.
- Trace: see Trace files and the canonical copy (M2) above.

## Limits

- The warm call's traced state depends on the untraced cold call, which ran first and changed the game state.
- The forced frame values A to E are operator choices. They are not a natural caller's values.
- Run 003 was ended by the supervisor's 30-minute deadline after both states were captured, and its scoped cleanup path
  is absent (see Run-003 validity).
- The callee bodies are not modelled. Their effects are declared from the same trace, so the composition is not independent of
  the trace.
- The composition does not verify call arguments or return addresses at any call. It establishes the end state and the final
  return (IP, CS and SP after RETF) only. Do not cite it for call-site correctness.
- Hashes above cover the entry and exit RAM images, the register files, part 3 and the top-level copy. Trace parts 1 and 2 are
  recorded by size and line count, and are not hashed.
- The baseline prefix run overwrote the full-run effects file; it has been restored by the re-run recorded under Artifact
  record. Re-running any prefix of the baseline overwrites the canonical effects file again.
- The local commit carries a placeholder author identity. It is a publication item only and must be re-authored before
  publication.
- No parity, function-identity, count, credit or acceptance claim.
