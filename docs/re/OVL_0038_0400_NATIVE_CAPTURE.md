# OVL_0038_0400 (U38 slot 11): composite in-game evidence (trace from run 003, entry and exit from run 006)

**Verdict: composite evidence only. The in-game trace of the slot-11 handler path (run `ovl38-0400-native-003`, 181 lines) is replayed from the entry state of run `ovl38-0400-native-006` and compared with that run's native exit state (untraced, stopped at 9FE0:0100). Store replay: 0 unattributed bytes and 0 differing bytes against the native exit image; 111 annotated memory checks match. C composition from the replayed entry state: end state EQUAL. Composition mutants: 4 of 4 detected. Replay mutants: 0 of 4 caught, because the path has no site for any of the four hooks. Evidence only: no parity, function-identity, count or credit claim.**

## Premise (corrected)

Issue #1019 and the backlog list OVL_0038_0400 slot 11 as "missing (no model)". That is stale on base `origin/main` `362c99935b63d6056f7b641a64bd218948e7b0eb`. The repository already has `src/recovered/overlay_u38_handler_0400_cpu.c`, `docs/re/OVL_0038_0400_CPU.md` (span `+0400..+04BA`, 186 bytes, SHA-256 `90be6f52f15970e5c8564d6adcde2b95a7fc4d7c3a188c5c3773d90b0e113b17`), its audit and mutation tools, and the #1554 slot-11 grant. The CPU doc's native comparison is an oracle comparison, not an in-game capture. This document adds an in-game trace and exit state. It is not a first model, and it does not re-grant or duplicate the #1554 credit.

## Method (composite, labelled)

This is **not** a single traced warm call. Under `LOGL` tracing the debugger does not stop at breakpoints or at the `LOGL` count in this configuration (see Blocked runs), so one traced run cannot also provide a native exit state. The evidence is therefore built from two runs:

1. **Trace.** Run `ovl38-0400-native-003`, traced under `LOGL B5` (181 lines) from its warm-entry state. The trace is `warm/parts/LOGCPU-part1.txt` of run 006 (a copy of run 003's file). Its SHA-256 is `f9e63d96b1046b5728c2a20953c2d114299db22e4e291b331fa4567cb784762f`. The last row is the first arrival at `9FE0:0100`, which the stop-duplicate rule drops, so 180 rows execute.
2. **Entry and exit.** Run `ovl38-0400-native-006`, which is untraced after the cold call. Its warm-entry RAM (`S0..S9`, SHA-256 `d7db2b7b61b1f071465eaa0d3d51e3c417b51df8e7fdcc678897f5ec4afe8e70`) is the replay's entry. Its native exit, taken at the untraced stop at `9FE0:0100`, has RAM SHA-256 `8789af4732feb145f61f6a76b82cbe919e395c699b6954177b2576537d8f20ec` and registers `0 27 7 0 6 8 3e98 3df0 9fe0 100 14a0 4529 1d16 flags 7246`.
3. **Replay.** The replay applies the run-003 trace to the run-006 entry and compares the result with the run-006 exit. Its annotation check ties the trace's recorded reads and writes to the run-006 entry.

Handler effects in run 006: `B+84h` (`DS:0184`) changes from `00h` to `B3h` and `A+3` (`DS:0103`) from `52h` to `72h`. These two bytes are the evidence that the path ran. `DS:6825` is `64h` at both entry and exit (the cold call had already set it), so it is not evidence for the warm call.

Driver invocation for this capture: `tests/replay/replay_ovl0038_0233_warm.py --body-seg 30FF --entry-ip 0400 --post-retf-ip 04B7`. The driver's default `--post-retf-ip 0374` is the 0233 value and must not be used for this slot.

## Forced state (operator-set; chosen to reach the call path)

- Frame: `SS:SP 1D16:3DE0`, return `9FE0:0100`, selector `A=0000`, pointer A `14A0:0100`, pointer B `14A0:0100`, index `000Bh` (slot 11), entry `08D4:0025`, `DS=14A0`. Readback matched.
- Record `14A0:0100` (as found): `A+3` = `52h`, so bit 5 is clear. With selector 0 this takes the call path (`LCALL 00BA:23DB` at `+04AC`). `B+84h` = `00h`, `B+10Eh` = `00h`.
- The far pointer at `B+108h` was **null** (`0000:0000`) as found. The handler would zero the words at `0000:000A` and `0000:000C` (interrupt-vector area), which is not a valid game path. It was set to `14A0:0800`. The bytes there are game text (`ize Exit`), and bytes `0A` to `0D` are already zero, so the handler's zero writes change nothing.
- The cold call (untraced, to `9FE0:0100`) changes `A+3` to `72h`; run 002 read this back, and `B+84h` was not read after a cold call. Before the warm call, `A+3`, `B+84h`, `B+10Eh` and `B+10Fh` were restored to their pre-cold values, so the warm call takes the same path.
- The cold call and the stub loads: `DS:6854` (slot 11) is `0865:0057` before the call (static estimate `0041:0057`). Afterwards `0865:0057` reads `EA 00 04 FF 30`, a far jump to `30FF:0400`.

## Replay (run 006 entry and exit, run 003 trace)

| Check | Result |
| --- | ---: |
| Annotated memory checks matched | 111 of 111 (`bad` empty) |
| Replayed image vs native exit image, below A0000 | 0 differing bytes |
| Differing bytes without a journaled writer | 0 |
| Rows executed (body 45; callee segment `08DE` 117; dispatcher `2F6B` 16; stubs 1 each) | 180 |
| Callee windows (the one far call) | 1 |
| Bytes changed from entry to exit (below A0000) | 4: `0x14B03` (`A+3`, 52h to 72h), `0x14B84` (`B+84h`, 00h to B3h), `0x14C0F` (`B+10Fh`, 00h to 01h), `0x20F35` (`SS:3DD5`, bp-9, 01h to 00h). `DS:6825` is 64h at both ends and is not changed. |
| Display stores | 0 |
| Snapshots | entry at `30FF:0400`; post at `2F6B:0022` |

Replayed handler-entry RAM SHA-256 `7b083646ecb00703dbea69f83ababf3e69f8cb138f53dff104893bf3b640e5cb`. Replayed post-body RAM SHA-256 `8789af4732feb145f61f6a76b82cbe919e395c699b6954177b2576537d8f20ec`, which equals the native exit RAM.

## C composition (45 instructions; callee effects declared from the trace)

`tests/replay/compose_ovl0038_0400_body.c` executes the 45 body instructions on this path. The body bytes are the pinned file span (GAME.OVR offset 61531, 186 bytes, SHA-256 `90be6f52…`), passed as `argv[1]`. They are not read from the replayed entry RAM. The registers and data come from the replayed handler-entry state. The live entry differs from the file in two bytes of the far-call segment word (`+04AF`/`+04B0`: live `08DE`, file `00BA`, a loader fixup); feeding the live bytes fails the call-target check. At `LCALL 00BA:23DB` it pushes the return CS:IP as the body does, then applies the declared callee block (effects file SHA-256 `5883590824dd8db3846c512fada9656bdac4d8ddbb6cccf01ccf3b8a3d7b6b4e`, 485 bytes). The reference is the replayed post-body snapshot at `2F6B:0022`. The comparator is `tests/replay/compare_ovl0038_017f_composition.py`, which is on `origin/main` (blob `bdf7f2091cf8a5fa75574c955ad73af995861130`) through the #1978 merge; it is not in this branch's base.

| Check | Result |
| --- | --- |
| Baseline: RAM below A0000, C model vs replayed post-body | EQUAL (0 differing bytes; 0 of 14 register fields) |
| Final return | `2F6B:0022`, `SP=3DDE`, `FLAGS=7246` |

Composition mutants (each must be caught by the comparator):

| Mutant | Result |
| --- | --- |
| `RETF 0Ah` changed to `0Ch` | detected: SP differs (3DDE vs 3DE0) |
| Pushed return IP of the call changed (`04B1` to `04B0`) | detected: 1 RAM byte differs |
| `MOV byte [6825], 64h` changed to `65h` | detected: 1 RAM byte differs |
| `ADD AX, 20h` in the `A+3` update changed to `40h` | detected: 2 RAM bytes differ |

So the comparator detects 4 of 4 composition mutants. On this path no pushes follow the call, so the pushed return bytes stay in RAM and the end-state comparison sees them. That is a property of this path, not a general claim about the body.

## Replay mutant triage

Each replay mutant is run through the same driver on the composite trace.

| Mutant | Result |
| --- | --- |
| `no-hw-frame` | not caught: `bad` empty, diff 0 |
| `int-frame-skip` | not caught: `bad` empty, diff 0 |
| `stos-skip` | not caught: `bad` empty, diff 0 |
| `tick-skip` | not caught: `bad` empty, diff 0 |

0 of 4 are caught. This is because the path has no site for any hook: no `IRET`, no `INT`, no BIOS tick and no string rows. These are survivors by construction on this path, not an equivalence finding. Replay-side triage for this handler needs a path that contains those rows, which this evidence does not include.

## Blocked and aborted runs (method evidence)

- `ovl38-0400-native-001`: a traced warm call. The breakpoint at `9FE0:0100` was set but logged and passed; the run continued into zero bytes and then an INT 6 loop. Kept with an `ABORTED.txt` note.
- `ovl38-0400-native-002`: the same failure, with the same breakpoint. Kept with an `ABORTED.txt` note.
- `ovl38-0400-native-003`: `LOGL B5` (181 lines) logged the clean path to `9FE0:0100` (this is the trace used above). The count did not stop the debugger, and execution continued into the INT 6 loop. Kept with an `ABORTED.txt` note.
- `ovl38-0400-native-004`: the first `RUN` was sent before the debugger was ready, so the keys landed out of order at the code-word screen. Kept with an `ABORTED.txt` note.
- `ovl38-0400-native-005`: a traced call with a breakpoint at `2F6B:0022`. It was set, but the row was logged twice and passed, and the run ended in the INT 6 loop. Kept with a `BLOCKED.txt` note.
- `ovl38-0400-native-006`: the composite evidence run. Untraced cold call and untraced warm call to a breakpoint at `9FE0:0100`, with no `LOGL` armed.

The cold-call breakpoint at `9FE0:0100` stops correctly when nothing is traced. The failure is specific to tracing under `LOGL` in this configuration. It is not diagnosed here. Changing the debugger is out of scope.

## Limits

1. **Composite method.** The replay runs the run-003 trace on the run-006 entry. Those two entry images differ in 104 bytes (runs 003 and 006; the 90-byte figure is for runs 003 and 005 and is not the pair used). The differing addresses include the BIOS tick word at `0040:006C` (`0x0046C`), stack bytes and data bytes. None of the 104 is annotated on this path, so the 111 annotated checks do not cover them. Their effect on this path is covered only by the final-state comparison (0 differing bytes against the native exit). Whole-image identity of the entry is not established; the annotation-level statement was checked by both independent reviews.
2. **One path.** Selector 0 with `A+3` bit 5 clear (the call path). The bit-5-set path and the non-zero selector path are not covered.
3. **Operator-set state.** The null `B+108h` pointer was replaced with a scratch pointer. Record bytes were restored after the cold call. The frame values were chosen by the operator.
4. **Callee.** `00BA:23DB` is not modelled. Its effects are declared from the same trace, so the composition is not independent of the trace.
5. **Scope of the composition.** It checks the end state and the final return. It does not verify call-site correctness.
6. **Replay mutants.** 0 of 4 caught, because the path has no site for any of the four hooks.
7. **Debugger.** Breakpoints and `LOGL` counts are not honoured under `LOGL` tracing in this configuration (runs 001 to 005). Not diagnosed.
8. **Premise and credit.** The "missing (no model)" premise in #1019 is stale. The #1554 grant stands. Nothing here grants credit or duplicates it.
9. **Base and tools.** Base `origin/main` `362c99935b63d6056f7b641a64bd218948e7b0eb`. The replay driver is the 0233 driver on main (`tests/replay/replay_ovl0038_0233_warm.py`), run with `--post-retf-ip 04B7` (see Method; the default `0374` is the 0233 value). The comparator is the 017F comparator on main. The composition C is new in this slice.
10. **Trace parts.** Only the run-003 trace (one part of 181 lines) is used. It is hashed (see Method) and is not otherwise summarized here.
11. **Authorship.** The commits carry the project placeholder identity (`reviewer-1 (maintainer rebase)`, `agents.local`), per the VP decision. Re-attribute before any public release.
12. Evidence only. No parity, function-identity, count or credit claim.
