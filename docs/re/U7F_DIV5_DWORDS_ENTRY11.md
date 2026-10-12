# Unit 0x7f entry 11 (+0x0185, stub 0x8F:0x0057): full translation of the code bytes (five-dword clear, div 5 split); game ownership evidenced, not asserted

`src/recovered/overlay_u7f_div5_dwords.c` translates DOS v1.3 `GAME.OVR`
unit `0x7f` public entry 11, the 86-byte leaf three fixup-confirmed
callers invoke with one word to zero the five dwords at `DS:0x67F4..0x6807`
and then store the word's unsigned quotient by 5 at `0x6804` and its
remainder at `0x6800`, both zero-extended to 32 bits. It is the
data-segment twin of entry 10 ([U7F_DIV5_ENTRY10.md](U7F_DIV5_ENTRY10.md),
PR #130), which does the same with word slots of the active-character
record. Classification of the bytes: **full translation** (FUNCTION_LEDGER.md
§1 vocabulary): every instruction path of the reachable code is reproduced
with the same input and the same twenty output bytes; no calls; nothing is
read. Classification of the routine's owner: see "Ownership" below; **no
strict game-function credit is claimed** by this note.

`deja "Poolrad unit 0x7f +0x0185 entry 11 0x67F4"` and one broader query
found only this session. Reused with citation and re-checked here: the
three caller routines and their entry 12 path from U7F_DIV5_ENTRY10.md;
the classifier's rules and rows from
[OVERLAY_CLASSIFICATION.md](OVERLAY_CLASSIFICATION.md) and
`overlay_classification.csv`; the ledger row (`0x7f:0x0185`, grade A,
bound 126, 125 decoded, 54 "instructions", `retf 2`, 3 far callers) from
`overlay_function_ledger.csv`.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x7f`: code at file offset 150,665 (`0x24C89`), code size 8,795,
fifteen public entries, run-time segment `0x8F`; entry 11 maps to
`+0x0185` (stub `0x8F:0x57`, `(0x57 - 0x20) / 5 = 11`), file offset
`0x24E0E` (151,054). Decoded with objdump (`-m i8086`) and Capstone 5.0.7
independently; both give the same 34 instructions over the 86 code bytes
and agree with the PM's read-only scan.

| Item | Value |
| --- | --- |
| code extent | `+0x0185..+0x01DA`, **86 bytes, 34 instructions**, one `retf 2` at `+0x01D8`; nothing branches past it |
| code SHA-256 | `93f37f967ba626b2d9646c827e3434121fd47b4c7425fb351cc61be6326e780e` |
| ledger span | `+0x0185..+0x0202`, 126 bytes, SHA-256 `05fc05aba6193f60789cb18746fd9cebaa3ab630cdcf405eed53d70af362cc90`; the extra 40 bytes are data (below), which is why the ledger's "54 instructions / 125 decoded" overshoot |
| trailing data | `+0x01DB..+0x0202`, 40 bytes, SHA-256 `dff8d32858b2413141dd2b139559b8b9cfa828b78f0e6427cde088cee755bb01`: a length-prefixed string constant (length byte `0x27`), referenced by `mov di, 0x1DB; push cs; push di; lcall 0xAF8:0xB0B` at `+0x0226` inside the next routine `+0x0203` (which begins with a prologue at `+0x0203`); not an instruction stream (Capstone decodes it as `daa`, `outsw`, `popaw`, …) and never reached or read by entry 11. The text is not reproduced here. |
| frame | `sub sp, 2`; `[bp+6]` = the one pushed word (`retf 2`); `[bp-1]` = loop counter byte (0..4); `[bp-2]` is never touched |
| branches | 2: `jmp +0x0194` (forward, skips the first `inc`) and `jne +0x0191` (backward, the loop); no indirect branches |
| call sites | none (near or far) |
| DS writes | fourteen word stores: `+0x01A0` / `+0x01A4` (`[di+0x67F4]` / `[di+0x67F6]`, `di = 4i`, five passes each), `+0x01BA` `[0x6804]`, `+0x01BD` `[0x6806]`, `+0x01CE` `[0x6800]`, `+0x01D1` `[0x6802]`; bytes `0x67F4..0x6807` |
| DS reads | none; no far pointer, no ES use |
| registers at return | AX = remainder, DX = 0, CX = 5, DI = 16; no caller reads any of them (each jumps away immediately) |

## Instruction paths (local effects in order; `v` = `[bp+6]`, `B` = the block at `DS:0x67F4`, dword slots `B[k]`)

| Offsets | Effect | Model |
| --- | --- | --- |
| `+0x0185..+0x0188` | prologue, `sub sp, 2` | |
| `+0x018B..+0x018F` | `[bp-1] = 0; jmp +0x0194` | `i = 0` |
| `+0x0194..+0x01A4` | `al = [bp-1]; cbw; di = ax; shl di, 1; shl di, 1; ax = 0; [di+0x67F4] = ax; [di+0x67F6] = ax` | `B[i] = 0` (two word stores); `cbw` never sign-extends because `i ≤ 4` |
| `+0x01A8..+0x01AC` | `cmp byte [bp-1], 4; jne +0x0191` (`inc [bp-1]`, back to the body) | do-while: `i = 0..4`, slots at `0x67F4, 0x67F8, 0x67FC, 0x6800, 0x6804` |
| `+0x01AE..+0x01B6` | `ax = v; xor dx, dx; cx = 5; div cx` | unsigned `0:v ÷ 5`: `AX = v div 5` (≤ 13107), `DX = v mod 5`; constant divisor, no `#DE` |
| `+0x01B8..+0x01BD` | `xor dx, dx; [0x6804] = ax; [0x6806] = dx` | `B[4] = v div 5` zero-extended (the remainder in DX is discarded here) |
| `+0x01C1..+0x01C9` | `ax = v; xor dx, dx; cx = 5; div cx` | the identical division again |
| `+0x01CB..+0x01D1` | `xchg dx, ax; xor dx, dx; [0x6800] = ax; [0x6802] = dx` | `B[3] = v mod 5` zero-extended |
| `+0x01D5..+0x01D8` | `mov sp, bp; pop bp; retf 2` | done |

Net state: `B[0] = B[1] = B[2] = 0`, `B[3] = v mod 5`, `B[4] = v div 5`,
all as little-endian dwords with zero high words; the loop's zeros in
slots 3 and 4 are overwritten, so the clear-then-store order is
unobservable in the final state but reproduced anyway. Slots 5 and 6 of
the 28-byte block (`0x6808..0x680F`) and everything else are untouched.
The division is unsigned (`xor dx, dx; div`, not `cwd; idiv`): 65535
gives 13107 r 0.

**Data boundary.** The ledger bound reaches the next prologue at
`+0x0203`. The code ends at `retf 2` (`+0x01D8..+0x01DA`); the only
backward branch targets `+0x0191` and the only forward branch `+0x0194`,
so no path reaches `+0x01DB`. The 40 bytes there are the string constant
described above, owned by `+0x0203`.

**Null pointer.** The routine addresses DS directly, so there is no
pointer to be nil and no failure path. The translation returns `ARGUMENT`
on entry, before any write, for a null host block or a host buffer
shorter than 0x14 bytes; that guard is host-side only.

## The block at DS:0x67F4 (decoded references; counts, not meanings)

Every code reference to `0x67F4..0x6807` in `GAME.OVR` was located by
scanning for the little-endian address after a plausible opcode; none is
in `START.EXE` (the resident image, which holds the Turbo Pascal runtime).

| Where | Routine (classifier row) | What the bytes do |
| --- | --- | --- |
| unit 0x1c `+0x1276` | `+0x125D`, **game_verified** (ECL PC `[0x49ED]`) | `mov di, 0x67F4; push ds; push di; push 0x1C; push 0; lcall 0xAF8:0x16A8`: runtime fill of the 28-byte block with 0 |
| unit 0x1e `+0x0D26` | `+0x0D09`, public entry 1 | the same fill call with the same three arguments, right after `push [0x5D92]; lcall 0xBA:0x767` |
| unit 0x23 `+0x055C` | `+0x053E`, public entry 1 | the same fill call, same preceding sequence |
| unit 0x7f `+0x00AE..+0x00BC` | entry 12 `+0x00A8`, helper_candidate | copies 0x1C bytes from its far-pointer argument (`lcall 0xAF8:0xA3F`) into a local, then combines the five local dwords with constants at `CS:0x0D24 + 4i` and `CS:0x0D30` through runtime `0xAF8:0xA5B` / `0xAF8:0xA76`, returning a dword in `DX:AX`; the three callers below pass `DS:0x67F4` to it |
| unit 0x20 `+0x0233..+0x029C` | `+0x0000` (853-byte bound) | reads all five dwords `0x67F4, 0x67F8, 0x67FC, 0x6800, 0x6804` (and `0x6808`), combining them with constants `0xC8, 0x14, 2, (1), 5` through `0xAF8:0xA76` / `0xAF8:0xA5B` into a local dword |
| unit 0x77 `+0x1E83` | `+0x1CEE` | `add [0x6804], ax; adc [0x6806], dx` right between `add es:[di+0x90], ax` and `add es:[di+0x8E], ax` on the active-character record at `[0x5D92]` |
| unit 0x7f `+0x025E..+0x026D` | `+0x0203`, game_evidenced (active_char, string) | `ax:dx = [bp+6] + [0x6804]:[0x6806] − local; [0x6804]:[0x6806] = ax:dx`, after `add es:[di+0x90], ax` on the record at `[0x5D92]` |
| unit 0x7f `+0x0185` | entry 11, this routine | the five-dword clear and the div 5 split |

So the block is a 28-byte, seven-dword structure (fill length `0x1C`,
copy length `0x1C`, reads up to `0x6808`); this routine writes its first
five dwords. Slots 3 and 4 (`0x6800`, `0x6804`) are the ones the other
writers add to alongside the record words `+0x8E` and `+0x90` that entry
10 writes. What the slots hold is not asserted here.

## Callers (three fixup-confirmed far calls; stub `0x8F:0x57`)

The same three routines that call entry 10, at the sites just after
their entry 12 call (decoded from each prologue in U7F_DIV5_ENTRY10.md
and re-decoded here from `+0x0196`, `+0x03FA`, `+0x2009`).

| Site | Routine (classifier row) | Pushed word | Context (decoded at the site) |
| --- | --- | --- | --- |
| unit 0x1e `+0x01CC` (file `0x4743`) | `+0x00BF`, game_evidenced (5 strings, active_char) | low word of `(entry12(DS:0x67F4) − zero-extended [bp+6])` | entry 12's `DX:AX` is kept in `[bp-0x106]:[bp-0x108]`; `0 < high` (signed `jl`) → proceed; `0 > high` (`jg`) or `high == 0` and `[bp+6] > low` (unsigned `ja`) → a string from `CS:0xA3` through `0xAF8:0xB0B` and `0xBA:0x1693` instead; else 32-bit subtract, push the low word, call; then `jmp +0x01ED` |
| unit 0x23 `+0x044C` (file `0x7120`) | `+0x0361`, game_evidenced (1 string, active_char) | low word of `(entry12(DS:0x67F4) − zero-extended [bp-8])` | same guard shape on `[bp-0xC]:[bp-0xE]`; then near `+0x0239` with two words and a far pointer to the local byte `[bp-0xF]`; that byte 0 → 32-bit subtract, push low word, call; byte ≠ 0 → skip the call; the "too large" side shows a string from `CS:0x34F`; then `jmp +0x0467` |
| unit 0x77 `+0x2038` (file `0x23072`) | `+0x1F52`, game_evidenced (5 strings, active_char) | low word of `(entry12(DS:0x67F4) − 0xC8)` | `high > 0` (signed `jg`), or `high == 0` and `low ≥ 0xC8` (unsigned `jb` otherwise) → `[bp-9] = 1`, 32-bit subtract of 0xC8, push `[bp-8]` only, call; else a string from `CS:0x1EFF`; then `jmp +0x2053` |

Each caller therefore passes the low word of a 32-bit difference that
the guard keeps non-negative; the high word is dropped at the push, so
the routine's input range is unbounded here and all 65,536 values are
tested. No caller reads AX, DX or the block after the call at the site.

## Ownership, stated against the classifier's rules

`overlay_classification.csv` puts this routine in
`game_or_other_library_unresolved` because it has none of the three
per-routine signals (a string constant, a documented engine global, a
call into a verified routine); OVERLAY_CLASSIFICATION.md says routines
"touching other game state fall into the unresolved class until those
globals are documented". The evidence this note adds for the PM:

- The block it writes is zero-filled, with the explicit length 0x1C, by
  a **game_verified** routine (unit 0x1c `+0x125D`) and by two public
  entries; it is read by entry 12 and by unit 0x20's reader; it is
  updated by two routines that in the same path update the
  active-character record words entry 10 writes. No resident (runtime)
  code references it.
- All three callers are **game_evidenced**, and each pushes a value
  derived from entry 12 applied to this same block.
- The routine is the data-segment twin of entry 10, which PR #130 merged
  as game_evidenced through the `[0x5D92]` global.

None of this is a per-routine signal under the current rules, so this
note does **not** claim game-function credit. If the PM documents
`0x67F4..0x680F` as an engine global on the strength of the
game_verified writer, the routine gains the "documented engine global"
signal and moves to `game_evidenced` by the existing rule; that decision
is the integration lane's.

## C contract and tests

`pr_u7f_entry11(block, block_size, value, report)`; `block` is host
memory whose byte 0 is `DS:0x67F4` (the engine block is 28 bytes; ≥ 0x14
accepted), `value` the pushed word. Statuses: `OK`, `ARGUMENT` (null
block or `block_size < 0x14`, checked on entry, nothing written). The
report carries the loop count (5), the quotient, the remainder and the
written extent `0x00..0x13`.

`tests/recovered/test_overlay_u7f_div5_dwords.c`: pinned values 0, 4, 5,
65535, 1, 6, 9, 10, 0x8000, 65530, 65531, 65534, 0xFF and 0x100 with
hand-computed quotients and remainders on a patterned block whose twenty
bytes start non-zero (so the clears and the zero high words are
observed); all 65,536 input values on a 28-byte block against an
incremental restatement that never uses the division operator (plus the
identity `5·q + r = v`, `r < 5`, `q ≤ 13107`), checking the five dwords
and that slots 5 and 6 are untouched; 65535 landing as `00 00 00 00 33 33
00 00` at `0x0C..0x13`; null block; sizes 0..0x13 rejected with the
buffer unchanged and exactly 0x14 accepted; the 28-byte and a 64-byte
host buffer; repeated calls replace rather than accumulate; optional
report.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc 15.2.0)
builds the source and the test with no diagnostics and prints `unit 0x7f
entry 11 translation tests passed`; the same with
`-fsanitize=address,undefined -fno-sanitize-recover=all` passes; the
CMake Release build has no warnings; `ctest` passes 39 of 39 targets on
main 963c3b7 (the existing 38 plus `poolrad_overlay_u7f_div5_dwords_tests`);
the hygiene workflow's three checks pass locally; a grep confirms no file
in the change reproduces the trailing string text.

## Limitations, stated plainly

- What the seven dword slots hold, why the split is by 5, and what the
  callers' thresholds (0xC8, the per-caller amounts) mean are not
  established; "div5" names the mechanism only.
- Game ownership is evidenced by cross-references, not by a per-routine
  signal; no strict game-function credit is claimed (see "Ownership").
- Entry 12 (`+0x00A8`), unit 0x77 entry 11 (`+0x28BE`), the `+0x0203`
  routine and unit 0x20's reader are decoded at the cited sites only, not
  translated.
- The host-side size guard has no counterpart in the engine.
- The accounting documents are the integration lane's.
