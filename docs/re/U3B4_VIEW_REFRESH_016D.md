# Unit 0x3B4 +0x016D: view refresh (public entry 1), whole-routine translation

Status: whole-routine C translation of the routine's effects, with six callees as hooks (four far calls, one near call to the unit's own
+0x0000, and the three near case maps). Checked by a focused behaviour test, a static decode audit, and a differential oracle against
the original bytes in which the three near case maps run their real bytes. Translation only. No game run, no live-memory check, no
dispatcher or caller integration, no parity, function-identity or credit claim.

## Claim, exactly

`pr_u3b4_view_refresh_016d` (src/recovered/overlay_u3b4_view_refresh_016d.c) performs, in the file's order: the flag test (the record word
at +0x1CC, or DS:0x49FA); on flag 1, the first far call with two words, the record-selected byte copied to DS:0x6A60, the three near case
maps when DS:0x84E5 is nonzero, the four-word far call, the clear of DS:0x6AAC, and the three-word far call; on flags 2 to 4, the
coordinate computation (table byte, record words, signed clamps to 0..0x27 and 0..0x1F, CBW of each byte), the far call with those two
words, and the near call to +0x0000; and no call for any other flag. The six callees are hooks; the translation makes no claim about what
they do. Claim boundary: the routine's own effects, given the callee behaviour the caller supplies.

Not claimed: the callee behaviour; that the callees are the routines named below in the live game; the content of the view refresh on
screen; live operand values (no game run); integration with the caller at unit 0x1C +0x3382 or the movement path; credit on the function
backlog.

## Span, boundary and decode

- Unit 0x3B4 code at GAME.OVR file offset 0x2DB87 (unit memory segment 0x3CA). Routine +0x016D: file offset 0x2DCF4, 307 bytes, SHA-256
  `c6bb0b220344d76f8924b0f96bcea506bb55dbdad753e2f945fa25bf782255e8`.
- Boundary: the three bytes before the span, at +0x016A..+0x016C, are the `RETF 4` (CA 04 00) that ends the preceding nested case map
  (+0x012F, docs/re/U3B4_CASE_MAPS.md); the span ends where the prologue `55 89 E5` starts at +0x02A0.
- capstone 5.0.7, 16-bit mode: 114 instructions tile the 307 bytes exactly. The last instruction is a plain RETF at +0x029F.

## Existing records (none is a translation of this routine)

- origin/main 80a6682e has no source, header or test for +0x016D. The case maps at +0x00C7, +0x00FB and +0x012F are translated
  (src/recovered/overlay_u3b4_case_maps.c); they are callees here, not this routine. No remote branch names a file for +0x016D.
- Movement path: docs/re/GAMEPLAY_PATH.md section 3, row `+0x336A..+0x3382`, calls this entry as the view refresh.
- Candidate queue: docs/re/STRICT_CANDIDATE_QUEUE.md lists `OVL_03B4_016D` (307 bytes, 114 instructions, grade A, `game_evidenced`).
- Other lanes cite the routine in evidence notes (SLOT_A_* documents). Those are observations, not translations; no slot-A branch adds a
  source or header for it.
- The ledger has no credit row for it (docs/re/credit_evidence_kind.csv).

## Callees and their ownership in this repository

| Call | Repository record | Treatment here |
| --- | --- | --- |
| `lcall 0x3D5:0x34`, two words | unit 0x3BD entry 4, `+0x057A` (docs/re/OVL_03BD_057A_CPU.md, credited) | hook; returns the callee's full AX |
| `lcall 0x3D5:0x25`, four words | unit 0x3BD entry 1, `+0x0155` (`pr_rec_u3bd_entry1`, translated) | hook; returns the callee's full AX |
| `lcall 0x3D5:0x39`, three words | unit 0x3BD entry 5, `+0x05FD` (2059 bytes; no translation on main) | hook; return not used |
| `push cs; call near +0x00C7` (one word) | unit 0x3B4 nested case map (translated) | hook; returns AL, the translation keeps AH |
| `push cs; call near +0x00FB` (one word) | unit 0x3B4 nested case map (translated) | hook; returns AL |
| `push cs; call near +0x012F` (one word) | unit 0x3B4 nested case map (translated) | hook; returns AL |
| `lcall 0x3CD:0x25`, two words | unit 0x3B6 entry 0 (`overlay_u3b6_window_0000_cpu`, a CPU model) | hook; return not used |
| `push cs; call near +0x0000` | unit 0x3B4 entry 0 (199 bytes; no translation on main) | hook; return not used |

The hooks are the mechanism the earlier unit translations use. None of the six is replaced by a substitute here. In the oracle the three
near case maps run their real owner bytes; the other callees are stubs.

## ABI facts fixed from the bytes

1. **AX is a full 16-bit register.** Each push takes the whole AX, so an argument word is (AH of the last AX write) : (AL that the last
   MOV AL loaded). MOV AL changes only AL. The routine writes AX in full at CBW, at MOV AX, WORD, and at ADD and DEC AX; each far call
   sets AX to its full return; each near case map sets AL and keeps AH.
2. **Entry AH is read.** The first argument words (+0x0193, +0x0197) carry the entry AH. Entry AL is not read.
3. **The second far call carries the first far call's return AH** (AX is not reloaded between them except by the case maps, which keep AH,
   and by the record reads, which change only DI). The four words of the second call carry the high byte of AX as it stands after the
   case maps, which is the first callee's return AH.
4. **CBW.** The two sign extensions (`cwde` in capstone's naming of byte 0x98, which is CBW in 16-bit mode) set AX to the sign extension
   of AL. The oracle confirms this: keeping AH in place of the sign extension changes the call words on every coordinate-path case.
5. **The record far pointer is re-read at each LES** from DS:0x49D2 (offset) and DS:0x49D4 (segment). A callee that changes those bytes
   changes what the later reads see. The record words read are +0x186, +0x188, +0x1CC, +0x1F6, +0x1FA and +0x1FC.
6. **Flag.** [BP-3] := DS:0x49FA; if the record word at +0x1CC is nonzero, [BP-3] := 1. Flag 1 runs the call sequence. Flags 2, 3 and 4
   run the coordinate path. Any other flag makes no call.
7. **Coordinate path.** [BP-1] := low byte of (DS table byte at 0x3C76 + flag, plus the record word at +0x186, minus 2), clamped as a signed
   byte to 0..0x27. [BP-2] := low byte of (record word at +0x188, minus 2), clamped to 0..0x1F. The clamps are signed comparisons.
8. **DS bytes touched.** Read: DS:0x49FA, DS:0x49D2..0x49D5, DS:0x84E5, DS:0x6AAD..0x6AAF, DS:0x6A60..0x6A63 (in the no-case-map path),
   DS:0x6F68, DS:0x6F69, DS:0x3C76..0x3C7A, and the byte at (record word + 0xC26). Written: DS:0x6AB1, DS:0x6A60, DS:0x6A61..0x6A63 (case maps),
   DS:0x6AAC (only if the record word at +0x1F6 is nonzero). The routine does not write any segment register or DS data it does not name.
9. **Frame.** [BP-1], [BP-2] and [BP-3] are locals. The routine's last instructions (mov sp,bp; pop bp; retf) restore BP and SP.
10. **Exit registers are not modelled.** AX, DX and flags after RETF depend on the path (the last callee's return, or the stale AX of the
    coordinate path). The translation returns only the status.

## Translation map (unit-relative offsets)

| Offsets | Effect in the C translation |
| --- | --- |
| +0x016D..+0x0176 | frame reserved; flag := DS:0x49FA; the AX register starts from the entry AH |
| +0x0179..+0x0189 | record word +0x1CC; flag := 1 if nonzero |
| +0x0193..+0x019B | two words (AH:[6AAD], AH:[6AAE]); `call_3d5_34`; AX := its return |
| +0x01A0..+0x01C9 | [6AB1] := AL; record word +0x1FA (if [6AB1] <= 0x7F) or +0x1FC; AL := DS[that + 0xC26]; [6A60] := AL |
| +0x01CC..+0x01F4 | if [0x84E5] != 0: `near_00c7`, `near_00fb`, `near_012f` on the bytes [0x6F69], [0x6F69], [0x6F68]; results to [6A61..6A63] |
| +0x01F7..+0x0207 | four words (AH:[6A60..6A63]); `call_3d5_25`; AX := its return |
| +0x020C..+0x0218 | record word +0x1F6; if nonzero, [0x6AAC] := 0 |
| +0x021D..+0x0229 | three words (AH:[6AAD..6AAF]); `call_3d5_39`; return not used |
| +0x0230..+0x0236 | flag must be 2, 3 or 4; otherwise return |
| +0x0238..+0x0250 | [BP-1]: table byte at 0x3C76 + flag, plus record +0x186, minus 2, signed clamp 0..0x27 |
| +0x0267..+0x0285 | [BP-2]: record +0x188, minus 2, signed clamp 0..0x1F |
| +0x0289..+0x0293 | two CBW words; `call_3cd_25` |
| +0x0298 | `near_0000` |
| +0x029C..+0x029F | frame teardown and RETF: returns OK |

## Checks

1. **Static decode audit**, tests/overlay/ovl3b4_016d_static_test.py, against the owner GAME.OVR (read at run time, not committed): 38 of
   38 PASS (span length and SHA-256; the next prologue; 114 instructions tile the span; one plain RETF last; the four far calls and
   their targets; the four near calls and their targets; the four `push cs`; the flag, record, table and clamp instructions by address; the
   two CBW instructions).
2. **Focused behaviour check**, tests/recovered/test_overlay_u3b4_view_refresh_016d.c (ctest `poolrad_overlay_u3b4_view_refresh_016d_tests`):
   compiled with `-std=c11 -Wall -Wextra -Werror -pedantic`, PASS. It checks the guards; the flag-1 low branch and the first and second
   words (entry AH, first return AH); the high branch and the [0x6AAC] clear; the case maps when [0x84E5] is set (the argument words,
   the AL stored, the second call's words); the coordinate clamps, including a negative value that clamps to 0; the no-call flags 0 and 5;
   and a far read failure that stops the routine before the third call.
3. **Differential oracle**, tests/overlay/ovl3b4_016d_oracle.py: the unit's 0x2A5 code bytes are loaded under Unicorn 2.1.4 (16-bit real
   mode), so the three near case maps run their real owner bytes and the case-map results are checked against the table in the oracle's
   harness. The routine runs from +0x016D. The far calls and the near call to +0x0000 go to stubs that log their pushed words, apply the
   case's DS writes, return the case's AX where the callee returns one, and pop their pushed words with RETF. Per case: the call log
   (with the case-map argument words read from the real callee's stack), a status that requires SP restored to entry+4, BP restored, and
   every stub entry to see the stack the routine implies, and the DS bytes that differ from their initial values.
   - Seed 1, 400 cases: 400 of 400 equal; stub arity mismatches 0. Executed in emulation: flag-1 path in 293 cases; coordinate path in 60;
     the [0x84E5] branch in 293.
   - Seed 2, 2000 cases: 2000 of 2000 equal; stub arity mismatches 0. Flag-1 path in 1378 cases; coordinate path in 310; [0x84E5] branch
     in 1378.
   - Mutants (one edit each to a copy of the translation; each must differ from the original bytes):
     | Mutant | seed 1 (400) | seed 2 (2000) |
     | --- | --- | --- |
     | entry AH ignored | 292 differ | 1371 differ |
     | first callee's return AH dropped before the four-word call | 293 differ | 1373 differ |
     | third word of the last call with AH zeroed | 292 differ | 1374 differ |
     | CBW keeps AH instead of sign-extending | 60 differ | 310 differ |
     | upper clamp on the second byte dropped | 21 differ | 103 differ |
     | coordinate path skipped | 60 differ | 310 differ |
   - Every mutant was killed in both runs.

## Limits

- **No game run.** The callee identities, the operand values in the live game, the content of the view refresh on screen, and the caller's
  use of the routine are not checked.
- **Oracle callees are models**, except the three near case maps, which run their real bytes. The far callees' effects are the case's
  DS writes and AX values.
- **Callees without translations on main**: unit 0x3BD entry 5 (`+0x05FD`, 2059 bytes) and unit 0x3B4 entry 0 (199 bytes). They are hooks
  with no behavioural claim.
- **Exit registers** AX, DX and flags are not modelled (see ABI fact 10).
- **16-bit wrap** of record offsets and DS offsets is not oracle-checked: the oracle keeps the record and table offsets below 0xF000 so no
  wrap is generated. The translation uses 16-bit arithmetic, as the effective address does.
- **Far memory** is the record window only (segments 0x3000 to 0x70FF); the oracle places the record there.
- **Dispatch and caller** integration is not done. The movement path's caller (unit 0x1C +0x3382) is not changed.
- **Authorship.** The commits carry the project's placeholder identity; re-attribute before any public release.

## Reproduce

```
python3 tests/overlay/ovl3b4_016d_static_test.py --game-ovr <owner GAME.OVR>
python3 tests/overlay/ovl3b4_016d_oracle.py --game-ovr <owner GAME.OVR> --work-dir <scratch dir outside the repo> --cases 400 --seed 1
cmake --build <build dir> --target poolrad_overlay_u3b4_view_refresh_016d_tests && (cd <build dir> && ctest -R u3b4_view_refresh_016d)
```
