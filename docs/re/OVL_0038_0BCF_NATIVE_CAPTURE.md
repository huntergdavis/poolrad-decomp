# OVL_0038_0BCF (U38 slot 34): composite in-game evidence

**Verdict: composite evidence, not an exact replay.** The trace comes from run 003 and the entry and exit come from run 002. The unmodified 0233 driver on `origin/main` (sha256 `aff2969a2bc31037bddbd7ff59f54c1c4321ad284a32cb3e4fbc9e9578dc53bc`) gives: annotated memory checks 437,335 matched and 0 mismatched; 79 bytes changed from entry to exit below A0000; 13 replayed-versus-native-exit differences; 0 differences without a journaled writer. The 13 differences have BIOS interrupt machinery as their last writers (see the attribution table). They are not game-code effects, and the replay's value for them is not claimed to be correct.

The composition baseline is EQUAL. Of the four original composition mutants, **1 of 4 composition mutants detected; 3 survived** (`retaddr-call`, `item-2B-2C`, `sel-disp-08`; mechanisms below). Two added second-call mutants are detected.

Replay mutants (unmodified driver): `no-hw-frame`, `int-frame-skip` and `stos-skip` are detected by annotation and IRET checks. `tick-skip` is detected **only** through one unjournaled difference (control: 0); it produces no annotation mismatch.

This is evidence only. It makes no parity, function-identity or credit claim. The reconciliation classification of this entry is unresolved and is not claimed here (see LIMITS).

## Premise (checked on `origin/main` `1a4a15b98f0952d36a010968dfc4dc9cb84d7692` before capture)

- `src/recovered/overlay_u9e_dispatch_0000_cpu.c:82` marks slot 34 `{K_GAP_UNCREDITED, 0x00B1, 0x0041, 0x0BCF}`.
- `docs/re/OVL_009E_0000_HANDLERS.md:82` lists slot 34, table `68B0h`, stub `0041:00B1`, as "named gap: uncredited handler OVL_0038_0BCF".
- `docs/re/U9E_DISPATCH_HANDLER_BACKLOG.md:73` is unchecked ("slots 34; missing").
- `docs/re/u9e_dispatch_handlers.json` record for slot 34: `canonical_strict_row: false`, `interface: "missing"`, empty `strict_evidence`.
- `docs/re/credit_evidence_kind.csv`, `strict_full_c_addresses.csv`, `reviewed_function_identities.csv` and `replaced_bodies.json` have no 0BCF row.
- No file under `src/recovered` or `include` models `OVL_0038_0BCF`.

## Slot, table entry and entry state

- Slot 34 uses the 009E convention: table address `DS:6838h + 4 * (slot - 4)` = `DS:68B0h`.
- After the cold call, `DS:68B0` holds a far pointer to `0865:00B1`, and the five bytes at that stub are `EA CF 0B FF 30`, a far jump to `30FF:0BCF` (run 002, `stub34-after-cold.bin`). The table gate passed empirically.
- The public-entry number 29 (`docs/re/OVERLAY_UNIT38_EFFECT_ENTRIES.md`, entry 29 at `+0BCF`) is a different list. The ledger's `entry_slot` column reads 29 for this entry, while the table index is 30. That bookkeeping mismatch is not resolved here.

## Handler facts (static; owner media, not committed)

- Span `[0BCF, 0C06)` in unit 38h: 55 bytes, file offset 63530, SHA-256 `9f1ec667c9bd2c225500980e91f7ea7112afcd9858ac407b2433fbd8c23dec4a`.
- 22 instructions tile the span. There are no branches and no record-field reads. The only read is the selector byte `[BP+6]`, twice.
- `push bp; mov bp,sp; mov al,2Bh; push ax; push [BP+0Eh]; push [BP+0Ch]; push [BP+0Ah]; push [BP+8]; mov al,[BP+6]; push ax; lcall 00B0:0025`, then the same sequence with item `2Ch` and a second `lcall 00B0:0025`, then `pop bp; retf 0Ah`.
- `RETF 0Ah` is at `+0C03`, the last instruction. The static test checks these facts (`tests/overlay/ovl038_0bcf_static_test.py`, 8/8 PASS).

## Live-versus-file fixups (disclosed)

The live body in run 002's entry RAM differs from the file in exactly four bytes: the far-call segment words at `+0BE8–0BE9` and `+0C00–0C01`. The file says `00B0`; live is `08D4`. The composition runs on the static file body, and its callee targets are file labels. Live callee identity is not established by this doc.

## Stub-target load state at both far calls

Both calls go to a loaded stub. At each call the live operand is `08D4:0025`, and the bytes there are a far jump to `2F6B:0000`; neither call takes the INT 3Fh loader path.

- First call: `30FF:0BE5` `call 08D4:0025` at trace part 1 line 27 (`ESP=3DC2`); the stub at line 28 is `jmp 2F6B:0000`.
- Second call: `30FF:0BFD` `call 08D4:0025` at trace part 2 line 2,082,994 (`ESP=3DC2`); the stub at line 2,082,995 is `jmp 2F6B:0000`.

Each call runs the dispatcher at `2F6B`, which in turn runs nested unit-38 code (`30FF:004A`, `30FF:111B`, `30FF:1126` and others, with their own far calls). The effect blocks used by the composition are the top-level windows of the body calls (see the diagnostic-copy LIMIT).

## Method (composite, labelled)

1. **Entry and exit, run `ovl38-0bcf-native-002`.** Timed recipe (RUN, Return at +20 s, `l` at +25 s, `a` at +6 s, Alt+Pause at +50 s), with a boot-RUN resume (the first check at +25 s read `F000:FFF0`; RUN #2 read `F000:D10B`). Frame set; cold call untraced to `9FE0:0100` (return state `SP=3DF0`, `DS=14A0`); table gate; WARM-ENTRY S0–S9 (10 files of 65,536 bytes); warm call untraced to `9FE0:0100`; WARM-EXIT S0–S9. Measured boot RUN to DONE: 240 s.
2. **Trace, run `ovl38-0bcf-native-003`.** Same procedure; the warm call is traced under `LOGL 200000` (hex). Part 1 is 2,097,152 lines. Part 2 reaches the first `9FE0:0100` row at line 2,083,301, which is the cut point; the stop row is line 2,083,301 of part 2. Measured boot RUN to DONE: 313 s.
3. **Replay.** Unmodified 0233 driver, `--body-seg 30FF --entry-ip 0BCF --post-retf-ip 0C03`, entry and exit from run 002, trace parts from run 003 (part 2 cut at line 2,083,301). The stop-duplicate rule drops each part's last row, so rows executed are 2,097,151 (part 1) plus 2,083,300 (part 2) = 4,180,451.
4. **Composition.** `tests/replay/compose_ovl0038_0bcf_body.c` runs the 55 file bytes from the replayed entry state, with the two body-level callee blocks declared from the diagnostic copy's top-level windows (see LIMITS).

## Operator writes (every write)

- Frame: `SS:SP 1D16:3DE0`; words at `SS:3DE0` return IP `0100` and `SS:3DE2` return CS `9FE0`; selector `0000`; pointer A `14A0:0100`; pointer B `14A0:0100`; index word `0022` at `SS:3DEE`.
- Registers: `SS=1D16 SP=3DE0 DS=14A0 CS=08D4 IP=0025 AX=0022 BX=0022`.
- Breakpoint `9FE0:0100` for the cold call. Frame re-applied before the warm call.
- Trace run only: `BPDEL *` before the traced warm call.
- No record write: the body reads no record field, so `B+108h` and `B+C7` were not set, and no restore step was needed.
- Run 002's first keyboard stop read `1264:0302`, with no further operator input before the frame.

## Results (unmodified driver)

| Measure | Result |
| --- | ---: |
| Annotated memory checks matched | 437,335 |
| Annotated memory checks mismatched (`ann-bad`) | 0 |
| Bytes changed entry to exit below A0000 | 79 |
| Replayed image vs native exit, below A0000 | 13 differing bytes |
| Differing bytes without a journaled writer | 0 |
| Rows executed | 4,180,451 (body 133; `0865` 3; `08D4` 5; `08DE` 188; `0C15` 1,471; `0C4C` 92; `0C88` 106,812; `0F2D` 85,688; `1264` 3,971,868; `131C` 6,529; `223B` 7,084; `2F6B` 188; `F000` 390) |
| Callee windows (unmodified driver) | 5 |
| Hardware-interrupt entries / interrupt frames / ROM-stub pushes | 26 / 156 / 156 |
| Tick callbacks | 26 |
| String rows | 3,308 |

### Attribution of the 13 replayed-versus-exit differences

The writer field is the replay's last write to each byte, given as `(row, kind, class)`. Rows are 0-based driver rows; trace line = row + 1 in part 1, and part-2 line = row − 2,097,150 in part 2 (the cut file). The lines were read from the trace.

| Byte | Entry | Replayed | Native exit | Last writer (row, kind, class) | Trace line and instruction |
| --- | --- | --- | --- | --- | --- |
| `0046C` | E2 | FC | FB | 4,178,866, bios-tick, bios-callback | part 2 line 2,081,716: `F000:FEA6` callback 0008 (IRQ 0 Clock) |
| `20C8E` | 0B | 06 | 46 | 1,407,085, int, int-frame | part 1 line 1,407,086: `F000:FEAD` `int 1Ch` |
| `20C90` | 04 | 23 | 1D | 1,407,084, push, rom-stub | part 1 line 1,407,085: `F000:FEAC` `push dx` |
| `20C96` | 2D | C0 | BD | 1,407,080, irq-frame, hw-frame | part 1 line 1,407,081: `F000:FEA5` `sti` |
| `20C9A` | 64 | 06 | 46 | 1,407,080, irq-frame, hw-frame | part 1 line 1,407,081: `F000:FEA5` `sti` |
| `20D98` | 00 | AF | 00 | 4,178,870, int, int-frame | part 2 line 2,081,720: `F000:FEAD` `int 1Ch` |
| `20D99` | B7 | FE | B7 | 4,178,870, int, int-frame | part 2 line 2,081,720: `F000:FEAD` `int 1Ch` |
| `20D9B` | 00 | F0 | 00 | 4,178,870, int, int-frame | part 2 line 2,081,720: `F000:FEAD` `int 1Ch` |
| `20D9C` | 00 | 03 | 00 | 4,178,870, int, int-frame | part 2 line 2,081,720: `F000:FEAD` `int 1Ch` |
| `20D9D` | E1 | 72 | E1 | 4,178,870, int, int-frame | part 2 line 2,081,720: `F000:FEAD` `int 1Ch` |
| `20D9E` | 3D | 01 | 3D | 4,178,869, push, rom-stub | part 2 line 2,081,719: `F000:FEAC` `push dx` |
| `20D9F` | 03 | 00 | 03 | 4,178,869, push, rom-stub | part 2 line 2,081,719: `F000:FEAC` `push dx` |
| `20DCE` | 06 | 36 | 30 | 4,017,612, push, rom-stub | part 2 line 1,920,462: `F000:FEAC` `push dx` |

All 13 last writers are BIOS interrupt machinery at `F000:FEA5–FEAD` (the IRQ0 path, `sti`, `push dx`, `int 1Ch`) or the IRQ0 clock callback that writes the tick word. None is a game-code instruction. The replay wrote these bytes through its hardware model. The value it wrote is not the native exit's value, and the reason for that difference is not established. The trace does not show the native run's interrupt timing, so this doc does not attribute the difference to any cause.

### Replay mutants (unmodified driver; control: `bad {}`, 13 diffs, 0 unjournaled)

| Mutant | Result | Verdict |
| --- | --- | --- |
| `no-hw-frame` | `bad {iret-bad: 26}`, 17 diffs, 0 unjournaled | DETECTED (IRET checks) |
| `int-frame-skip` | `bad {ann-bad: 23, iret-bad: 26}`, 28 diffs, 0 unjournaled | DETECTED (annotation and IRET checks) |
| `stos-skip` | `bad {ann-bad: 44}`, 33 diffs, 0 unjournaled | DETECTED (annotation checks) |
| `tick-skip` | `bad {}` (no annotation mismatch), 13 diffs, **1 unjournaled diff** | DETECTED only through the unjournaled count, a weak signal; not a clean annotation detection |

### Composition (declared callee effects from the diagnostic copy; see LIMITS)

| Mutant | Site (body offset) | Verdict |
| --- | --- | --- |
| `none` (baseline) | | EQUAL (22 instructions, 2 declared calls, final `2F6B:0022`, `SP=3DDE`, `FLAGS=7202`) |
| `retf-pop-0C` | `RETF 0Ah` immediate, `+0C04` | DIFFERS (`SP` 3DE0 vs 3DDE) |
| `retaddr-call` | pushed return IP of the first call | EQUAL (survived) |
| `item-2B-2C` | first `mov al,2Bh` immediate, `+0BD3` | EQUAL (survived) |
| `sel-disp-08` | `mov al,[BP+6]` displacement, `+0BE3` | EQUAL (survived) |
| `item-2nd-2B` (added) | second `mov al,2Ch` immediate, `+0BEB` | DIFFERS |
| `retaddr-2nd` (added) | pushed return IP of the second call | DIFFERS |

Summary: 1 of 4 original composition mutants detected; 3 survived. The two added second-call mutants are detected.

### Survivor mechanisms

- **`retaddr-call` (survived).** The first call's pushed return IP is overwritten before the body returns. Both calls run with `ESP=3DC2` (lines 27 and 2,082,994), so both lcalls push their return IP to `SS:3DBE`. The first return lands at `30FF:0BEA` at line 2,082,986 (`ESP=3DCE`). The second call pushes `0C02` to the same slot, and the second return lands at `30FF:0C02` at line 2,083,297. The final byte at `SS:3DBE` is the second call's `02`, so the first call's value is not observable at the end state. This is a trace-line-based explanation; the composition does not execute the callee.
- **`item-2B-2C` (survived).** The first `push ax` (`30FF:0BE4`, line 26) pushes `2Bh` to `SS:3DCC`, and the dispatcher reads `002B` there at line 37. The second `push ax` (`30FF:0BEC`) pushes `A02Ch` to the same slot, and the dispatcher reads `A02C` at part-2 line 2,083,004. The final value at `SS:3DCC` is the second call's, so the first call's item value is not observable at the end state.
- **`sel-disp-08` (survived; state-indistinguishable here).** The first selector read (`30FF:0BE1`, line 25) reads `SS:3DD4 = 0000`. The mutant would read `[BP+8]` instead (`30FF:0BDE`, line 24, `SS:3DD6 = 0100`). Both low bytes are `00`, so in this entry the mutant does not change the pushed value. This is state-indistinguishable on this trace, not equivalent in general.

## LIMITS

1. **Composite method.** The trace (run 003) and the entry and exit (run 002) come from different runs. Their WARM-ENTRY RAM differs in 49 bytes below A0000, including the tick word `0046C` (measured by comparing the two runs' `warm/S0..S9.BIN`). The replay is not clean at 13 bytes; this doc does not claim the replay reproduces the native exit.
2. **13 BIOS-attributed bytes.** Their last writers are interrupt-machinery and ROM-stub pushes (`F000:FEA5–FEAD`) or the IRQ0 clock callback (`0046C`). They are not game-code effects. The replay's value for them is a model output, not a verified native value. A "last writer is an interrupt frame" attribution means only that the replay wrote the byte through its hardware model, not that the value is correct. The reason the native exit differs is not established.
3. **Diagnostic copy of the driver.** The composition's declared callee effects (the `CALL 0` and `CALL 1` blocks) come from a stack-aware diagnostic copy of the 0233 driver, not from the unmodified driver. The unmodified driver's single window slot overwrites nested windows, so its window list has 5 entries where the body makes 2 calls. The diagnostic copy has sha256 `30a72557dbfffdb50493a01c8418251cc94f0452ec68888a614dff5229799c10`; it is not committed. It changes only the window logic and adds diagnostic prints. Its headline numbers equal the unmodified driver's on every field (`bad`, checks, changed bytes, diffs, unjournaled count, row counts, journal classes); its window count is 7. Its exact diff against the unmodified driver is shown below (87 lines, sha256 `f6640541867aa964b0ca78d1939e9b6809af7b2a1be0e01cc8758740a5bd77b2`; the path headers are replaced by labels, the hunks are verbatim).
4. **Classification unresolved.** The reconciliation row for this entry reads `game_or_other_library_unresolved` with `game_ownership: unknown`. This slice makes no ownership claim and no classification change, and it does not state whether the body is game code or library code.
5. **Run location.** The runs are stored under the 0A65 chain's run root, `ovl0038-0a65/runs` (`ovl38-0bcf-native-002` and `-003`), not under an `ovl0038-0bcf` runs tree. The approved chain's run-directory guard accepts only that root, and the chain is unchanged. Attempt 001 was aborted by a warm-entry gate that wrongly required `ES=14A0`, before any exit was captured; its directory is kept outside the run tree.
6. **Operator writes** are listed above. No record bytes were written, so the warm entry is the state after the cold call.
7. **Live-versus-file fixups** are the four segment-word bytes listed above. The composition's callee targets are file labels; live callee identity is not established.
8. **Nested callee code is not modelled.** The composition applies declared effect blocks for the body's two calls. It does not execute the stub, the dispatcher or the nested unit-38 code.
9. **Composition scope.** The composition compares the end RAM below A0000 and the registers with the replayed post-body state. It does not verify call-site correctness, and it does not validate the callee's behaviour.
10. **Survivors.** Three original composition mutants survived (mechanisms above). Their survival is an observability limit of the end-state comparison, not a claim of equivalence.
11. **`tick-skip` detection** is weak: one unjournaled difference and no annotation mismatch.
12. **Ledger bookkeeping.** The ledger's `entry_slot` reads 29 and the table index is 30. Not reconciled here.
13. **Placeholder authorship.** The commits carry the project placeholder identity. Re-attribute before any public release.
14. **Owner media.** The body bytes are extracted from the owner `GAME.OVR` at run time and are not committed.
15. **Evidence only.** No parity, function-identity, count or credit claim.

## Diagnostic copy: exact diff against the unmodified driver

`diff -u` output, path headers replaced by labels (`unmodified` = `tests/replay/replay_ovl0038_0233_warm.py`, blob `aff2969a…`; `diagnostic` = the stack-aware copy, sha256 `30a72557…`):

```diff
--- unmodified
+++ diagnostic
@@ -96,6 +96,9 @@

     mem, vcount = {}, [0]
     jcount, written = collections.Counter(), set()
+    lastw = {}
+    annbad_detail = []
+    globals()["DBG"] = {"lastw": lastw, "ann": annbad_detail}
     taint = [False]
     label = ["body"]
     checks = collections.Counter()
@@ -113,6 +116,7 @@
         return rd(a) | (rd(a + 1) << 8)

     active = [None]
+    stk = []

     def wr(i, a, n, v, kind, cls):
         a &= 0xFFFFF
@@ -122,10 +126,11 @@
                 vcount[0] += 1
                 continue
             mem[b] = byte
-            if active[0] is not None:
-                active[0]['writes'].append((b, byte))
+            for w_ in stk:
+                w_['writes'].append((b, byte))
             jcount[(cls, kind)] += 1
             written.add(b)
+            lastw[b] = (i, kind, cls, byte)

     ids = {X.X86_REG_AX: ("ax", 0xFFFF, 0), X.X86_REG_BX: ("bx", 0xFFFF, 0), X.X86_REG_CX: ("cx", 0xFFFF, 0),
            X.X86_REG_DX: ("dx", 0xFFFF, 0), X.X86_REG_SI: ("si", 0xFFFF, 0), X.X86_REG_DI: ("di", 0xFFFF, 0),
@@ -221,9 +226,8 @@
                 snaps['entry'] = (dict(r, ip=ip), flags, bytes(mem.get(a, base[a]) for a in range(VIDEO_BASE)))
             if 'post' not in snaps and prv is not None and prv[0] == BODY_SEG_RT[0] and prv[1] == POST_RETF_IP[0] and mnemonic(prv[2]) == 'retf':
                 snaps['post'] = (dict(r, ip=ip), flags, bytes(mem.get(a, base[a]) for a in range(VIDEO_BASE)))
-        if active[0] is not None and cs == BODY_SEG_RT[0] and ip == active[0]['ret']:
-            active[0]['regs'] = dict(r); active[0]['flags'] = flags; active[0]['sp'] = r['sp']
-            windows_out.append(active[0]); active[0] = None
+        while stk and cs == BODY_SEG_RT[0] and ip == stk[-1]['ret']:
+            top_ = stk.pop(); top_['regs'] = dict(r); top_['flags'] = flags; top_['sp'] = r['sp']; windows_out.append(top_)
         sspb, sp = r["ss"] << 4, r["sp"]
         cls = "rom-stub" if cs == 0xF000 else "game"
         label[0] = "body" if cs == BODY_SEG_RT[0] else "cs:%04X" % cs
@@ -240,7 +244,7 @@
                 if got == int(hexv, 16).to_bytes(len(hexv) // 2, "little"):
                     checks["ann-match"] += 1
                 else:
-                    bad["ann-bad"].append(i)
+                    bad["ann-bad"].append(i); annbad_detail.append((i, text[:110], "%05X" % a, hexv, got.hex()))
         # hardware interrupt entry: the row after an instruction is its resolved successor unless an interrupt landed
         if prv is not None and not prv[2].startswith("callback"):
             pins = dis(prv[3], prv[1])
@@ -352,7 +356,7 @@
                 raise SystemExit("unsupported memory-destination form %s at row %d" % (mn, i))
             wr(i, a, size, new & ((1 << (8 * size)) - 1), mn, cls)
         if mn == "lcall" and cs == BODY_SEG_RT[0]:
-            active[0] = {'k': len(windows_out), 'ret': (ip + ins.size) & 0xFFFF, 'writes': []}
+            stk.append({'ret': (ip + ins.size) & 0xFFFF, 'writes': [], 'depth': len(stk)})
     return mem, jcount, written, vcount[0], checks, bad, executed


@@ -391,8 +395,9 @@
     a.effects_out.parent.mkdir(parents=True, exist_ok=True)
     eff = a.effects_out
     with open(eff, 'w') as fh:
-        for w in windows:
-            fh.write('CALL %d\n' % w['k'])
+        top_ws = [w for w in windows if w['depth'] == 0]
+        for n_, w in enumerate(top_ws):
+            fh.write('CALL %d\n' % n_)
             for addr, val in w['writes']:
                 fh.write('W %05X %02X\n' % (addr, val))
             rr = w['regs']
@@ -429,6 +434,10 @@
                "diff_without_writer": unexplained, "video_stores_counted": vcount,
                "checks": dict(checks), "bad": {k: len(v) for k, v in bad.items()}, "notes": dict(notes),
                "mutant": MUT or "none", "unexplained_examples": examples}
+    for d in DBG["ann"]: print("ANNBAD", d)
+    for addr in range(VIDEO_BASE):
+        got2 = mem.get(addr, entry[addr])
+        if got2 != exit_img[addr]: print("DIFF %05X replayed=%02X exit=%02X entry=%02X lastwriter=%s" % (addr, got2, exit_img[addr], entry[addr], DBG["lastw"].get(addr)))
     print(json.dumps(summary, indent=1, sort_keys=True))
     if a.out:
         a.out.write_text(json.dumps(summary, indent=1, sort_keys=True))
```

## Hygiene

- `tests/overlay/ovl038_0bcf_static_test.py`: 8/8 PASS against the owner GAME.OVR.
- `tests/replay/check_ovl0038_0bcf_composition.sh`: compiles with `-std=c11 -Wall -Wextra -Werror`; baseline EQUAL; the six mutants are reported above.
- `tools/overlay/check_byte_dumps.py` runs on this document; this document carries digests, addresses and code only.
- `tools/overlay/check_body_doc_pairing.py --base origin/main` and `git diff --check` are run before the commit.
