# OVL_0038_07E0 (U38 slot 23): composite in-game evidence (trace from run 002, entry and exit from run 003)

**Verdict: composite evidence, not an exact replay.** The C composition from the replayed entry state (baseline) matches the replayed post-body snapshot at 2F6B:0022 (EQUAL, 76 body instructions, seven declared far calls). Composition mutants: 2 of 4 detected; the other 2 are call-boundary words this comparator does not observe. Store replay: 0 unattributed bytes (writer account) and no annotation mismatch (`bad {}`), but the replayed image differs from the native exit in **12 bytes** (one BIOS tick byte and 11 stack bytes). By inference from the tick count, the cause is one IRQ0 that the untraced exit run took and the traced run did not. That inference is not closed. Replay-side mutants: 3 of 4 caught (`no-hw-frame`, `int-frame-skip`, `stos-skip`); `tick-skip` is not evaluable on this trace (see the table). Evidence only: no parity, function-identity, count or credit claim.

## Premise gate (checked on origin/main before any capture)

Base `origin/main` `9851b7b62be1c00a8e1e0c97acce4fff5d08c1f7`. No `OVL_0038_07E0` handler, model, CPU doc or grant exists on the base:

- `docs/re/OVL_009E_0000_HANDLERS.md` line 71: slot 23 (`6884h`, stub `0041:0089`) is "named gap: uncredited handler OVL_0038_07E0".
- `docs/re/U9E_DISPATCH_HANDLER_BACKLOG.md` line 68: "- [ ] OVL_0038_07E0 — slots 23; missing".
- `docs/re/function_backlog_reconciliation.csv` line 202 and `docs/re/u9e_dispatch_handlers.json` lines 547 and 1933 list the entry as a bare handler id (no model, no strict or credit row).
- Name and offset mentions elsewhere are not coverage: `docs/re/RES_00BA_16EC_PORT_HARNESS.md` line 14 lists `+07E0` as a caller of `00BA:16EC`; `tools/overlay/res16ec_entry_contract.py` line 63 lists `0038:0902` calling `0x38:0x07e0`; `docs/re/OVL_0038_0775_CPU.md` and `tools/overlay/ovl038_0775_cpu_audit.py` use `07E0` as the bound of entry 0775. None models or grants the `+07E0` body.

The premise holds: the entry is uncredited and unmodelled, so this slice proceeds.

## Handler facts (static; owner media, not committed)

- Span: GAME.OVR offset 62523 (`60507 + 0x07E0`), 312 bytes, SHA-256 `e69db1e310b0ec6e71b5b97975e799e122561e24d933abd9ba02922b011f4fd4`, 103 instructions tiling the span, one `RETF 0Ah` at `+0915`.
- Arguments: selector byte `[BP+6]`; pointer `[BP+8]` (unused on this path); record pointer `[BP+0Ch]` (`B`). The handler walks a chain from `B+C8h` (far pointer), then, on the path below, makes its far calls.
- Far calls in address order: `0852 00BA:1509` (only on a non-zero selector, not on this path); `087B 0AF8:023F`; `0892 0AF8:16A8`; `08D3 00BA:15F7`; `08E2 0AF8:0254`; `08F7 0AF8:0B0B`; `0902 00BA:16EC`; `090D 00BA:0BB8`.
- The captured path: selector `0`, chain pointer `B+C8h` null (so no walk, flag `[BP-5]` stays 0), `B+C7h` below `10h`. It makes seven calls (all but `0852`) and then `RETF 0Ah`.

## Method (composite, labelled)

This is **not** a single traced warm call. Under LOGL tracing the breakpoint and count behaviour was unreliable (see `OVL_0038_0400_NATIVE_CAPTURE.md`), and the trace is longer than 2M rows. The evidence is built from two runs of the same procedure:

1. **Trace**, run `ovl38-07e0-native-002`: an untraced cold call, the warm-entry capture, then the warm call traced under `LOGL 200000` in three parts. Part 1 (2,097,152 lines, SHA-256 `469905fe…`) and part 2 (2,097,152 lines, SHA-256 `dcedffc0…`) end at LOGL limits. Part 3 is cut at its first `9FE0:0100` row (the return; 227,291 lines, SHA-256 `ad414218…`). The composite trace is therefore 4,421,595 rows including the stop line. The uncut part 3 is kept in the run directory, outside the parts glob.
2. **Entry and exit**, run `ovl38-07e0-native-003`: the same procedure, but the warm call is untraced and stops at the breakpoint `9FE0:0100`. Entry RAM SHA-256 `c9f10343…`; exit RAM SHA-256 `461737bc…`; exit registers `fffe ffff 0 0 d1a 100 3e98 3df0 9fe0 100 14a0 14a0 1d16 flags 7246`.
3. **Replay**: the run-002 trace is replayed on the run-003 entry and compared with the run-003 exit. The driver is the 0233 driver on `origin/main`, run with `--body-seg 30FF --entry-ip 07E0 --post-retf-ip 0915`. The driver default `--post-retf-ip 0374` is the 0233 value and must not be used for this slot.

The entry images of runs 002 and 003 differ in 89 bytes (including the tick word `0040:006C`). The replay uses run 003's entry for its start, and the annotation check ties the trace's reads to it.

## Forced state (operator-set)

- Frame: `SS:SP 1D16:3DE0`, return `9FE0:0100`, selector `A=0000`, pointers `A` and `B` both `14A0:0100`, index `0017h` (slot 23), entry `08D4:0025`, `DS=14A0`. Readback matched.
- Record `14A0:0100` as found: `B+C7h` = `74h`, `B+C8h` = `6C61:6369` (non-null). With that state the walk would follow a garbage pointer, so run 001 was aborted (`ABORTED.txt`).
- Operator-set before the cold call and again before the warm entry, in runs 002 and 003: `B+C7h` = `00h`, `B+C8h..CBh` = `0000:0000`. The cold call changes `B+C7h` to `01h` and the chain pointer to a non-null value, so the state is re-applied before each warm entry.
- The slot-23 pointer at `DS:6884` reads `0865:0089` before the cold call (bytes `89 00 65 08`). The cold call returns to `9FE0:0100` with `SP=3DF0`. The unit-9E stub `08D4:0025` reads a far jump to `2F6B:0000` after it. The slot-23 stub itself was not read after the cold call in this slice.

## Native results

- Warm call (run 003, untraced to the breakpoint): exit RAM differs from the warm-entry RAM in 51 bytes. The record chain pointer changes (`B+C8h..CBh` from `00 00 00 00` to `08 00 51 45`, i.e. `4551:0008`), `B+C7h` goes `00h` to `01h`, the BIOS tick byte `0046C` goes `7Ch` to `98h`, and stack and data bytes change. So the path ran.

## Replay (run-002 trace, run-003 entry and exit)

| Check | Result |
| --- | ---: |
| Annotated memory checks matched | 460,330; `bad` empty (0 mismatches) |
| Interrupt frames (`IRET`) matched | 54 of 54 |
| Hardware interrupt landings replayed (IRQ0) | 27 |
| BIOS tick callbacks (tick word +1) | 27 |
| Callee windows recorded (the seven far calls) | 7 |
| String rows (REP chunks) processed | 3,371 |
| Bytes changed from entry to exit (below A0000) | 51 |
| Differing bytes without a journaled writer | 0 |
| Replayed image vs native exit image, below A0000 | **12 differing bytes** (not zero; see below) |
| Display stores (A0000 and above), counted, not compared | 15,392 |
| Rows executed (body 76; callee segments and stubs the rest) | 4,421,592 |

The 12 differing bytes, replayed versus native exit: `0x0046C` (tick, 97h versus 98h) and 11 bytes in `0x20CAC`–`0x20DEC` (`SS:3B4C`–`SS:3C8C`, below the handler frame): `0x20CAC`, `0x20CAE`, `0x20CB4`, `0x20CB8`, `0x20DA6`, `0x20DA8`, `0x20DA9`, `0x20DAA`, `0x20DAE`, `0x20DEA`, `0x20DEC`.

Cause, as far as it is established: the replayed tick word advances by 27, which matches the 27 IRQ0 landings in the trace. The native exit reads 28 ticks. So the untraced exit run took one more IRQ0 than the traced run. The stack bytes fit an interrupt frame and the BIOS handler's stack use for that extra tick. This is an inference from the counts; the exit run's own trace does not exist, so it is not proven. Timing-dependent interrupts across two runs are the composite method's inherent limit, and this result is the measured case.

Replay-side note: `unexplained 0` means every differing byte has a journaled writer. It does not mean the writer's value matches the native value. The 12-byte difference is a measured mismatch, not a zero.

## C composition (76 instructions; callee effects declared from the trace)

`tests/replay/compose_ovl0038_07e0_body.c` executes the 76 body instructions on this path from the replayed handler-entry state. The body bytes are the pinned file span passed as `argv[1]`, not read from the replayed entry RAM. At each of the seven far calls it pushes the return CS:IP as the body does, then applies the declared callee block (effects file SHA-256 `07db45fd…`, 618,889 bytes), checking the target against the decode. The reference is the replayed post-body snapshot at `2F6B:0022` (SHA-256 `58d20609…`, registers `FFFE FFFF 0000 0000 0D1A 0100 3DDE 3DDE 2F6B 0022 14A0 14A0 1D16 7246`).

| Check | Result |
| --- | --- |
| Baseline: RAM below A0000, C model vs replayed post-body | EQUAL (0 differing bytes; 0 of 14 register fields) |
| Final return | `2F6B:0022`, `SP=3DDE`, `FLAGS=7246`; 76 instructions; 7 calls |

Composition mutants (each must be caught by the comparator; the comparator is `tests/replay/compare_ovl0038_017f_composition.py`, on `origin/main`):

| Mutant | Result |
| --- | --- |
| `RETF 0Ah` changed to `0Ch` | detected: SP differs (3DDE vs 3DE0) |
| Pushed return IP of the 16EC call (call 6, `0907` to `0906`) | **not observed** by this comparator (end state equal) |
| Argument byte of the 16EC call (`AL 0Ah` to `0Bh`, `+08FD`) | **not observed** by this comparator (end state equal) |
| Record byte `+3Eh` set by `MOV byte [es:di+3Eh], 89h` changed to `88h` (`+08C6`) | detected: 1 RAM byte differs |

So the comparator detects 2 of 4 composition mutants. The two undetected are call-boundary words for the 16EC call. They are "not observed by this comparator" and are not shown to be equivalent. The mechanism is not re-derived here. Do not cite the composition for call-site correctness.

## Replay-side mutant triage

Each replay mutant runs through the same driver on the composite trace. A catch needs an annotation, `IRET`, or final-state mismatch, not the rc alone (prefix runs return rc 1 by design, because the exit comparison does not apply to a 500,000-row prefix).

| Run | Catcher | Result |
| --- | --- | --- |
| Control: unmutated driver, 500,000-row prefix | `bad` must be empty | `bad` empty |
| `no-hw-frame`, 500,000-row prefix | `IRET` check | caught: 3 `IRET` mismatches |
| `int-frame-skip`, 500,000-row prefix | annotation and `IRET` checks | caught: 6 annotation, 3 `IRET` |
| `stos-skip`, 500,000-row prefix | annotation check | caught: 32 annotation mismatches |
| `tick-skip`, full trace | final-state comparison | NOT EVALUABLE: the run was stopped at 500,000 rows by operator decision. The unmutated full replay already differs from the native exit by 12 bytes, including the tick word, so a final-state result cannot separate a tick-skip effect from that existing mismatch. |

The composite trace contains interrupt, `IRET`, string and tick rows, so these hooks have sites here. This is catcher evidence on this path only.

## Hygiene and tests

- `tests/overlay/ovl038_07e0_static_test.py --game-ovr <owner GAME.OVR>` (a static test, not a credit checker: it registers no CHECKER_ROWS and claims no credit): 8 of 8 checks PASS (span offset, length and hash; 103 instructions tile the span; one `RETF 0Ah`; the eight static far calls; the seven on the path; the composition callee tables match the decode; the composition source exists). The owner media is read at run time and is not committed.
- `tests/replay/check_ovl0038_07e0_composition.sh BODY RUN_COMP_DIR COMPARATOR OUT`: compiles the composition with `-std=c11 -Wall -Wextra -Werror`, runs the baseline and the four mutants, and compares each with the replayed post-body snapshot. Baseline EQUAL; the verdicts above.
- Byte-dump, pairing and diff-check hygiene: see the commit message.

## Limits

1. **Base.** `origin/main` `9851b7b62be1c00a8e1e0c97acce4fff5d08c1f7`.
2. **Composite method.** The trace (run 002) and the exit (run 003) come from different runs. Interrupt timing differs between traced and untraced runs, so the IRQ0 count differs by one here, which gives the 12-byte replay-to-native mismatch. Not closed.
3. **Replay not exact.** Store replay reports 0 unattributed bytes but 12 differing bytes against the native exit. The exact-replay claim is not made.
4. **Composition scope.** The composition checks the end state and the final return only. It does not verify call-site values. Two of four composition mutants are not observed by it. The composition's reference is the replayed post-body snapshot, not the native exit, so it does not absorb the 12-byte mismatch.
5. **One path.** Selector `0`, `B+C8h` null, `B+C7h` below `10h`, flag `0`. The selector-non-zero path (which makes the `0852` call) and the walk path are not covered.
6. **Operator-set state.** `B+C7h` and `B+C8h` were set by the operator before the cold call and before each warm entry. The frame values are operator choices.
7. **Callees.** The seven far callees are not modelled. Their effects are declared from the same trace, so the composition is not independent of the trace.
8. **Premise.** The base has no 07E0 model or grant. The entry stays uncredited. Nothing here grants or claims credit.
9. **Debugger.** LOGL-traced runs with breakpoints and counts were unreliable in earlier slices (run 001 of this entry was aborted for the operator-state reason above, not for the debugger). Not diagnosed here.
10. **Placeholder authorship.** The commits carry the project placeholder identity (`reviewer-1 (maintainer rebase)`, `agents.local`). Re-attribute before any public release.
11. **Owner media.** The audit and the composition read the owner `GAME.OVR` at run time. No body bytes are committed.
12. **Evidence only.** No parity, function-identity, count or credit claim.
