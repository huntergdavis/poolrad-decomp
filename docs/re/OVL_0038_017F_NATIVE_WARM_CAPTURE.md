# OVL_0038_017F (U38 slot 4): native warm capture, run ovl38-017f-native-003

**Verdict: native capture COMPLETE for the warm call. Store replay and C composition are WRITTEN and NOT RUN.**
Blocked on the owner's CPU gate: the replay is about 4.66 million trace rows and needs a serial slot (PSI10 below 8). Host
PSI10 was 61 to 75 during this slice. No replay or composition result is claimed here.

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

## Not run

- Store replay: `tests/replay/replay_ovl0038_017f_warm.py --run <run dir> --body-seg 30FF`. A bounded validation on the
  first 30,000 rows (part 1) checked 11,455 memory annotations with 0 mismatches, and processed 209 string rows. The full
  replay has not been run.
- C composition: `tests/replay/compose_ovl0038_017f_body.c` compiles under `-std=c11 -Wall -Wextra -Werror`. It models the
  31-instruction body and applies the three callee effects declared from the native trace. It has not been run against the
  native exit state.
- Mutants: planned for the replay (`POOLRAD_017F_MUTANT`: no-hw-frame, tick-skip, int-frame-skip, stos-skip). Not run.

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
