# Unit 0x1C +0x2B33: WHO (ECL opcode 0x39), whole-routine translation

Status: whole-routine C translation of the routine's calls, with four far callees as hooks, checked by a focused behaviour test and a
differential oracle against the original bytes. Translation only. No game run, no live-memory check, no dispatcher integration, no
parity, function-identity or credit claim.

## Claim, exactly

`pr_u1c_who_2b33` (src/recovered/overlay_u1c_who_2b33.c) performs, in the file's order, the four far calls of the routine, with the
argument words the file builds (including the high bytes carried in AX), and returns `PR_U1C_WHO_OK` when it reaches the frame teardown.
The routine writes no memory of its own except the pushes of its calls; the 256-byte frame it reserves is written only by the StrStore
callee. The four callees are hooks; the translation makes no claim about what they do. Claim boundary: the routine's calls, given the
callee behaviour the caller supplies.

Not claimed: the callee behaviour; that the callees are the routines the file names in the live game; live-versus-file operand values
(no game run); the content of what WHO displays; ECL dispatch integration; credit on the function backlog.

## Span and decode

- File offset 0x3879 (unit base 0x0D46 + 0x2B33), 75 bytes, SHA-256
  `b373a049e2df103b77382331370aefb2a5378b0f9d66b947333c16298e646083`.
- capstone 5.0.7, 16-bit mode: 36 instructions tile the 75 bytes exactly. The last instruction is a plain RETF at +2B7D.
- Boundary: the byte before the span, at +2B32 (file 0x3878), is 0xCB, the RETF that ends the CLEAR BOX routine at [+2B14, +2B33)
  (docs/re/U1C_CLEAR_BOX_2B14.md). The span starts with `push bp` at +2B33.
- Decoded routine: `push bp; mov bp,sp; sub sp,0x100`; then four argument-word groups, each a `mov al,imm; push ax` pair, and four
  far calls: `lcall 0x2B:0x2A` (+2B3D), `lcall 0x709:0x0DD3` (+2B4E), `lcall 0xAF8:0x0B25` (+2B62) and `lcall 0xBA:0x2C48` (+2B75);
  then `mov sp,bp; pop bp; retf`.

## Existing records (none is a translation of this routine)

- No C for unit 0x1C +0x2B33 exists in `src/` or `include/` on origin/main b06c7c50.
- ECL dispatch: `src/recovered/ecl_helpers.c` maps opcode 0x39 to 0x2B33 (line 889); docs/re/ECL_CONTRACTS.md row `0x39 | WHO | 1 |
  0x03879 | 0x2B33`. This is a dispatch mapping, not a claim of translation.
- Backlog: docs/re/FUNCTION_BACKLOG_STEP0_95218080.md line 76 (`OVL_001C_2B33`, class `glue`, 75 bytes, 36 instructions, retf,
  `entry=0;far=0;near=1`). The backlog's `far=0` does not match the four `lcall` instructions in the bytes; this translation follows the
  bytes (the same discrepancy as the 02E0 row).
- docs/re/strict_queue_discards.json has a row `OVL_001C_2B33`, rule `BOUNDARY_ASSERTION`, `covered_by OVL_001C_2B14`, from the CLEAR
  BOX test's assertion that its last instruction ends at 0x2B33. That discards a boundary mention in a test; it is not a claim that
  this routine is translated or credited.

## Callees and their ownership in this repository

| Far call | Repository record | Treatment here |
| --- | --- | --- |
| `0x2B:0x2A` | ECL load-operands entry: `pr_rec_ecl_load_operands`, unit 0x25 +0x0000 (docs/re/FUNCTION_LEDGER.md row 6) | hook |
| `0x709:0x0DD3` | resident display routine; also the sole call of CLEAR BOX (`resident_0709_clear_0dd3_cpu.c`, a raw-CPU model) | hook |
| `0xAF8:0x0B25` | StrStore: `system_strstore_fullstate_0b25.c`, `0AF8:[0B25,0B49)` (docs/re/SYSTEM_STRSTORE_0B25.md) | hook |
| `0xBA:0x2C48` | resident; listed as unresolved in docs/re/RESIDENT_FUNCTION_INVENTORY.md (`0x2C48..0x2F57`) | hook, unresolved |

The hooks are the same mechanism the 02E0 translation uses. None of the four callees is replaced by a substitute here.

## ABI facts fixed from the bytes

Argument words are `AL=constant` with AH from the register AX at the push. The routine never reads memory into AX; its only assignment
to AX is the immediate `mov ax,0xFF` (+2B5E). The AH values reaching pushed words are:

1. **First call** (+2B3A, push at +2B3C): AX is the caller's entry AX, so the word is `entry_ah:01`.
2. **Display call** (+2B42..+2B4B): four words, `AH:01`, `AH:11`, `AH:26`, `AH:16`, where AH is the high byte of the first callee's
   returned AX (AX is not reloaded between the call and these pushes). Pushed in that order: the first word (`:01`) is deepest.
3. **StrStore call** (+2B53..+2B62): pushes DS, `0x70C8`, SS, the frame address and `0x00FF`. AX is set to `0x00FF` at +2B5E, so the
   last word's AH is 0.
4. **Final call** (+2B6D..+2B75): `mov al,0; push ax` pushes `AH:00`, where AH is the high byte of the StrStore's returned AX (AX is
   not reloaded after the StrStore call). Then DS and `0x5D92` are pushed as a far pointer, so the callee sees (word, DS, 0x5D92).

Frame: BP = entry SP - 2 (push bp; mov bp,sp), and `sub sp,0x100` reserves [BP-0x100, BP). The StrStore destination is the frame
address BP-0x100 = entry SP - 0x102. The translation rejects an entry SP below 0x0102, where that address would wrap.

Registers: BP and SP are restored by the routine's own instructions (`mov sp,bp; pop bp; retf`). DS and SS are not written. ES is not
touched. The translation models none of BP, SP, DS or SS beyond the values it pushes.

## Translation map (unit-relative offsets)

| Offsets | Effect in the C translation |
| --- | --- |
| +2B33..+2B36 | frame reserved: rejects entry SP < 0x0102; StrStore destination = entry SP - 0x102 |
| +2B3A..+2B3D | `call_2b_2a((entry_ah << 8) \| 1)`; keep the high byte of its return |
| +2B42..+2B4E | `call_709_0dd3` with four words `(AH<<8)\|0x01, \|0x11, \|0x26, \|0x16` |
| +2B53..+2B62 | `call_af8_0b25(DS, 0x70C8, SS, frame address, 0x00FF)`; keep the high byte of its return |
| +2B6D..+2B75 | `call_ba_2c48((AH<<8)\|0x00, DS, 0x5D92)` |
| +2B7A..+2B7D | frame teardown and RETF: returns OK |

## Checks

1. **Static decode audit**, tests/overlay/ovl1c_2b33_static_test.py, against the owner GAME.OVR (read at run time, not committed): 15 of
   15 PASS (span length and SHA-256; 36 instructions tile the span; one plain RETF last; the four far calls and their operands; the
   seven argument-word constants; the frame reservation and the two offset loads).
2. **Focused behaviour check**, tests/recovered/test_overlay_u1c_who_2b33.c (ctest `poolrad_overlay_u1c_who_2b33_tests`): compiled with
   `-std=c11 -Wall -Wextra -Werror -pedantic`, PASS. It checks the guards (null env and hook; entry SP 0x0101 rejected before any call;
   entry SP 0x0102 gives frame offset 0), the call order A, D, C, E, the exact argument words for entry AH 0x12 and a second case with
   high bytes 0xFF and 0x00, and the StrStore frame address.
3. **Differential oracle**, tests/overlay/ovl1c_2b33_oracle.py: the 75 owner bytes run under Unicorn 2.1.4 (16-bit real mode). The four
   far calls go to stubs that log the pushed words in pushed order, return the case's AX values, and return with `retf N` for the
   pushed bytes. The same cases run through the C translation via tests/overlay/ovl1c_2b33_harness.c. Per case: the call log, and a
   status that requires SP restored to entry+4, BP restored to its entry value, and no memory write of the body inside the frame
   [BP-0x100, BP). Results (scratch outputs outside the repository):
   - seed 1, 400 cases: 400 of 400 equal. The body made 9,600 memory writes (its pushes, counted by the write hook); none landed inside
     the frame.
   - Mutants (one edit each to a copy of the translation; each must differ from the original bytes):
     | Mutant | seed 1 (400) |
     | --- | --- |
     | display words' AH from the entry AH | 400 differ |
     | StrStore-return AH taken from the first return | 399 differ |
     | frame address without the saved-BP offset (entry SP - 0x100) | 400 differ |
     | StrStore maximum 0x0100 | 400 differ |
     | first word's AH zeroed | 399 differ |
   - Every mutant was killed.
   - seed 2, 2000 cases: 2000 of 2000 equal. The body made 48,000 memory writes; none landed inside the frame. Mutants killed: display
     words' AH from the entry AH, 1999 of 2000; StrStore-return AH from the first return, 1990 of 2000; frame address without the
     saved-BP offset, 2000 of 2000; StrStore maximum 0x0100, 2000 of 2000; first word's AH zeroed, 1990 of 2000.
   - Focused ctest and the 0x2B33 static audit were run on the committed head (see the report).

## Limits

- **No game run.** The callee identities, the operand values in the live game, the content displayed, and the ECL dispatch path are not
  checked.
- **Oracle callees are models.** The stubs pop their pushed words and return the case's AX values; they do not write the frame. The
  comparison is of the routine's calls and frame discipline under those callees, not of the game's callees.
- **The frame's content is not compared.** The routine does not write it, and the StrStore hook does not model its writes.
- **0xBA:0x2C48 is unresolved** in this repository, so it stays a hook with no behavioural claim.
- **Entry SP wrap.** The oracle draws entry SP from [0x0102, 0xFFF0]. Pushes below a small frame address can wrap the 16-bit stack
  pointer; both models wrap identically in the cases run, but this is not separately established.
- **Callee cleanup** is not modelled in the translation (the oracle's stubs pop their arguments as the RETF counts say). The routine
  restores SP, so the compared state does not depend on it.
- **Strict-queue discard row.** `OVL_001C_2B33` is a boundary-mention discard (see Existing records). It is not a claim of translation.
- **Backlog discrepancy.** `far=0` in the backlog does not match the four `lcall` instructions in the bytes. The backlog owner needs to
  correct it.
- **Authorship.** The commits carry the project's placeholder identity; re-attribute before any public release.

## Reproduce

```
python3 tests/overlay/ovl1c_2b33_static_test.py --game-ovr <owner GAME.OVR>
python3 tests/overlay/ovl1c_2b33_oracle.py --game-ovr <owner GAME.OVR> --work-dir <scratch dir outside the repo> --cases 400 --seed 1
cmake --build <build dir> --target poolrad_overlay_u1c_who_2b33_tests && (cd <build dir> && ctest -R who_2b33)
```
