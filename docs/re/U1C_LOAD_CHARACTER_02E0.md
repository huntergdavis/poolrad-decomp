# Unit 0x1C +0x02E0: LOAD CHARACTER (ECL opcode 0x0A), whole-routine translation

Status: whole-routine C translation of the routine's effects, checked against the original bytes with a differential oracle.
No game run, no live-memory check, no dispatcher integration, no function-identity or credit claim.

## What is claimed, exactly

`pr_u1c_load_character_02e0` (src/recovered/overlay_u1c_load_character_02e0.c) performs, in the file's order, the DS byte and
word writes, the far-pointer chain reads through ES, and the four far calls of the routine at file offset 0x1026..0x10B3, with
the argument words the file computes, and returns `PR_U1C_LC02E0_OK` when it reaches the frame teardown. The four callees are
hooks supplied by the caller; the translation makes no claim about what they do. Claim boundary: the routine's own effects,
given the callee behaviour the caller supplies.

Not claimed: the behaviour of the four callees; that the callees are the routines the file names in the live game; any
live-versus-file difference in the operands of the four far calls (not checked here; no game run); integration with the ECL
dispatcher (the table entry for opcode 0x0A reads 0x02E0 in src/recovered/ecl_helpers.c, line 842, and that table was not
re-checked against the live game); credit on the function backlog.

## Span and decode

- File offset 0x1026 (unit base 0x0D46 + routine offset 0x02E0), 212 bytes, SHA-256
  `07903667cd47edfcd46ca97d08b10a6bb494b07008fd02290b22b642d7e42333`.
- capstone 5.0.7, 16-bit mode: 70 instructions tile the 212 bytes exactly. The last instruction is a plain RETF at +03B3.
- Four far calls, in order: +02EE to 0x2B:0x2A (one word argument), +02FA to 0x2B:0x25 (one word argument), +0394 to 0x78:0x2A
  (two word arguments), +03A1 to 0xBA:0x767 (two word arguments, the segment pushed first).
- Discrepancy to resolve in the backlog: docs/re/FUNCTION_BACKLOG_STEP0_95218080.md line 36 records this routine as
  `entry=0;far=0;near=1`. The bytes contain four far calls (`lcall`) and one RETF; the backlog's far/near split does not match
  them. This translation follows the bytes.

## Facts the translation fixes from the bytes

1. **The high byte of each argument word comes from AX at the push.** The routine does load AX from memory several times, but
   only three AX values reach a pushed word:
   - (a) first call: `mov al,1` at +02EB, `push ax` at +02ED. AX is the caller's entry AX, so AH is `entry_ah`; word = AH:01.
   - (b) second call: `mov al,1` at +02F7, `push ax` at +02F9. AX is the first callee's returned AX, so AH is that return's high
     byte; word = AH:01. The returned AL is the count.
   - (c) third and fourth calls, flag path only: `mov al,0` at +038E, `push ax` at +0390 (word AH:00); `mov al,1` at +0391,
     `push ax` at +0393 (word AH:01). AX was loaded at +0376 from word [0x4436], so AH is the high byte of [0x4436].

   Loads that set AH but reach no push: `mov ax,[0x5D96]` at +0302 (the head offset; its AH is overwritten before the flag path),
   the chain loads at +0338, and the pointer loads at +034D and +0355. Each is overwritten before any push reads AH.
2. **The AL value on entry is not read.** The first use of AL is a `mov al,1` before the first push.
3. **The third and fourth calls run only on the flag path.** The three conditional jumps `je` to +03B0 (at +0366, +036D and +0374)
   go straight to the frame teardown, skipping the calls and the two clears of [0x84D2] and [0x84D3]. The flag path is: count
   bit 0x80 set, [0x84D2] nonzero and [0x84D3] nonzero.
4. **The fourth call's words are read after the third call returns**, because the callee may change [0x5D92]/[0x5D94].
5. **The chain walk.** The loop runs while the masked count is nonzero, the index is below the count (unsigned), and the pointer is
   nonzero. Each step reads the offset word at +0x104 and the segment word at +0x106 of the current node, through ES. The output
   pair [0x5D92]/[0x5D94] is stored only when the final pointer is nonzero, so a walk that ends on null leaves the prior value.
6. **Frame and segment registers.** The routine's own instructions preserve BP and SP (`push bp; mov bp,sp`, then `mov sp,bp;
   pop bp; retf`), and it never writes DS as a segment register. ES is **not** restored: the chain walk's `les di,[bp-4]` at +0335
   loads ES with each pointer's segment, so after a walk that ran, ES holds the last pointer's segment. The C translation models
   none of BP, SP or ES (it has no registers); the oracle checks that SP is restored to entry+4 and BP to the entry value after
   the RETF, and it does not compare ES. No stack cleanup is modelled: the callee cleanup convention does not change the
   compared state because the teardown restores SP.

## Translation map (unit-relative offsets)

| Offsets | Effect in the C translation |
| --- | --- |
| +02E6 | `ds[0x4430] = 1` |
| +02EB..+02EE | `call_2b_2a((entry_ah << 8) \| 1)`; keep the high byte of the return |
| +02F3 | loop index `i = 0` |
| +02F7..+02FF | `call_2b_25((ah << 8) \| 1)`; count = return AL |
| +0302..+030C | pointer := [0x5D96] (offset), [0x5D98] (segment) |
| +030F..+031C | hi_flag = count & 0x80; count = count & 0x7F |
| +031F..+034B | walk: while count != 0, i < count, pointer != 0: pointer := ES:[pointer+0x104] / +0x106; i++ |
| +034D..+035E | if pointer != 0: [0x5D92] := offset, [0x5D94] := segment |
| +0362..+0374 | flag path only if hi_flag, [0x84D2] and [0x84D3] are nonzero (otherwise the jumps skip to +03B0) |
| +0376..+0389 | w := [0x4436]; AH := high(w); if [0x4438] = [0x5D94] and w = [0x5D92] then [0x4430] := 0 |
| +038E..+0394 | `call_78_2a(AH << 8, (AH << 8) \| 1)` |
| +0399..+03A1 | `call_ba_767([0x5D92], [0x5D94])` (read after the previous call) |
| +03A6..+03AB | [0x84D2] := 0; [0x84D3] := 0 |
| +03B0..+03B3 | frame teardown and RETF: returns OK |

## Checks

1. **Static audit**, tests/overlay/ovl1c_02e0_static_test.py, against the owner GAME.OVR (read at run time, not committed): 9 of 9
   PASS (span length and SHA-256; 70 instructions tile the span; one plain RETF last; the four far calls and their operands; the
   three `mov al,1` argument moves; the three jumps to +03B0).
2. **Focused behaviour check**, tests/recovered/test_overlay_u1c_load_character_02e0.c, built as
   `poolrad_overlay_u1c_load_character_02e0_tests` (Debug, `-Wall -Wextra -Werror -pedantic` clean): ctest passes. It covers the
   argument guards, the AH carried into the first and second calls, the null-stop walk with no store, the flag clear with the
   third-call AH and the read order of the fourth call, the flag-path skip and the segment mismatch, and a far-read failure that
   stops the routine before any store or call.
3. **Differential oracle**, tests/overlay/ovl1c_02e0_oracle.py: the 212 owner bytes run under Unicorn 2.1.4 (16-bit real mode).
   The four far calls go to stubs that log the arguments, apply the case's DS writes, and return with `retf N`. The same cases run
   through the C translation via tests/overlay/ovl1c_02e0_harness.c. Compared per case: the call log, the status (which requires SP
   restored to entry+4 and BP to its entry value after the RETF), and every DS byte that differs from its initial value. ES is
   not compared. Coverage is counted from emulated execution, not from diff lines: an executed store of 0 to [0x4430] (flag
   clear), an executed store into [0x5D92], and an executed chain-walk load at unit +0x0338. The earlier diff-line counters
   (28 and 117 flag clears) undercounted, because a clear whose final value equals the initial DS value leaves no diff line.
   Results (scratch logs outside the repository):
   - seed 1, 400 cases: 400 of 400 equal. Flag-clear store executed in 42 cases; [0x5D92] store in 220; chain-walk load in 231,
     two or more loads in 74.
   - seed 2, 2000 cases: 2000 of 2000 equal. Flag-clear store executed in 211 cases; [0x5D92] store in 1135; chain-walk load in
     1202, two or more loads in 357.
   - Mutants (one edit each to a copy of the translation; each must differ from the original bytes):
     | Mutant | seed 1 (400) | seed 2 (2000) |
     | --- | --- | --- |
     | second-arg-ah-from-first-return-zero (second word's AH forced to 0) | 398 differ | 1992 differ |
     | walk bound off by one (`i <= count`) | 96 differ | 447 differ |
     | flag compare drops the segment word | 13 differ | 61 differ |
     | fourth call's segment forced to 0 | 141 differ | 681 differ |
     | flag-path AH from the low byte of [0x4436] | 136 differ | 658 differ |
     | [0x84D3] clear dropped | 145 differ | 703 differ |
   Every mutant was killed in both runs.

## Limits

- **No game run.** The callee identities, the operand values in the live game, and the ECL dispatch path are not checked.
- **Oracle callees are models.** The stubs pop their arguments and write only DS bytes; their return values are random. The oracle
  therefore compares the routine's behaviour under these callees, not the game's. Since the routine restores SP at its end, the
  choice of callee stack cleanup does not change the compared state.
- **Far memory is generated only in safe windows** (segments 0x3000 to 0x70FF; the junk segment 0x7000 holds no words). The
  generator keeps a null head unchanged, keeps every head offset's high byte below 0xF0, and writes only those segment values, so
  no read reaches DS, the stack or code. A 16-bit offset wrap (an effective address above 0xFFFF) is therefore not generated; the C
  uses 16-bit offset arithmetic, as the effective address does, but the wrap is not oracle-checked.
- **The far-read failure** is exercised only by the focused test; the oracle does not generate failures.
- **AL on entry** is fixed to one value in the oracle. The claim that the routine does not read AL on entry rests on the bytes.
- **Owner media** are read at run time from the ignored resources directory and are not committed. Digests and addresses are the only
  owner-derived values in this document.
- **Authorship.** The commits carry the project's placeholder identity; re-attribute before any public release.

## Reproduce

```
python3 tests/overlay/ovl1c_02e0_static_test.py --game-ovr <owner GAME.OVR>
python3 tests/overlay/ovl1c_02e0_oracle.py --game-ovr <owner GAME.OVR> --work-dir <scratch dir outside the repo> --cases 400 --seed 1
cmake --build <build dir> --target poolrad_overlay_u1c_load_character_02e0_tests && (cd <build dir> && ctest -R load_character_02e0)
```
