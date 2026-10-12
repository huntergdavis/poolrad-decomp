# Unit 0x7f entry 10 (+0x0130, stub 0x8F:0x0052): full translation (five-word clear, div 5 split)

`src/recovered/overlay_u7f_div5.c` translates DOS v1.3 `GAME.OVR` unit
`0x7f` public entry 10, the 85-byte leaf three fixup-confirmed callers
invoke with one word to zero five words at offsets `+0x88..+0x90` of the
record behind the active-character far pointer `[0x5D92]` and then store
the word's unsigned quotient by 5 at `+0x90` and its remainder at `+0x8E`.
Classification: **full translation** (FUNCTION_LEDGER.md §1 vocabulary):
every instruction path is reproduced with the same input and the same ten
output bytes; the routine makes no calls, reads nothing from the record,
and its only data-segment access is the far pointer load. `ecl_helpers.c/h`,
the unit 0x3bd files and the resident files are untouched.

`deja "Poolrad unit 0x7f +0x0130 active character helper"` (and two
broader queries) found no prior session. Reused with citation and
re-checked here: `[0x5D92]/[0x5D94]` as the active-character far pointer
and the verified 285-byte record frame from
[GAMEPLAY_PATH.md](GAMEPLAY_PATH.md),
[ECL_STORE_HOOK_0A74.md](ECL_STORE_HOOK_0A74.md) and
`docs/formats/DOS_CHARACTER.md`; the ledger row (`0x7f:0x0130`, grade A,
bound 85, 31 instructions, class `state`, `retf 2`) from
`overlay_function_ledger.csv`; the classifier's `game_evidenced /
active_char` row from `overlay_classification.csv`.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x7f`: descriptor at file 2,044, code at file offset 150,665
(`0x24C89`), code size 8,795, fifteen public entries, run-time segment
`0x8F` (from the next descriptor, sequential check ok); entry 10 maps to
`+0x0130` (stub `0x8F:0x52`, since `(0x52 - 0x20) / 5 = 10`), file offset
`0x24DB9` (150,969). Decoded with objdump (`-m i8086`) and Capstone 5.0.7
independently; both give the same 31 instructions and agree with the
PM's read-only scan.

| Item | Value |
| --- | --- |
| extent | `+0x0130..+0x0184`, **85 bytes, 31 instructions**, one `retf 2` at `+0x0182`; exact: entry 11 begins at `+0x0185` and the ledger bound of 85 is fully decoded |
| span SHA-256 | `5dbf59644f6cb84c5c918802d9b964d43843033b3870607eeae47cf07fbc9db6` |
| frame | `sub sp, 2`; `[bp+6]` = the one pushed word (`retf 2`); `[bp-1]` = loop counter byte (0..4); `[bp-2]` is never touched |
| branches | 2: `jmp +0x013F` (forward, skips the first `inc`) and `jne +0x013C` (backward, the loop); no indirect branches |
| call sites | none (near or far) |
| data-segment reads | the far pointer `[0x5D92]` (`les di`) at three sites, `+0x0145` (inside the loop), `+0x0162` and `+0x0176`, executed seven times in all (five loop passes plus the two later stores); nothing else |
| ES accesses | three `mov word ptr es:[di + k], ax` sites: `+0x014D` (`k = 0x88`, di pre-advanced by `2*i`, five passes), `+0x0166` (`k = 0x90`), `+0x017A` (`k = 0x8E`); no ES reads |
| registers at return | AX = remainder, DX = quotient (after `xchg`), CX = 5, DI = pointer offset; no caller reads any of them (each jumps away immediately) |

## Instruction paths (local effects in order; `v` = `[bp+6]`, `R` = the record at `[0x5D92]`)

| Offsets | Effect | Model |
| --- | --- | --- |
| `+0x0130..+0x0133` | prologue, `sub sp, 2` | |
| `+0x0136..+0x013A` | `[bp-1] = 0; jmp +0x013F` | `i = 0` |
| `+0x013F..+0x014D` | `al = [bp-1]; cbw; shl ax, 1; les di, [0x5D92]; di += ax; ax = 0; es:[di+0x88] = ax` | `R[0x88 + 2i] = 0` (word); `cbw` never sign-extends because `i ≤ 4` |
| `+0x0152..+0x0156` | `cmp byte [bp-1], 4; jne +0x013C` (`inc [bp-1]`, back to the body) | do-while: `i = 0, 1, 2, 3, 4`, five stores at `+0x88, +0x8A, +0x8C, +0x8E, +0x90` |
| `+0x0158..+0x0160` | `ax = v; xor dx, dx; cx = 5; div cx` | unsigned `DX:AX = 0:v` ÷ 5: `AX = v div 5` (≤ 13107, fits), `DX = v mod 5`; the divisor is a constant 5, so `#DE` is impossible |
| `+0x0162..+0x0166` | `les di, [0x5D92]; es:[di+0x90] = ax` | `R[0x90] = v div 5` (word) |
| `+0x016B..+0x0173` | `ax = v; xor dx, dx; cx = 5; div cx` | the identical division again |
| `+0x0175..+0x017A` | `xchg dx, ax; les di, [0x5D92]; es:[di+0x8E] = ax` | `R[0x8E] = v mod 5` (word) |
| `+0x017F..+0x0182` | `mov sp, bp; pop bp; retf 2` | done |

Net state: bytes `R[0x88..0x8D] = 0`; `R[0x8E..0x8F] = v mod 5`
(little-endian); `R[0x90..0x91] = v div 5` (little-endian); the loop's
zeros at `+0x8E` and `+0x90` are overwritten, so the order "clear, then
store" is unobservable in the final state but is reproduced anyway. No
other byte of the record, and no DS global, is written. The division is
unsigned (`xor dx, dx; div`, not `cwd; idiv`): 65535 gives 13107 r 0, not
−1.

The ten record bytes `0x88..0x91` lie inside the verified 285-byte frame.
The same five word slots are written (from window addresses
`0x6BBB/BD/BF/C1/C3`) by the class-1 ECL store hook
(ECL_STORE_HOOK_0A74.md), which corroborates that they are word-sized
slots; `docs/formats/DOS_CHARACTER.md` names nothing at those offsets and
this routine does not establish what they hold.

**Null pointer.** The routine reads `[0x5D92]` at three `les di` sites
(seven executions: five in the loop, two for the later stores) and writes
through it with no nil check; a nil pointer would store to segment 0 at
offsets `0x88..0x91`. The translation returns `ARGUMENT` on entry, before
any write, for a null host record or a host buffer shorter than 0x92
bytes. The size bound is a host-side guard (the engine has no length
notion here), stated so a short synthetic buffer cannot be overrun.

## Callers (three fixup-confirmed far calls; stub `0x8F:0x52`)

All three sites were decoded from their routine's prologue. Each first
pushes `[0x5D94]`, `[0x5D92]` and calls unit 0x77 entry 11 (`+0x28BE`,
stub `0x85:0x57`; near `push cs; call` from inside unit 0x77 itself),
compares its own amount with that result, and on the "enough" side
pushes the difference to this routine; on the other side it pushes
`DS:0x67F4` and calls unit 0x7f entry 12 (`+0x00A8`, stub `0x8F:0x5C`)
instead. Neither of those two routines is translated here.

| Site | Routine | Pushed word | Context (decoded at the site) |
| --- | --- | --- | --- |
| unit 0x1e `+0x018F` (file `0x4706`) | `+0x00BF` (not a public entry; unit segment `0x22`) | `[bp-0x104] = helper(AX) − [bp+6]` | after string composition through the runtime (`0xAF8:0xB25/0xB0B/0xB98`), a call to `0xBA:0x1073` with `[bp+6]`, a display call `0x709:0x768`, and a prompt `0x3C5:0x39` compared with `0x59`; `[bp+6] > helper` (`ja`, unsigned) → the entry 12 path; else the difference; then `jmp +0x01ED` |
| unit 0x23 `+0x03F3` (file `0x70C7`) | `+0x0361` (not a public entry; unit segment `0x28`) | `[bp-0xA] = helper(AX) − [bp-8]` where `[bp-8]` came from `es:[di+0x3A]` of a far object | `[bp-8] > helper` (`ja`) → the entry 12 path; else near `+0x0239` is called with two words and a far pointer to the local byte `[bp-0xF]`; if that byte is 0 the difference is pushed and this routine called; if not, the call is skipped; then `jmp +0x0467` |
| unit 0x77 `+0x2002` (file `0x2303C`) | `+0x1F52` (not a public entry; unit segment `0x85`) | low word of `(helper(DX:AX) − 0xC8)` | after a prompt `0x3C5:0x39` compared with `0x59`; the helper's `DX:AX` is kept as a 32-bit value (`[bp-2]:[bp-4]`); `DX > 0`, or `DX == 0` and `AX ≥ 0xC8` (`jb`, unsigned) → `[bp-9] = 1`, 32-bit subtract of 0xC8, push `[bp-4]` only, call; else the entry 12 path; then `jmp +0x2053` |

So every caller passes an unsigned word no larger than the helper's
result (two callers) or the low word of a 32-bit difference (unit 0x77;
its high word is dropped at the push). The argument's range is otherwise
unbounded here, which is why all 65,536 values are tested rather than a
subset. No caller reads AX or the record after the call at the site; the
sites described are decoded, not modelled.

## C contract and tests

`pr_u7f_entry10(record, record_size, value, report)`; `record` is host
memory for the object at `[0x5D92]` (the engine frame is 285 bytes;
≥ 0x92 accepted), `value` the pushed word. Statuses: `OK`, `ARGUMENT`
(null record or `record_size < 0x92`, checked on entry, nothing written).
The report carries the loop count (5), the quotient, the remainder and
the written extent `0x88..0x91`.

`tests/recovered/test_overlay_u7f_div5.c`: pinned values 0, 4, 5, 65535,
1, 6, 9, 10, 0x8000, 65530, 65531, 65534, 0xFF and 0x100 with
hand-computed quotients and remainders on a patterned 285-byte record
whose five slots start non-zero; all 65,536 input values against an
incremental restatement that never uses the division operator (plus the
identity `5·q + r = v`, `r < 5`, `q ≤ 13107`), checking the ten bytes and
that every other byte of the record is untouched; 65535 landing as
`00 00 33 33` at `0x8E..0x91`; null record; buffer sizes 0..0x91
(`ARGUMENT`, buffer unchanged) and exactly 0x92 (accepted, bytes
`0x00..0x87` untouched); a 256-byte record; repeated calls replace rather
than accumulate; optional report.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc 15.2.0)
builds the source and the test with no diagnostics and prints `unit 0x7f
entry 10 translation tests passed`; the same with
`-fsanitize=address,undefined -fno-sanitize-recover=all` passes; the
CMake Release build has no warnings; `ctest` passes 38 of 38 targets on
main 318574f (the existing 37 plus `poolrad_overlay_u7f_div5_tests`);
the hygiene workflow's three checks pass locally.

## Limitations, stated plainly

- What the five word slots `+0x88..+0x90` hold, and why the argument is
  split into a quotient and a remainder of 5, are not established;
  "div5" names the mechanism only.
- Unit 0x77 entry 11 (`+0x28BE`, the helper every caller consults first)
  and unit 0x7f entry 12 (`+0x00A8`, the alternate path) are not
  translated; the callers are described as decoded.
- The host-side size guard has no counterpart in the engine.
- The accounting documents are the integration lane's; this note claims
  one full translation for the PM to record.
