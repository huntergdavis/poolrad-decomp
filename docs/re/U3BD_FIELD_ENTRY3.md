# Unit 0x3bd entry 3 (+0x0350, stub 0x3D5:0x002F): full translation (2-bit field lookup)

`src/recovered/overlay_u3bd_field.c` translates DOS v1.3 `GAME.OVR` unit
`0x3bd` public entry 3, the routine four fixup-confirmed callers invoke
(three with the party x/y bytes `[0x6AAD]`/`[0x6AAE]` and the facing byte
`[0x6AAF]`) to read one 2-bit field of the fourth 256-byte quarter of the
1,024-byte buffer at far `[0x6A5C]` that entry 7 fills
([U3BD_WALLSET_ENTRY7.md](U3BD_WALLSET_ENTRY7.md)), gated by the nibble
that entry 2 returns for the same cell and selector. Classification:
**full translation** (FUNCTION_LEDGER.md §1 vocabulary): every
instruction path is reproduced with the same inputs and outputs, the
nested call goes through the merged entry 2 translation
([U3BD_NIBBLE_ENTRY2.md](U3BD_NIBBLE_ENTRY2.md), PR #126), and the one
hidden stack byte that call reads is an explicit input whose value is
shown not to reach the output. The routine makes no far calls; the local
leaf `+0x031F` is accounted for and not counted. `ecl_helpers.c/h`, the
COMPARE files and the resident files are untouched.

`deja "Poolrad unit 0x3bd entry 3 +0x0350"` found no prior session.
Reused with citation and re-checked from the bytes at this entry's own
offsets: the leaf, the map-id gate and the wrap from
[U3BD_CELL_ENTRY4.md](U3BD_CELL_ENTRY4.md); entry 2's contract, including
its unwritten-`[bp-1]` path, from U3BD_NIBBLE_ENTRY2.md; the `[0x6A5C]`
buffer from the entry 7 note; `[0x84DC]` (map id) and
`[0x6AAD]`/`[0x6AAE]` (party x/y) from
[GAMEPLAY_PATH.md](GAMEPLAY_PATH.md), which also names unit 0x68 entry 1
(`+0x0D6D`, a caller below) the view refresh.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x3bd`: file offset 190,711, code size 4,619, eight public entries,
run-time segment `0x3D5`; entry 3 maps to `+0x0350` (stub `0x3D5:0x2F`),
file offset `0x2EC47` (191,559). Decoded with objdump (`-m i8086`) and
Capstone 5.0.7 independently; both agree with the PM's read-only scan.

| Item | Value |
| --- | --- |
| extent | `+0x0350..+0x047A`, **299 bytes, 120 instructions**, one `retf 6` at `+0x0478`; the ledger bound of 299 is exact, no trailing data (entry 2 begins at `+0x047B`) |
| span SHA-256 | `3b0a525351d0a12b3afeed4c9d9d39e28fdf3564166cdab0d16219fad1b60498` |
| frame | `sub sp, 2`; `[bp+0xA]` = x (pushed first), `[bp+8]` = y, `[bp+6]` = selector; all read as bytes; x and y compared signed and extended with `cbw`; `[bp-2]` = working result (set to 1 at `+0x03A3`, overwritten by the four field arms), `[bp-1]` = returned byte (0 on the map gate, else a copy of `[bp-2]` at `+0x046F`) |
| branches | 17, all forward (`+0x0364`, `+0x036B`, `+0x0372`, `+0x0378`, `+0x037F`, `+0x0389`, `+0x0393`, `+0x039D`, `+0x03B9`, `+0x03BB`, `+0x03C3`, `+0x03EC`, `+0x03F0`, `+0x0419`, `+0x041D`, `+0x0446`, `+0x044A`), no loops, no indirect branches |
| call sites | 2, both `push cs; call`: `+0x035F` → the local leaf `+0x031F` (`retf 4`); `+0x03B4` → entry 2 `+0x047B` (`retf 6`); no far calls |
| data-segment reads | byte `[0x84DC]` (map id) twice; far pointer `[0x6A5C]` at four sites (one per field arm); four ES reads, all `es:[di+0x300]` |
| writes | none outside the frame (the clamps rewrite the argument bytes `[bp+0xA]`/`[bp+8]` in place; entry 2 receives copies) |

## Instruction paths (local effects in order; `x` = `[bp+0xA]`, `y` = `[bp+8]`, `s` = `[bp+6]`)

| Offsets | Effect | Model |
| --- | --- | --- |
| `+0x0356..+0x0362` | push x byte, push y byte, `push cs; call +0x031F`; `or al,al` | `in_range = in_grid(x, y)` |
| `+0x0364` | `jne +0x037B`: in range → the clamps (no-ops) and the call | |
| `+0x0366..+0x0372` | `cmp byte [0x84DC],0; je +0x0374; cmp byte [0x84DC],10; jne +0x037B` | out of range: map id 0 or 10 → zero; any other map id → wrap |
| `+0x0374..+0x0378` | `[bp-1] = 0; jmp +0x0472` | `*result = 0`, `returned_zero`; entry 2 is not called and `[0x6A5C]` is never touched |
| `+0x037B..+0x039F` | `x > 15 → 0; x < 0 → 15; y > 15 → 0; y < 0 → 15` (signed, in that order) | the same |
| `+0x03A3` | `[bp-2] = 1` | `value = 1` |
| `+0x03A7..+0x03B4` | push x, push y, push s, `push cs; call +0x047B` | `pr_u3bd_entry2(map, buffer, residue, x, y, s)` |
| `+0x03B7..+0x03BB` | `or al,al; ja +0x03BE` (`or` clears CF, so `ja` ⇔ AL ≠ 0); else `jmp +0x046C` | nibble 0 → keep 1 |
| `+0x03BE..+0x03C3` | `al = s; cmp al,6; jne +0x03EE` | selector chain, order 6, 4, 2, 0 |
| `+0x03C5..+0x03EC` | s = 6: `dx = cbw(x); ax = cbw(y) shl 4; les di,[0x6A5C]; di += ax+dx; al = es:[di+0x300]; and 0xC0; xor ah,ah; shr ax,6; [bp-2] = al; jmp +0x046C` | `(buffer[0x300 + 16·y + x] & 0xC0) >> 6` |
| `+0x03EE..+0x0419` | s = 4: same address; `and 0x30; shr 4` | `(byte & 0x30) >> 4` |
| `+0x041B..+0x0446` | s = 2: `and 0x0C; shr 2` | `(byte & 0x0C) >> 2` |
| `+0x0448..+0x0469` | s = 0: `and 0x03` | `byte & 0x03` |
| `+0x044A` | s ∉ {0, 2, 4, 6}: `jne +0x046C` with `[bp-2]` still 1 | result 1 |
| `+0x046C..+0x0478` | `[bp-1] = [bp-2]; al = [bp-1]`; epilogue; `retf 6` | `OK` |

**What entry 2 sees.** The call passes the wrapped coordinates, so entry
2's leaf returns 1 and its map-id gate is never consulted: for s ∈ {0, 2,
4, 6} it returns the nibble of quarter 0 or 1 (0..15), for any other s it
returns its unwritten `[bp-1]`. The result here is therefore: 0 on this
routine's own map gate; else 1 when the nibble is 0; else the 2-bit field
of the quarter-3 byte at bit position s for s ∈ {0, 2, 4, 6}; else 1.
The quarter-3 read happens only when the nibble is non-zero and s is
recognised, so its offset is always `0x300..0x3FF`.

**The hidden stack byte, located.** Relative to this routine's BP (call
it B; `sub sp,2` makes SP = B−2): the leaf call pushes x, y, CS and the
return offset at B−4..B−11 and the leaf's `push bp; sub sp,2` makes its
`[bp-1]` = B−13, the only byte it writes. The entry 2 call pushes x, y, s,
CS and the return offset at B−4..B−12; entry 2's `push bp` lands at
B−14..B−13, so its BP = B−14 and its unwritten `[bp-1]` is **B−15, this
routine's `[bp-0xF]`**. Nothing in this routine, the leaf or entry 2
writes B−15 before entry 2 reads it; it holds whatever was on the stack
below this frame when the routine was entered. The model takes it as
`state->entry2_frame_residue` and passes it to the merged entry 2 so the
call is reproduced exactly. It cannot reach the output: if the byte is 0
the `ja` at `+0x03B9` falls through to `+0x046C` with `[bp-2]` = 1; if it
is not 0 the chain at `+0x03BE..+0x044A` matches none of 6, 4, 2, 0 (the
selector was unrecognised by entry 2 for the same reason) and reaches
`+0x046C` with `[bp-2]` = 1. Either way AL = 1. The test sweeps residues
0x00, 0xA5 and 0xFF over every input and checks that the result is the
same.

A nil far pointer at `[0x6A5C]` is reported as `ARGUMENT` at the first
read (inside entry 2 for a recognised selector; the quarter-3 read cannot
be reached without that earlier read), not on entry, because the map gate
path and the unrecognised-selector path never touch it.

## Callers (four fixup-confirmed far calls; stub `0x3D5:0x2F`)

| Site | Routine | Pushed (x, y, s) | Context (decoded at the site) |
| --- | --- | --- | --- |
| unit 0x33 `+0x0155` | `+0x012B` (nested, static link at `[bp+6]`) | its own `[bp+0xC]`, `[bp+0xA]`, `[bp+8]` | after it checks both coordinates in 0..15 itself; result 0 → local 1 and done; non-zero → it then calls entry 2 with the same three values (U3BD_NIBBLE_ENTRY2.md) |
| unit 0x68 `+0x02D4` | `+0x0294` (internal) | `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]` | after saving a far pointer from `[bp-5]`; result compared with 3 (`je`), otherwise a long jump |
| unit 0x68 `+0x06C0` | `+0x068D` (internal) | locals `[bp-2]`, `[bp-4]`, `[bp-6]` (the last zero-extended from a byte at `+0x06A4`) | after clearing window-1 word `[0x49D6]+0x5AA`; result 0 → skip; else a table byte at `[0x2736 + s]` is added to `[bp-2]` |
| unit 0x68 `+0x0D9E` | `+0x0D6D` (entry 1, the view refresh of GAMEPLAY_PATH.md) | `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]` | only when window-1 word `[0x49D6]+0x592` < 0xFF; result compared with 1, then 2, then further |

The callers compare the result with 0, 1, 2 and 3, consistent with the
2-bit field plus the default of 1. The facing byte's writers are bounded
in U3BD_NIBBLE_ENTRY2.md (every direct writer keeps it in {0, 2, 4, 6});
the unit 0x68 `+0x068D` site's selector is a local whose range is not
bounded here, which is why the unrecognised-selector path is modelled
rather than assumed unreachable.

## C contract and tests

`pr_u3bd_entry3(state, x, y, selector, &result, report)`; `state` holds
the map id byte, the 1,024-byte host buffer for `[0x6A5C]` and
`entry2_frame_residue` (this frame's `[bp-0xF]`); the three arguments are
the raw pushed bytes; `result` receives AL on `OK`. Statuses: `OK`,
`ARGUMENT` (null state or result; null buffer at a read). The nested call
is the merged `pr_u3bd_entry2`; the leaf is a static helper.

`tests/recovered/test_overlay_u3bd_field.c`: all 65,536 (x, y) byte pairs
for nine selectors (0, 2, 4, 6, 1, 3, 5, 7, 0xFF), three map ids (0, 10,
1) and three residues (0x00, 0xA5, 0xFF) against an independent raw-byte
restatement that includes entry 2's nibble rule, checking the result,
the gate and wrap flags, that the residue reaches entry 2 only for
unrecognised selectors and then always yields 1 without a read, and that
a read is at `0x300..0x3FF` with the selector's shift and a non-zero
nibble; pinned cells with zero and non-zero nibbles in each quarter
(fields 3, 1, 0 and the default), every selector 0..255 at one cell for
six residues, wraps (16, 3), (0xFF, 3), (0x80, 0x80) landing at 0x330,
0x33F, 0x3FF, every map id 0..255 at the gate and in range, quarter 2
inverted with no result changing; null state and result; a null buffer on
the gate and unrecognised-selector paths (never touched, result 0 or 1)
and on a recognised selector (`ARGUMENT` inside entry 2, result
untouched); optional report.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc) builds
the two sources and the test with no diagnostics and prints `unit 0x3bd
entry 3 translation tests passed`; the same with
`-fsanitize=address,undefined` passes; CMake build has no warnings;
`ctest` passes 37 of 37 targets on main a51c544 (the existing targets
plus `poolrad_overlay_u3bd_field_tests`).

## Limitations, stated plainly

- The byte at `[bp-0xF]` is supplied by the host; what the real stack
  holds there at a given call is a property of the caller's recent stack
  use and is not derived here. Its value is shown not to affect the
  result.
- What the 2-bit fields of quarter 3 encode, and why a zero nibble in
  quarters 0/1 forces the result 1, are not established; "field" is a
  positional name only. The callers are described as decoded, not
  modelled.
- The leaf `+0x031F` is a third static copy; a shared header would be one
  coordinated edit if the PM wants it.
- The accounting documents are the integration lane's; this note claims
  one full translation for the PM to record.

## Reproduce

Added 2026-10-22 by par-e (VP i21177). Until then this document described its checks but
gave no command to rerun them. The evidence kind is recorded in
[credit_evidence_kind.csv](credit_evidence_kind.csv) (labels: [CREDIT_EVIDENCE_KIND.md](CREDIT_EVIDENCE_KIND.md)).

Synthetic C tests. These are hand-derived checks of the C, with no DOS bytes, so they are not
an original-byte differential:

```sh
cmake -S . -B build
cmake --build build --target poolrad_overlay_u3bd_field_tests
ctest --test-dir build -R '^poolrad_overlay_u3bd_field_tests$' --output-on-failure
```

| Credit | Exercised by |
| --- | --- |
| `OVL_03BD_0350` | the whole `poolrad_overlay_u3bd_field_tests` executable |

Measured on `ee07bd68`: `poolrad_overlay_u3bd_field_tests` passed (1/1 under ctest).
