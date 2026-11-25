# OVL_0038_0A65 (U38 slot 30): composite in-game evidence

**Verdict: composite evidence, not an exact replay.** The trace comes from one run (run 007) and the entry and exit come from another (run 003). The 0233 store replay of the trace on the run-003 entry is not clean. Five annotated reads disagree with the replayed memory (run-003 entry plus replayed writes); four final bytes disagree with the run-003 native exit. The five annotated mismatches are caused by the entry difference (patched control: ann-bad 0). The four final bytes lie in the same 379-byte set; the chain to 20EE9 is traced (SS:3D7D -> AL -> SS:3D89, trace lines 649 -> 653), and the chains to 20EEA, 20EEB and 20EF8 are open (#1985). The C composition from the replayed entry state (baseline) is EQUAL, and all five composition mutants are detected. Replay mutants are vacuous on this trace: it contains no interrupt, string, tick or callback sites.

This is evidence only. It makes no parity, function-identity, count or credit claim.

## Premise (checked on `origin/main` before any capture)

Base `origin/main` `1a4a15b98f0952d36a010968dfc4dc9cb84d7692` (the merge commit of PR #1982).

- `src/recovered/overlay_u9e_dispatch_0000_cpu.c` (slot-30 row) marks the entry `K_GAP_UNCREDITED`.
- `docs/re/OVL_009E_0000_HANDLERS.md` slot 30 is a "named gap: uncredited handler OVL_0038_0A65".
- `docs/re/credit_evidence_kind.csv` has no row for OVL_0038_0A65.
- `docs/re/RES_00BA_16EC_PORT_HARNESS.md` lists +0A65 only as a caller of `00BA:16EC`.

The entry is uncredited and unmodelled on the base, so this slice proceeds.

## Handler facts (static; owner media, not committed)

- Span `[0A65, 0B1B)` in unit 38h, 182 bytes, SHA-256 `0d704deb92dab5d9608e8630cf29dcb59e529b14f677855518cffdcad0daf31c`.
- Code body `[0A65, 0B11)`, 172 bytes, SHA-256 `6e01f0bd068676f7cce373b86d65d381ae966a4049d86637537bfc9827f48886`, tiles into 54 instructions with one `RETF 0Ah` at `+0B0E`, the last instruction.
- The 10 bytes after the body are a length-prefixed data block (length byte 09h). They are not code and are not committed.
- Argument layout: selector word at `[BP+6]`; pointer A at `[BP+8]` (unused on the path below); record pointer B at `[BP+0Ch]`.
- Record field `B+108h` holds a far pointer Q. The body reads `Q+2`: if non-zero it takes path A (a callback call `0AF8:0B0B` with the callback `0038:0A59`, then `00BA:16EC`); if zero it takes path B.
- Static far calls: `0A8A 0AF8:0B0B` and `0A95 00BA:16EC` (path A only); `0ABA 00BA:0BB8` (both paths); `0B06 00BA:09E3` (only when P equals the far word at `DS:5D92:5D94`).

## Path exercised

Path B. `B+108h` = `14A0:0800` and `[14A0:0802] = 00`, so the body takes the zero branch. The word at `DS:5D92` is `0008h` and the word at `DS:5D94` is `449Ah` (memory order), so neither matches P (`14A0:0100`), and `00BA:09E3` is not called. The trace makes one far call, to `00BA:0BB8`. Path A and the conditional call are not exercised.

## Method (composite, labelled)

1. **Trace, run `ovl38-0a65-native-007`.** Timed recipe from the 07E0 run-002 spacing: RUN, Return at +20 s, `l` at +25 s, `a` at +6 s, Alt+Pause at +50 s. Cold call untraced to `9FE0:0100`, then the warm call traced under `LOGL 200000` (hex, 2,097,152 lines). The first part reached the return row `9FE0:0100` at line 846. The part was cut at that row. The trace logs on past the return, because the breakpoint was deleted before the warm call. The cut has 846 lines, SHA-256 `38ed0c4c50782bf7bb1d2a4d066cebf51e2169be45c15008d14ed152f4e12f9b`. The uncut 2,097,152-line part has SHA-256 `33ebe59226597c0722039ef76f3e149341c039f2d566c953591c4096fe0a1543`.
2. **Entry and exit, run `ovl38-0a65-native-003`.** WARM-ENTRY RAM (`warm/S0..S9.BIN`, 640 KiB, concatenated SHA-256 `ea2ce73413d6d5b8ca842947ec963de8f4b1c41b9432a5e67b0db23ddfb12deb`) and WARM-EXIT RAM (`warm/exit/S0..S9.BIN`, SHA-256 `7dcc2a2e65f5200ee072c9c973e9274fe671fae7333059cdd1942438bc605291`). The exit stop is `9FE0:0100`. The 846-line trace cut was staged into run 003's `warm/parts/LOGCPU-part1.txt`, which is byte-identical to the first 846 lines of run 007's uncut LOGCPU part (SHA-256 `38ed0c4c…`).
3. **Replay.** The 0233 driver on `origin/main`, unmodified, applies the run-007 cut trace to the run-003 entry and compares with the run-003 exit:
   `--body-seg 30FF --entry-ip 0A65 --post-retf-ip 0B0E`.
4. **Composition.** `tests/replay/compose_ovl0038_0a65_body.c` runs the body bytes (from the owner GAME.OVR, extracted to scratch and checked against the body SHA-256 above) from the replayed entry state. The declared callee effects come from the replay. The comparator is `tests/replay/compare_ovl0038_017f_composition.py`, from `origin/main`.

## Operator writes (every write, from the run-007 log)

- `B+108h := 14A0:0800`. As found it was `0000:0000`.
- `[14A0:0802] := 00`. As found it was `65`. Path B requires 00. Run 003 made no write to this byte and observed it as 00 at its warm entry; run 007 wrote it before the cold call so the pre-call state matches path B.
- Frame: `SS:SP 1D16:3DE0`, return `9FE0:0100`, selector `0000`, pointer A `14A0:0100`, pointer B `14A0:0100`, index word `001E` at `SS:3DEE`; registers `AX=BX=001E`, `DS=14A0`, `CS:IP=08D4:0025`.
- Breakpoint `9FE0:0100` for the cold call, then deleted with `BPDEL *` before the traced warm call.
- Restore of the pre-cold record bytes: eight `SM` writes covering the changed bytes of record `14A0:0100` (the cold call changed them; the restore put them back).
- Frame re-applied before the warm call.

Run 003 (the entry and exit run) made one record write before its cold call, `B+108h := 14A0:0800` (log time 00:18:10Z), then the frame and the breakpoint. Its `record-pre.bin` was dumped at 16:16:06 PST (00:16:06Z), before that write, so it is the as-found state. Run 003's action log has DUMP_MISSING markers for `record-pre.bin` and `q-pre.bin`, and both files exist now. Run 007's `record-pre.bin` was re-dumped after its `B+108h` write, so it is post-B+108-write and not the as-found state; the restore is correct relative to that file.

`B+C7` and `B+C8` were not set. This is deliberate, because the 0A65 body does not read them. The 07E0 operator state (`B+C7 = 00h`, `B+C8 = 0000:0000`) is not used here. The callee does write `B+C7` (see "Callee effect on B+C7" below).

## Results

### Replay control (unmodified 0233 driver, run-003 entry and exit, run-007 cut trace)

| Measure | Result |
| --- | ---: |
| Annotated memory checks matched | 426 |
| Annotated memory checks mismatched (`ann-bad`) | 5 |
| Bytes changed entry to exit below A0000 | 38 |
| Replayed image vs native exit, below A0000 | 4 differing bytes |
| Differing bytes without a journaled writer | 0 |
| Rows executed by segment | body 33; callee `08DE` 794; `2F6B` 16; `0865` 1; `08D4` 1 |
| Callee windows | 1 |

The five annotated mismatches are rows 454, 648, 652, 653 and 675, at stack addresses `SS:3D7D`, `SS:3D89`, `SS:3D8A` and `SS:3D98` (linear `20EDD`, `20EE9`, `20EEA`, `20EF8`). The four differing final bytes are linear `20EE9`, `20EEA`, `20EEB` and `20EF8`. Row numbers 454, 648, 652, 653, 675 are 0-based driver indices; trace line = row + 1.

### Cause: entry-image difference (measured)

Run 003 and run 007 entry RAM differ in 379 bytes over the 640 KiB below A0000. The set includes the tick word `0046C`–`0046E`, the stale stack bytes `2016D`–`20FA3` and others. The five annotated addresses and the four final bytes all lie in this set. one callee stack write (trace line 653, SS:3D89) takes its value from a read of SS:3D7D (line 649); SS:3D7D differs between the runs. The run-003 native exit keeps its own values there.

### Control with the entry difference patched out (diagnostic copy of the driver)

The run-003 entry with the 379 bytes replaced by the run-007 values gives `ann-bad` 0 and 431 matches. That confirms the five annotated mismatches come only from the entry difference. The exit-side count rises to 379, of which 375 have no journaled writer, because the trace does not rewrite those bytes and they keep the patched values. This control validates annotations only and is not a clean final-state match. The committed evidence uses the unpatched control. The patched run used a diagnostic copy of the driver with prints added, not committed.

### Composition (baseline and mutants)

Baseline (`none`): EQUAL. 33 body instructions, 1 declared far call, final `2F6B:0022`, `SP=3DDE`, `FLAGS=7297`, compared against the replayed post-body snapshot. Check script: `tests/replay/check_ovl0038_0a65_composition.sh`.

| Mutant | Site (body offset) | Verdict |
| --- | --- | --- |
| `retf-pop-0C` | `RETF 0Ah` immediate, `+0B0F` | DIFFERS (`SP` 3DE0 vs 3DDE) |
| `retaddr-call` | pushed return IP of the `00BA:0BB8` call | DIFFERS |
| `arg-seg-0BB8` | `push word [BP+0Eh]` instruction at `+0AB4`; displacement patched at `+0AB6` | DIFFERS |
| `sub-112-value` | `sub byte [ES:DI+112h], 2` immediate, `+0AD2` | DIFFERS |
| `zero-Q2-value` | `mov byte [ES:DI+2], 0` immediate, `+0AA6` | DIFFERS |

All five composition mutants are killed by the comparator. None survive.

### Replay mutants (0233 driver hooks `POOLRAD_0233_MUTANT`)

| Mutant | Verdict | Mechanism |
| --- | --- | --- |
| `no-hw-frame` | vacuous | no hardware-interrupt landings in the trace |
| `int-frame-skip` | vacuous | no `INT` or `IRET` rows in the trace |
| `stos-skip` | vacuous | no `REP`, `STOS` or `MOVS` rows in the trace |
| `tick-skip` | vacuous | no writes to the BIOS tick word `0040:006C` |

Each mutant produces a summary identical to the control (`ann-bad` 5, diff 4, unexplained 0). The trace has zero `IRET`, zero `INT`, zero `REP`/`STOS`, zero tick writes and zero callbacks, so none of these hooks has a site on this path. They are not catches and are not evidence of sensitivity.

### Registers at the return row

| Register | Trace, row 846 (`9FE0:0100`) | Run 003 WARM-EXIT |
| --- | --- | --- |
| AX | 0100 | 0100 |
| BX | 0000 | FFFF |
| CX | 0000 | 0000 |
| DX | 14A0 | 14A0 |
| SI | 0011 | 0D1A |
| DI | 0100 | 0100 |
| BP | 3E98 | 3E98 |
| SP | 3DF0 | 3DF0 |
| FLAGS | 7297 | 7297 |

BX and SI differ. The replay takes its registers from the trace, so the replayed post-state does not carry run 003's BX and SI. The RAM comparison is the evidence, and the register difference is recorded as a limit. The run-003 entry SI was `0D1A`, and the run-007 entry SI was `0011`.

### Callee effect on B+C7

The callee `00BA:0BB8` writes `B+C7`. In the trace, `mov byte es:[C7h], 00` at `08DE:0BED` sets it to 00, and `inc byte es:[C7h]` at `08DE:0C2C` makes it 01. That matches the run-003 exit, which changes `B+C7` from `74h` (entry) to `01h` (exit). The operator did not set `B+C7`, so this is a callee effect the evidence records, not a value the operator chose.

## Limits

1. **Composite method.** The trace is from run 007 and the entry and exit from run 003. The two runs have different entry images (379 bytes), so the replay is not clean (`ann-bad` 5, diff 4, unexplained 0). The control with the difference patched out gives `ann-bad` 0.
2. **Register difference at return.** `BX` and `SI` differ between the trace-run return and the run-003 exit. The trace-run entry `SI` is `0011`, and run 003's is `0D1A`.
3. **Operator writes.** The writes are listed above. The eight restore writes come from a diff of record `14A0:0100` against the cold-call state. Bytes outside that record that the cold call changed were not restored, and they are part of the run-007 entry.
4. **Callee effect on B+C7.** The callee writes `B+C7`, so the operator state differs from the 07E0 recipe (which sets `B+C7`). This is recorded, not suppressed.
5. **Supervisor timeout.** The run-007 unit ended with `returncode=124` from the supervisor's 1800 s deadline, after DONE. This is not a capture failure. The outer process returned 125 after the timeout.
6. **Run-007 S2 dump.** `warm/S2.BIN` of run 007 was 4,096 bytes at its WARM-ENTRY capture and is 65,536 bytes now; its tail is unverified. Of the 379 differing bytes, 364 lie in S2 below offset 4,096 and 15 lie in S0 and S1; none lies at S2 offset 4,096 or above.
7. **One path.** Path B only (`[14A0:0802] = 00`, `B+108h = 14A0:0800`). Path A, the conditional `00BA:09E3` call, and non-zero selectors are not exercised.
8. **Replay mutants vacuous.** The four replay hooks have no sites on this trace, which executes 845 rows (see the table). They are not catches.
9. **Replayed rows.** The replay executes 845 rows: 33 body, 794 callee at `08DE`, 16 dispatcher at `2F6B`, and one each at `0865` and `08D4`. The cut has 846 lines; the 846th is the stop row at `9FE0:0100`, which the driver does not execute.
10. **Placeholder authorship.** The commits carry the project placeholder identity. Re-attribute before any public release.
11. **Owner media.** The body bytes are extracted from the owner `GAME.OVR` at run time and are not committed.
12. **Evidence only.** No parity, function-identity, count or credit claim.
13. **Live versus static body.** The live body in run 003 RAM differs from the GAME.OVR file body at 8 bytes, the segment words of the four far calls (`+0A8D/8E`, `+0A98/99`, `+0ABD/BE`, `+0B09/0A`). The trace executes the live target `08DE:0BB8` where the file says `00BA:0BB8`. The composition runs on the static file body, and the end state does not depend on those words. The composition's callee targets are file labels; live callee identity is not established.
14. **Open follow-up #1984.** The origin of the 379-byte entry-image difference is not established. The doc measures the difference and shows that the trace reads or writes five of these bytes (annotated addresses 20EDD, 20EE9, 20EEA, 20EEB, 20EF8); it does not claim an origin.
15. **Open follow-up #1985.** Traced: final byte 20EE9 via SS:3D7D -> AL -> SS:3D89 (trace lines 649 -> 653). Not traced: 20EEA and 20EEB (write at line 675) and 20EF8 (write at line 454); BX/SI attribution. The doc states set membership and the control result only.

## Hygiene

- `tests/overlay/ovl038_0a65_static_test.py`: 9/9 PASS against the owner GAME.OVR.
- `tests/replay/check_ovl0038_0a65_composition.sh`: compiles with `-std=c11 -Wall -Wextra -Werror`; baseline EQUAL; five mutants DIFFER.
- `tools/overlay/check_byte_dumps.py` runs on this document; this document carries digests and addresses only.
