# Unit 0x1C +0x2E10: PROGRAM (ECL opcode 0x38), whole-routine translation

Status: whole-routine C translation of the routine's effects, with five far callees and two CS-pushed near callees as hooks, checked by a
focused behaviour test and a differential oracle against the original bytes. Translation only. No game run, no live-memory check, no
dispatcher integration, no parity, function-identity or credit claim.

## Claim, exactly

`pr_u1c_program_2e10` (src/recovered/overlay_u1c_program_2e10.c) performs, in the file's order: the flag-gated copy of [0x4436]/[0x4438]
into [0x5D92]/[0x5D94] and the clear of [0x4430]; the two far calls that load and select; the branch on the selector (the low byte of the
second callee's return); the calls of that branch, with the saved-PC save and restore around the first near call; and the return. The
seven callees are hooks; the translation makes no claim about what they do. Claim boundary: the routine's own effects on the DS bytes it
touches and the calls it makes, given the callee behaviour the caller supplies.

Not claimed: the callee behaviour; that the callees are the routines named below in the live game; live operand values (no game run);
the content of what PROGRAM does for its selector values; ECL dispatch integration; credit on the function backlog.

## Span and decode

- File offset 0x3B56 (unit base 0x0D46 + 0x2E10), 109 bytes, SHA-256
  `839f9df6a713088c9889d3d9969424d858f757a7ab51e1d7e05f9a8f0bab100c`.
- capstone 5.0.7, 16-bit mode: 40 instructions tile the 109 bytes exactly. The last instruction is a plain RETF at +2E7C.
- Decoded routine: `push bp; mov bp,sp; sub sp,4`; the flag test `cmp byte [0x4430],0` and the copy; `mov al,1; push ax; lcall 0x2B:0x2A`;
  `mov al,1; push ax; lcall 0x2B:0x25; mov [bp-1],al`; the selector compares against 0, 8 and 9; the calls of each branch; `mov sp,bp;
  pop bp; retf`.

## Existing records (none is a translation of this routine)

- origin/main 773f98be has no C for unit 0x1C +0x2E10 in `src/` or `include/`. It is not stale in any way that changes this translation:
  main already contains the 02E0 and WHO translations (PRs #1988 and #1989).
- ECL dispatch: `src/recovered/ecl_helpers.c` maps opcode 0x38 to 0x2E10 (line 888), and tests/recovered/test_ecl_helpers.c checks that row
  for membership. docs/re/ECL_CONTRACTS.md records `0x38 | PROGRAM | 1 | 0x03B56 | 0x2E10`. These are dispatch mappings, not translation claims.
- Backlog: docs/re/FUNCTION_BACKLOG_STEP0_95218080.md line 80 (`OVL_001C_2E10`, class `state`, 109 bytes, 40 instructions, retf,
  `entry=0;far=0;near=1`). The backlog's `far=0` does not match the five `lcall` instructions in the bytes; this translation follows the
  bytes (the same discrepancy as the 02E0 and WHO rows).
- Shared inventories name the same span: docs/re/RESIDENT_LOAD_ROUTE_27D9.md (call site +2E4F to resident 00BA:27D9),
  docs/re/RESIDENT_RUNTIME_CLASS.md (the 2E10 row), docs/re/STRICT_QUEUE_INLINE_DETECTOR.md (a span decoded en route). They record the
  call site and classification; none is a translation or a credit.

## Callees and their ownership in this repository

| Call | Repository record | Treatment here |
| --- | --- | --- |
| `lcall 0x2B:0x2A`, one word | `pr_rec_ecl_load_operands` (unit 0x25 entry 2; docs/re/FUNCTION_LEDGER.md row 6) | hook |
| `lcall 0x2B:0x25`, one word | `pr_rec_ecl_slot_value` (function 21, unit 0x25 +0x0136; FUNCTION_LEDGER.md row 21) | hook; the returned AL is the selector |
| `lcall 0x78:0x25`, no words | none on main | hook, unresolved |
| `lcall 0x82:0x25`, no words | none on main | hook, unresolved |
| `lcall 0xBA:0x27D9`, no words | RES_00BA_27D9; the `resident_27d9` callback in `include/poolrad/recovered/ecl_helpers.h` | hook |
| `push cs; call near 0x2DD3` | unit 0x1C +0x2DD3 (RESIDENT_RUNTIME_CLASS.md row; the near callee of the 3387 loop model) | hook; see the return convention below |
| `push cs; call near 0x0001` | unit 0x1C +0x0001 (OVL_001C_0001; backlog row, class `state`, near=2) | hook; see the return convention below |

The hooks are the same mechanism the 02E0 and WHO translations use. None of the seven is replaced by a substitute here.

Return convention of the CS-pushed near calls: each call pushes CS and then calls near, so the stack after the call is the same as
before only if the callee returns with RETF (popping the pushed CS). The translation assumes this for both near calls. The assumption is
not verified against the callee bytes in this document; the oracle's stubs implement the same assumption, so the oracle does not check it.

## ABI facts fixed from the bytes

1. **First argument word.** `mov al,1; push ax` at +2E30. AX has full-width values only on the flag path: there `mov ax,[0x4436]` (+2E1D)
   loads it, so the word is `high([0x4436]):01`. Off the flag path AX is the caller's entry AX, so the word is `entry_ah:01`. The
   `mov al,1` keeps AH.
2. **Second argument word.** `mov al,1; push ax` at +2E38, with AH the high byte of the first callee's returned AX (AX is not reloaded
   between the call and the push).
3. **Selector.** `mov [bp-1],al` after the second call: only the returned AL is used (+2E43 `mov al,[bp-1]`). The other byte of the return is
   ignored. Selector 0 runs `0x78:0x25` then `0xBA:0x27D9`; 8 runs `0x82:0x25`; 9 runs the near pair; any other value makes no further call.
4. **Saved PC.** For selector 9, `[0x49ED]` is read into the frame word (+2E65) before the first near call, and written back after it
   (+2E72). The second near call (+2E76) comes after the restore and does not restore again. The translation models both.
5. **DS bytes touched.** Read: `[0x4430]`, and on the flag path `[0x4436]` and `[0x4438]`. Written: `[0x4430]` (cleared on the flag path),
   `[0x5D92]`/`[0x5D94]` (flag path only), and `[0x49ED]` (restored after the first near call). The routine loads no segment register
   from DS; ES and SS are not touched.
6. **Frame.** Four bytes of locals (`[bp-1]`, the selector; `[bp-3]`, the saved PC). They are not returned state.
7. **BP and SP** are restored by the routine's own instructions (`mov sp,bp; pop bp; retf`).

## Translation map (unit-relative offsets)

| Offsets | Effect in the C translation |
| --- | --- |
| +2E10..+2E15 | frame reserved; no effect in the translation |
| +2E16..+2E2B | if DS[0x4430] != 0: copy [0x4436] to [0x5D92], [0x4438] to [0x5D94], clear [0x4430], and take AH from [0x4436]; else AH = entry AH |
| +2E30..+2E33 | `call_2b_2a((AH<<8)\|1)`; AH := high byte of its return |
| +2E38..+2E40 | `call_2b_25((AH<<8)\|1)`; the selector is the low byte of its return |
| +2E43..+2E61 | selector 0: `call_78_25`, `call_ba_27d9`; 8: `call_82_25`; 9: save [0x49ED], `call_near_2dd3`, restore [0x49ED], `call_near_0001`; else nothing |
| +2E79..+2E7C | frame teardown and RETF: returns OK |

## Checks

1. **Static decode audit**, tests/overlay/ovl1c_2e10_static_test.py, against the owner GAME.OVR (read at run time, not committed): 24 of 24
   PASS (span length and SHA-256; 40 instructions tile the span; one plain RETF last; the five far calls and their targets; the two
   near calls and their targets; the two `push cs`; the flag test, copy, selector compares and [0x49ED] save and restore by address).
2. **Focused behaviour check**, tests/recovered/test_overlay_u1c_program_2e10.c (ctest `poolrad_overlay_u1c_program_2e10_tests`): compiled
   with `-std=c11 -Wall -Wextra -Werror -pedantic`, PASS. It checks the guards; the flag path (the first word's AH from [0x4436], the copy,
   the clear); the non-flag path (AH from the entry); the second word's AH; the selector from the low byte for 0, 8, 9 and unlisted values;
   and the [0x49ED] restore after a callee that overwrites it.
3. **Differential oracle**, tests/overlay/ovl1c_2e10_oracle.py: the 109 owner bytes run under Unicorn 2.1.4 (16-bit real mode) with the
   unit's code at CS 0x0100. The five far calls and two CS-pushed near calls go to stubs at their targets. Each stub logs its call, applies
   the case's DS writes for that callee, returns the case's AX where the callee returns one, and pops its pushed words with RETF. The same
   cases run through the C translation via tests/overlay/ovl1c_2e10_harness.c. Per case: the call log, a status that requires SP restored
   to entry+4, BP restored, and each stub entry to see the stack the translation implies (SP = entry SP - 6 - 2 x argument words - 4), and the
   DS bytes that differ from their initial values below 0x5D96.
   - Seed 1, 400 cases: 400 of 400 equal; stub arity mismatches 0. Executed in emulation: flag-path load in 244 cases; selector 0 in 56;
     selector 8 in 54; selector 9 in 119.
   - Mutants (one edit each to a copy of the translation; each must differ from the original bytes), seed 1:
     | Mutant | cases that differ (of 400) |
     | --- | --- |
     | flag-path first word's AH from the entry AH | 244 |
     | second word's AH from the entry AH | 398 |
     | selector 8 calls 0x78:0x25 instead of 0x82:0x25 | 54 |
     | [0x49ED] not restored after the first near call | 10 |
     | flag-path copy to [0x5D92] dropped | 217 |
     | second near call dropped | 119 |
   - Every mutant was killed in seed 1.
   - Seed 2, 2000 cases: 2000 of 2000 equal; stub arity mismatches 0. Executed in emulation: flag-path load in 1195 cases; selector 0 in
     304; selector 8 in 282; selector 9 in 589. Mutants killed: flag-path AH from the entry, 1190 of 2000; second-word AH from the entry,
     1992 of 2000; selector 8 calling 0x78:0x25, 282 of 2000; [0x49ED] not restored, 66 of 2000; flag-path copy dropped, 1053 of 2000;
     second near call dropped, 589 of 2000.
   - Focused ctest (Debug build) passes.

## Limits

- **No game run.** The callee identities, the operand values in the live game, the content PROGRAM produces, and the ECL dispatch path are
  not checked.
- **Oracle callees are models.** The stubs apply the case's DS writes, return the case's AX values, and assume the near-call return
  convention above. The comparison is of the routine's effects under those callees.
- **Unresolved callees.** `0x78:0x25` and `0x82:0x25` have no repository model; they stay hooks with no behavioural claim. The near
  callees `+2DD3` and `+0001` have no translation in main; their behaviour is not claimed.
- **Return convention** of the CS-pushed near calls is assumed, not verified against the callee bytes (see above).
- **Backlog discrepancy.** `far=0` in the backlog does not match the five `lcall` and two `call` instructions in the bytes. The backlog
  owner needs to correct it.
- **Authorship.** The commits carry the project's placeholder identity; re-attribute before any public release.

## Reproduce

```
python3 tests/overlay/ovl1c_2e10_static_test.py --game-ovr <owner GAME.OVR>
python3 tests/overlay/ovl1c_2e10_oracle.py --game-ovr <owner GAME.OVR> --work-dir <scratch dir outside the repo> --cases 400 --seed 1
cmake --build <build dir> --target poolrad_overlay_u1c_program_2e10_tests && (cd <build dir> && ctest -R program_2e10)
```
