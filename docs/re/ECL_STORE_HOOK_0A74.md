# Class-1 ECL store hook, unit 0x25 + 0x0A74 (issue #90)

`src/recovered/overlay_ecl_store_hook.c` translates the DOS v1.3 `GAME.OVR`
routine at unit `0x25 + 0x0A74`: the hook that the ECL store-word core
(`+0x0C77`, public entry 10) runs after every word store into the `0x6B00`
window, and that the merged model reports as `HOOK` without executing
([FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) store-word row;
[ECL_NEW_GAME_ROUTE.md](ECL_NEW_GAME_ROUTE.md) §4). Status: **path-complete
local-body model with two callback boundaries**, in the vocabulary of
[FUNCTION_LEDGER.md](FUNCTION_LEDGER.md) §1 (a full translation reproduces
every instruction path with the same inputs and outputs; this routine's
own body is reproduced case by case, but the behavior behind its four
call-out sites, two distinct untranslated targets, is left to caller-supplied
callbacks). It therefore does not join the strict full-C count; the PM
handles classification after merge. It is standalone: nothing in `ecl_helpers.c/h` is
changed; the PM connects it to the store-word core after review.

`deja "unit 0x25 +0x0A74 ECL store hook class 1"` returned the SAVE-route
investigation (PR #88 lineage) and this worker's unit 0x38 sessions; the
route note's one-line summary of the hook was a lead and is re-derived
below from the bytes, where it holds (its address list is confirmed and
completed).

## Provenance (ignored local input; hashes only)

`GAME.OVR`, 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`. Unit
`0x25` at file offset 29,907 (code size 8,730; stub segment `0x2B`).

| Routine | Unit offset | File offset | Bytes | Insns | Return | SHA-256 of the routine's bytes |
| --- | --- | --- | ---: | ---: | --- | --- |
| store hook | +0x0A74 | 0x07F47 (32,583) | 515 | 163 | `retf 4` | `258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938` |

The ledger bound (515) equals the decoded span; no trailing bytes. All 45
branch targets lie inside +0x0A74..+0x0C76 (lowest +0x0A96, highest +0x0C71,
the common exit). Branch census: 28 conditional (21 `jne`, 4 `jbe`, 2 `jb`,
1 `ja`) and 17 unconditional `jmp`, every `jmp` to the common exit. Four
`lcall` sites to two distinct targets: +0x0C03 → `0x802:0x094A`; +0x0C26,
+0x0C49 and +0x0C6C → `0x3D5:0x003E`. No near calls, no indirect branches.
(An earlier draft said three call-outs; corrected after PM parity review.)

## Identity and caller

- Public classification: grade B (no public entry), `far_in` 0, one near
  caller (ledger `near_called` 1). The only `call +0x0A74` in unit 0x25 is
  at `+0x0CD5` inside the store-word routine `+0x0C77` (checked by decoding
  every ledger routine of the unit).
- Call site, class-1 branch of store word (`+0x0CB7..+0x0CD8`): `cmp al,1;
  jne; mov dx,[bp+6]; mov ax,[bp+8]; shl ax,1; les di,[0x49D6]; add di,ax;
  mov es:[di+0x2A00],dx; push [bp+8]; push [bp+6]; push cs; call +0x0A74;
  jmp exit`. So the **window word is written first** (index `address -
  0x6B00`: `2·address + 0x2A00` wraps to `2·(address − 0x6B00)` in 16 bits,
  confirming the merged model's window-1 indexing), **then the hook runs**
  with the same two words. `push cs` before a near call builds a far frame,
  matching the hook's `retf 4`.
- Hook frame: `[bp+8]` = address (first pushed), `[bp+6]` = value. The hook
  overwrites `[bp+8]` with `address − 0x6B00` and, in two cases, `[bp+6]`
  with an adjusted value; both are its own argument slots, discarded by
  `retf 4`, so the caller sees no change.

## Body: cases in compare order

`rel = address − 0x6B00` (16-bit). Record = the 285-byte active character
record at far pointer `[0x5D92]`, dereferenced without a null check in every
record case (the script-state initializer nulls `[0x5D92]`, FUNCTION_LEDGER
§ initializer; the party menu sets it).

| Case (offset) | Condition on `rel` | Effect |
| --- | --- | --- |
| +0x0A86 | `== 0` | if value `== 0`: byte `[0x84D3] = 1` |
| +0x0A99 | `0x20 ≤ rel ≤ 0x70` (unsigned) | `record[0x17 + (uint8)(rel − 0x1F)] = low byte of value`, i.e. `record[rel − 8]`, offsets 0x18..0x68 |
| +0x0AC1 | `== 0xB8` | if value `> 0xB2` (unsigned word): `value −= 0x32`; `record[0x84] = low byte` |
| +0x0AE0 | `== 0xBB` | word `record[0x88..0x89] = value` |
| +0x0AF4 | `== 0xBD` | word at 0x8A |
| +0x0B08 | `== 0xBF` | word at 0x8C |
| +0x0B1C | `== 0xC1` | word at 0x8E |
| +0x0B30 | `== 0xC3` | word at 0x90 |
| +0x0B44 | `== 0xF7` | word at 0xB8 |
| +0x0B58 | `== 0xF9` | `record[0xBA] = low byte` |
| +0x0B6C | `== 0x100` | if value `≥ 0x80`: `record[0x10D] = 0`, and if value `== 0x87`: `record[0x10C] = 7`; then (independently) if value `== 0`: byte `[0x84D2] = 1` |
| +0x0BA1 | `== 0x10C` | value 0: `record[0x10E] = 0, [0x10F] = 0`; 0x80: `(0, 1)`; 0x81: `(1, 1)`; else nothing |
| +0x0BFA | `== 0x312` | `mov al,[bp+6]; push ax; lcall 0x802:0x094A`: AX still holds rel, so the pushed word is `0x0300 \| (value & 0xFF)` |
| +0x0C0A | `== 0x322` | if value `> 0x80`: `value &= 0x7F`; `lcall 0x3D5:0x3E` (unit 0x3bd entry 6, `+0x0E08`) with (value low byte, 1) |
| +0x0C2D | `== 0x324` | same with selector 2 |
| +0x0C50 | `== 0x326` | same with selector 3 |
| (fall through) | anything else | nothing |

Every case ends at the common exit `+0x0C71` (`mov sp,bp; pop bp; retf 4`).
The conditions are disjoint, so the compare order is not observable. All
record offsets written (0x18..0x68, 0x84, 0x88..0x91, 0xB8..0xBA, 0x10C..
0x10F) lie inside the verified 285-byte frame; `0x104..0x10B` are documented
far-pointer slots and `0x10C..0x10F` follow them. Word stores are
little-endian `mov word ptr es:[di+k], ax`.

In window-address terms the cases are `0x6B00`, `0x6B20..0x6B70`, `0x6BB8`,
`0x6BBB/BD/BF/C1/C3`, `0x6BF7`, `0x6BF9`, `0x6C00`, `0x6C0C`, `0x6E12`,
`0x6E22/24/26`: the list the route note gave, now with the exact conditions.

## Dependencies, named

| Call-out | Identity | Public evidence | Modelled as |
| --- | --- | --- | --- |
| `0x802:0x094A` | resident `RES_0802_094A`, exact Pascal prologue, two overlay callers (unit 0x25 here, unit 0x6f), unresolved role | `resident_callable_ledger.csv` | callback `res_802_094a(context, pushed word)`; the word is exactly `0x0300 \| (value & 0xFF)` |
| `0x3D5:0x3E` | overlay unit 0x3bd entry 6, `+0x0E08`, 703 bytes, `retf 4`, eight callers (units 0x1c, 0x25, 0x6f), `game_evidenced` | `overlay_function_ledger.csv` row 630 | callback `u3bd_entry6(context, value byte, selector byte)`; both pushed words have AH = 0 (`and ax, 0x7F`, then `mov al, n`), so bytes are exact |

Each call-out is the last act of its case, so a missing callback leaves
nothing else pending; the translation reports `PR_U25_HOOK_CALLOUT` with
the case, value and selector in the report. Pushed words are reproduced
exactly: the 0x312 case pushes AX after loading only AL, with AH still the
high byte of rel (`0x03`); the 0x322/4/6 cases push words whose high byte
is zero. (An earlier draft passed the 0x312 argument as a byte, losing AH;
corrected after PM parity review.)

## C contract

`pr_u25_store_hook_0a74(address, value, record, flags, callouts, report)`.
`record` is the writable 285-byte active record (NULL is accepted and
rejected with `PR_U25_HOOK_ARGUMENT` only when a case would write it, since
the engine would write through the null far pointer). `flags` holds the two
engine bytes `[0x84D2]`/`[0x84D3]` (the merged loader state carries fields
of the same names, for the PM's integration). `report` names the case, the
first record offset written, and the call-out made.

## Tests (`tests/recovered/test_overlay_ecl_store_hook.c`)

Hand-derived expectations; every check also asserts that no other record
byte or flag moved. Address sweep over all `rel` 0..0x3FF with a
non-special value (each case's exact offsets, callback captures, and no
effect elsewhere) plus the wrapped address 0x6AFF; `[0x84D3]` only on value
0; rel 0x100 over 0..0x1FF and sampled to 0xFFFF (0x10D clear from 0x80,
0x10C = 7 only at 0x87, `[0x84D2]` only at 0); rel 0xB8 over 0..0xFF and
sampled, with the six boundary values 0xB2/0xB3/0xFF/0x100/0x132/0xFFFF;
rel 0x10C's three recognised values against all others; byte-range ends
(0x18 and 0x68, high byte ignored, neighbours intact); little-endian word
store; call-outs (0x312 pushes `0x0300 | low byte`, e.g. 0x42 → 0x0342 and
0xFF42 → 0x0342; `> 0x80` gate, `& 0x7F`, selector 1..3 for 0x322/4/6;
0x81→1, 0x100→0, 0xFF→0x7F); missing callbacks; null contracts by case.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc) builds the
library and test with no diagnostics and prints `ECL store hook +0x0A74
tests passed`; CMake build has no warnings; `ctest` passes 25 of 25 targets
on main 5baf6ed (twenty-four existing plus
`poolrad_overlay_ecl_store_hook_tests`).

## Limitations, stated plainly

- Classification: path-complete local-body model with two callback
  boundaries (resident `0x802:0x094A` and unit 0x3bd entry 6), not a full
  translation; the strict full-C count is unchanged by this note.

- The two call-outs are not translated; their roles are unknown. The hook's
  own behavior is complete without them except for what they do.
- Field meanings (record bytes 0x18..0x68, 0x84, 0x88..0x91, 0xB8..0xBA,
  0x10C..0x10F; flags `[0x84D2]`, `[0x84D3]`; the constants 0xB2/0x32, 0x87,
  0x80/0x81) are not asserted. The 0xB8 case subtracting 0x32 above 0xB2 is
  stated as arithmetic only.
- The hook runs after the window write; connecting it to
  `pr_rec_ecl_store_word` (replacing the `HOOK` status with the hook's
  result and its call-out reports) is the PM's integration step and is not
  done here. This note does not make the new-game route executable.
