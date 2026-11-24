# OVL_0038_0233 (U38 slot 7): native forced-call capture, store replay and C composition

**Verdict: native capture COMPLETE (untraced cold call, traced warm call, exit state). Store replay PASSES on the full warm trace: 0 unattributed bytes and 0 differing bytes against the native exit image. The C composition, run from the replayed handler-entry state, reproduces the replayed post-body snapshot at 2F6B:0022 (end state EQUAL). Composition mutants: 2 of 4 detected; the other 2 are not observed by this comparator (a stated limit). Replay mutants: 4 of 4 caught (the unmutated control is clean). Evidence only: no parity, function-identity, count or credit claim.**

Clearance: the eb5d0b5d controller hold was cleared by owner decision (via the VP) for native runs through the t4492 controller, session and supervisor chain under the standing snapshot permission. The same decision directed the pending native steps, naming the OVL_0038_0233 native follow-up. This note records only that capture.

Pin: branch `evidence/ovl-0038-0233-native`, created from `origin/main` `5eda46b2b96b1f588c9661de71ba0e846d579f96`, with the static evidence commit `cb64f5be` cherry-picked as `e8cd8b49`. The static branch `evidence/ovl-0038-0233` is not changed. The static note pinned base `b7e74f48`; `origin/main` has moved since.

Owner media (hash only): `START.EXE` `d825df2b…`, `GAME.OVR` `872964a3…`, `SAVGAMA.DAT` `abca2483…`, the same as the 017F run. Unit 0x38 handler `+0233` is 324 bytes at `GAME.OVR` offset 61070. Its body SHA-256 is `18b0cf6db70feb85e97737e9a9cccec0d5b66fec55423fc06a2638288990aa2c`, which matches the static note. The body bytes are not reproduced in this document.

## Runs

- `ovl38-0233-native-001`: aborted. The tool shell had `TERM=unknown`, the debugger's ncurses exited at start, and no DOSBox-X process ran. Supervisor rc 0, controller rc 125. Kept with an `ABORTED.txt` note, not evidence.
- `ovl38-0233-native-002`: the capture. Started under `TERM=xterm-256color` inside the heavy-job wrapper, ARMED at 01:36 PST. Artifacts are under the ignored `resources/oracle-runs/ovl38-0233-native-002/`.

## Live addresses in run 002

| Item | Static estimate (static note) | Live in run 002 |
| --- | --- | --- |
| Slot-7 pointer, DS:6844h | 0041:0043 | far pointer 0865:0043 (CD 3F before the cold call; far jump to 30FF:0233 after) |
| Unit 9E entry stub | 00B0:0025 (inventory) / 08D4:0025 (run 003) | 08D4:0025 (CD 3F before; far jump to 2F6B:0000 after) |
| Unit 38 handler | +0233 in the unit | 30FF:0233 |
| Return point used as breakpoint | 9FE0:0100 | 9FE0:0100 |

The slot-7 pointer's stub segment (0865) is session-dependent, as the 017F note recorded for slot 4. Both stubs changed from `CD 3F` to a far jump during the cold call, so units 9E and 38 are loaded.

## Forced frame (operator choice, same shape as 017F run 003)

SS:SP = 1D16:3DE0. Words at SS:SP: return IP 0100, return CS 9FE0, forwarded A 0000, B 0000, C 0000, D 0100, E 14A0 (the game's DS), then the index word **0007** (slot 7; 017F used 0004). CS:IP = 08D4:0025, DS = 14A0. The frame was written with SR and SM, and the sixteen bytes read back equal to the intended values.

The handler reads its record through the far pointer D:E, which is 14A0:0100. The record's count byte at +14h (14A0:0114) reads `1F` before the cold call. The handler increments it on every entry, so it reads `20` after the cold call and `21` in the warm call. Both values are at or above 14h, so **this capture takes path C** (calls 1, 2 and 3, then the epilogue). Paths A and B are not exercised by this run. The record was not forced; the count is the game's own state.

## Cold call (untraced)

BP at 9FE0:0100, no LOGL armed. It returned to 9FE0:0100 with SP = 3DF0 (3DE0 + 4 + 12), and both stubs then read as loaded.

## Warm call (traced)

The frame was reapplied, the entry state was captured (registers `warm/CPU-WARM-ENTRY.raw.txt`; RAM `warm/S0..S9.BIN`, 640 KiB), and the LOGL trace was armed as `200000` hex lines per part. The LOGL count is hexadecimal, so each part holds 2,097,152 lines.

| Item | Value |
| --- | --- |
| Entry registers (13 + flags) | `ff 454a 8 4549 4549 8 3e98 3de0 8d4 25 14a0 4549 1d16 flags 7246` |
| Entry RAM (S0..S9) SHA-256 | `5910a530bf9b8e3a62be7223aab1b0d526adfc87bb3e4130db02ae7d8a1fc154` |
| Exit registers | `ff 454b 8 454a 454a 8 3e98 3df0 9fe0 100 14a0 454a 1d16 flags 7246` |
| Exit RAM (S0..S9) SHA-256 | `971b441983ebed76813c6a8ffc85efe9da6930968aab1290ba93dfc13ea66bdc` |
| Trace part 1 | 2,097,152 lines (LOGL limit stop at 1264:02BD) |
| Trace part 2 | 1,962,703 lines (breakpoint stop at 9FE0:0100) |
| Total | 4,059,855 lines; 4,059,853 rows executed |

Stop-duplicate rule: the last row of part 1 is the LOGL stop line, and the next part's first row is the executed copy, so part 1 drops its last row. The last row of part 2 is the 9FE0:0100 breakpoint stop, which does not execute, so it is dropped. That is two rows dropped in total, and 4,059,855 − 2 = 4,059,853 rows executed. This equals the sum of `rows_executed_by_segment` in the replay summary.

The part-1 stop was detected manually after the chunk loop stalled in its own stop check. The loop was stopped by its recorded PID, part 1 was copied before the next LOGL, and the trace was then resumed with the same LOGL and RUN sequence. The part copies are logged in `warm-chunks.log`.

## Replay and composition: results (run 002)

### Store replay (full warm trace, 4,059,853 rows executed)

| Check | Result |
| --- | ---: |
| Memory annotations matched by replayed memory | 425,781 matched; `bad` empty (0 mismatches) |
| Interrupt frames: `IRET` popped values equal the next row | 50; `bad` empty (0 mismatches) |
| Hardware interrupt landings replayed | 25 |
| BIOS tick callbacks (tick word +1) | 25 |
| Callee windows recorded (the three far calls) | 3 |
| String rows (REP chunks) processed | 3,301 |
| Bytes changed from entry to exit (below A0000) | 27 |
| Replayed image vs native exit image, below A0000 | 0 differing bytes |
| Differing bytes without a journaled writer | 0 |
| Display stores (A0000 and above), counted, not compared | 15,200 |

Journal bytes by class: hardware frames 150, `INT` frames 150, BIOS tick 100, game instruction writes (the rest). The replay summary is `warm/composition-0233/replay-summary.json` in the ignored run directory.

### C composition (37 instructions on the traced path; callee effects declared from the same trace)

The composition is `tests/replay/compose_ovl0038_0233_body.c`. It is compared with the 017F comparator, `tests/replay/compare_ovl0038_017f_composition.py` (on `origin/main`), which checks RAM below A0000 and the 14 register fields against a reference directory. It executes the body's own bytes, the 119 instructions of the pinned body, from the replayed handler-entry state, and it follows the 37 instructions on the path this trace took. At each far call it pushes the return CS:IP as the body's `lcall` does, then applies the trace's declared callee effects in order: the W writes, one R register line and one SP line for that call. It does not execute the callees.

The reference is the replayed **post-body snapshot**, taken at the handler's final `RETF 0Ah` landing at 2F6B:0022. Its registers are `00FF 454B 0008 454A 454A 0008 3DDE 3DDE 2F6B 0022 14A0 454A 1D16 7246`. Its RAM below A0000 is byte-identical to the native exit image (SHA-256 `971b4419…`). The end state is not the warm exit at 9FE0:0100.

The replayed handler-entry snapshot (SHA-256 `45bd21cb…`) differs from the frame-set entry capture in one byte, at linear address 20F35. The composition starts from the replayed snapshot, as the 017F composition did.

| Check | Result |
| --- | --- |
| RAM below A0000 (655,360 bytes), C model vs replayed post-body snapshot | 0 differing bytes |
| Registers (14 fields), C model vs replayed post-body snapshot | 0 differing fields |
| Final return (IP, CS and SP after `RETF 0Ah`) | 2F6B:0022, SP 3DDE, as the snapshot |

Composition mutants (each must be caught by the comparator):

| Mutant | Result |
| --- | --- |
| `RETF 0Ah` pop changed to 0Ch | detected: SP differs (3DDE vs 3DE0) |
| argument byte of call 3 changed (AL 3Eh to 3Fh) | detected: 1 RAM byte differs |
| return address of call 2 changed (pushed IP 0259 to 0258) | not observed by this comparator (end state equal) |
| argument byte of call 2 changed (AL 0Ah to 0Bh) | not observed by this comparator (end state equal) |

So the end-state comparison detects 2 of 4 mutants. What this comparator observes: RAM below A0000 and the 14 register fields of the post-body snapshot. It does not observe call targets, call arguments or return addresses at each call, and it applies declared callee effects that come from the same trace. Its control-flow evidence is the final return only. The two undetected mutants are therefore "not observed by this comparator" and are not shown equivalent. They change call-boundary values that this composition does not check. Do not cite the composition for call-site correctness.

## Replay mutant triage

Each replay mutant is run through the same store-replay driver with `POOLRAD_0233_MUTANT` set. The control is the unmutated driver on a 500,000-row prefix.

| Run | Catcher | Result |
| --- | --- | --- |
| Control: unmutated driver, 500,000-row prefix | `bad` must be empty | `bad` empty (0 mismatches). Diff 99 and unexplained 18 are expected on a prefix, which never reaches the exit image, so the exit comparison is not applicable to prefixes. |
| `no-hw-frame`, 500,000-row prefix | `IRET` check | caught: 3 `IRET` mismatches |
| `int-frame-skip`, 500,000-row prefix | annotation and `IRET` checks | caught: 6 annotation, 2 `IRET` |
| `stos-skip`, 500,000-row prefix | annotation check | caught: 16 annotation mismatches |
| `tick-skip`, full trace | final-state comparison | caught in the full run only: 1 byte at 0046C (the BIOS tick word 0040:006C; native 179, replayed 154) |

All four replay mutants are caught. Only `tick-skip` needs the full trace, as in the 017F triage. Each mutant run wrote to its own directory under `warm/composition-0233/replay-mutant-<name>/`.

## Run-002 cleanup

The warm trace finished before the supervisor's 1800-second deadline (the supervisor started 01:35:55 PST). The exit state was captured at 01:53 PST. I then sent TERM only to the recorded controller PID, after verifying its command line named this run. At 02:57 PST no DOSBox-X, controller, supervisor or owned-child process was running. The supervisor's own wait records were not re-read after the TERM, so the clean-exit status of the chain is not established by this note.

## Limits

- Only path C was exercised: 37 of the body's 119 instructions executed. Paths A and B are not covered by the capture or the composition.
- One warm call, after an untraced cold call. The forced frame values A to E are operator choices. The record's count byte is the game's state, not a forced value.
- The composition checks the end state and the final return only. Call targets, call arguments and return addresses at each call are not verified. Two of four composition mutants are not observed by this comparator. Do not cite it for call-site correctness.
- The callee bodies are not modelled. Their effects are declared from the same trace, so the composition is not independent of the trace.
- Hashes cover the entry and exit RAM images, the replayed snapshots, the effects file and the body. Trace parts 1 and 2 are recorded by size and line count and are not hashed here.
- The part-1 stop was detected by hand after the chunk loop stalled (see Warm call). The copy was taken at the same stop state, but the loop's automatic stop record is absent.
- The supervisor's wait records were not re-read after the controller TERM (see Run-002 cleanup).
- The local commits carry a placeholder author identity. It is a publication item only and must be re-authored before publication.
- No parity, function-identity, count, credit or acceptance claim.
