# Unit 0x3bd entry 4 (+0x057A, stub 0x3D5:0x0034): full translation (cell lookup)

`src/recovered/overlay_u3bd_cell.c` translates DOS v1.3 `GAME.OVR` unit
`0x3bd` public entry 4, the routine five fixup-confirmed callers invoke
with the party x/y bytes `[0x6AAD]`/`[0x6AAE]` to read one byte of the
1,024-byte buffer at far `[0x6A5C]` that entry 7 fills
([U3BD_WALLSET_ENTRY7.md](U3BD_WALLSET_ENTRY7.md)). Classification:
**full translation** (FUNCTION_LEDGER.md §1 vocabulary): every instruction
path of the entry and of the local leaf it near-calls is reproduced with
the same inputs and outputs. The routine makes no far calls; its two
data-segment reads are explicit state. "Cell" names the byte at (x, y)
in the buffer's third 256-byte quarter; no further game role is asserted.
`ecl_helpers.c/h`, the COMPARE files and the resident files are untouched.

`deja "Poolrad unit 0x3bd entry 4 +0x057a"` matched no prior work on this
entry. Reused with citation and re-checked from the bytes: the entry 7
note for the `[0x6A5C]` buffer (1,024 bytes, four 256-byte quarters from
block offset 2), [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) for the identities
of `[0x84DC]` (the map id, written from window 0 word 0xF2 at
`+0x3321..+0x332A`) and `[0x6AAD]`/`[0x6AAE]` (the party x/y bytes the
program body initialises to 0x0F/1), and FUNCTION_LEDGER.md §1 for the
load-word pseudo-addresses that read `[0x6AAD]`, `[0x6AAE]`, `[0x6AB1]`.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Unit `0x3bd`: file offset 190,711, code size 4,619, eight public entries,
run-time segment `0x3D5`; entry 4 maps to `+0x057A` (stub `0x3D5:0x34`),
file offset `0x2EE71` (192,113). Decoded with objdump (`-m i8086`) and
Capstone 5.0.7 independently; both agree.

| Item | Value |
| --- | --- |
| extent | `+0x057A..+0x05F2`, **121 bytes, 45 instructions**, one `retf 4`; the ledger bound of 121 is exact, no trailing data (the 10-byte glue routine `+0x05F3` follows) |
| span SHA-256 | `2932e53408f1bba96864ff51e310132dda4c72411bd11cd4127c3d09ba010037` |
| frame | `sub sp, 2`; `[bp+8]` = x (pushed first), `[bp+6]` = y; both read as bytes only, compared signed (`jle`/`jge`) and extended with `cbw`; `[bp-1]` holds the result byte |
| branches | 8 forward (`+0x058E`, `+0x0595`, `+0x059C`, `+0x05A2`, `+0x05A8`, `+0x05B2`, `+0x05BC`, `+0x05C6`), no loops, no indirect branches |
| call sites | 1: `push cs; call +0x031F` (a near call emulating a far call to the unit's local leaf, which ends in `retf 4`); no far calls |
| data-segment reads | byte `[0x84DC]` (map id) twice; far pointer `[0x6A5C]` once (`les di`); one ES read `es:[di+0x200]` |
| writes | none outside the frame (the clamps rewrite the argument bytes `[bp+8]`/`[bp+6]` in place) |

**Local leaf `+0x031F..+0x034D`** (ledger grade B `helper_candidate`,
49 bytes, 18 instructions, `retf 4`, span SHA-256
`ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b`):
`[bp+8]` = a, `[bp+6]` = b; `cmp a,15; jg no; cmp a,0; jl no; cmp b,15;
jg no; cmp b,0; jge yes`; result byte 1 at `+0x0343`, 0 at `+0x033D`,
returned in AL. No globals, no calls. The callgraph records five near
sites to it inside unit `0x3bd`: `+0x035F` (entry 3), `+0x048A` (entry
2), `+0x0589` (this entry), `+0x06E4` and `+0x0805` (entry 5). It is
translated here as a static helper; entries 2, 3 and 5 are not modelled.

## Callers (five fixup-confirmed far calls; stub `0x3D5:0x34`)

Every site pushes `mov al,[0x6AAD]; push ax; mov al,[0x6AAE]; push ax`
(so x = `[bp+8]`, y = `[bp+6]`, AH ignored by the callee) and stores AL
into `[0x6AB1]` immediately after the call.

| Site | Routine | Context (decoded at the site) |
| --- | --- | --- |
| unit 0x1c `+0x2D06` | `+0x2CCF` (internal) | after `0x2B:0x4D` (unit 0x25 entry 9) returns a word, only when `word - 0x7FFF == 0xAC91`, i.e. the word is `0x2C90`; then tests `[0x4431]`, `[0x84E0]`, `[0x84E3]` |
| unit 0x1c `+0x31C2` | `+0x31B4` (internal) | first statement of the routine; then clears window-1 word at `[0x49D6]+0x5AA` |
| unit 0x25 `+0x1A47` | `+0x19DB` (entry 19, stub `0x2B:0x7F`) | after a case chain on a byte in AL adjusts `[0x6AAD]`/`[0x6AAE]`: case 6 decrements `[0x6AAD]` or sets it to 15 when it is not positive; the result is then followed by pushes of `[0x6AAD]`, `[0x6AAE]`, `[0x6AAF]` |
| unit 0x68 `+0x07C2` | `+0x073B` (internal) | after writing `[0x6AB0]` and runtime `0xAF8:0x16A8` with (`ds:0x6DDE`, 3, 1); then tests bit 0 of window-1 word `[0x49D6]+0x594` |
| unit 0x3b4 `+0x019B` | `+0x016D` (entry 1) | only when a local byte is 1, which window-0 word 0xE6 (`[0x49D2]+0x1CC`) being non-zero forces; afterwards `cmp [0x6AB1],0x7F; jbe` branches on the stored result |

The unit 0x25 site, which moves the x byte with the same 0 → 15 wrap this
routine applies, and the unit 0x3b4 site, which inspects the stored byte
against 0x7F, are reported as decoded context; their routines are not
modelled here.

## Instruction paths (local effects in order; `x` = `[bp+8]`, `y` = `[bp+6]`)

| Offsets | Effect | Model |
| --- | --- | --- |
| `+0x0580..+0x058C` | push x byte, push y byte, `push cs; call +0x031F`; `or al,al` | `in_range = pr_u3bd_in_grid(x, y)` |
| `+0x058E` | `jne +0x05A4`: in range → the clamps (no-ops) and the read | |
| `+0x0590..+0x059C` | `cmp byte [0x84DC],0; je +0x059E; cmp byte [0x84DC],10; jne +0x05A4` | out of range: map id 0 or 10 → zero; any other map id → wrap |
| `+0x059E..+0x05A2` | `[bp-1] = 0; jmp +0x05EA` | `*result = 0`, `returned_zero`; `[0x6A5C]` is never touched on this path |
| `+0x05A4..+0x05B4` | `cmp x,15; jle; x = 0` then `cmp x,0; jge; x = 15` (signed) | `x > 15 → 0; x < 0 → 15` in that order |
| `+0x05B8..+0x05C8` | the same for y | `y > 15 → 0; y < 0 → 15` |
| `+0x05CC..+0x05E7` | `dx = cbw(x)`; `ax = cbw(y) shl 4`; `les di,[0x6A5C]`; `di += ax + dx`; `al = es:[di+0x200]`; `[bp-1] = al` | `*result = buffer[0x200 + 16·y + x]` |
| `+0x05EA..+0x05F0` | `al = [bp-1]`; epilogue; `retf 4` | `OK` |

Both values are 0..15 at the read, so the offset is always `0x200..0x2FF`:
the read never leaves the third quarter of the 1,024-byte buffer and
never reaches the heap block header or the other quarters. The only
undefined case is a nil far pointer at `[0x6A5C]` (the buffer never
loaded), which the model reports as `ARGUMENT` at the read, not on entry,
because the zero path returns without touching it (same parity rule as
entry 7's buffer check).

A wrapped value is never written back to the globals: the clamps rewrite
the stack copies of the arguments only, and the callers re-read
`[0x6AAD]`/`[0x6AAE]` themselves.

## C contract and tests

`pr_u3bd_entry4(state, x, y, &result, report)`; `state` holds the map id
byte and the 1,024-byte host buffer for `[0x6A5C]`; `x` and `y` are the
raw pushed bytes (interpreted as signed inside, as the original does);
`result` receives AL. The leaf is `static uint8_t pr_u3bd_in_grid(int a,
int b)` in the same file.

`tests/recovered/test_overlay_u3bd_cell.c`: all 65,536 (x, y) byte pairs
on eight map ids (0, 10, 1, 9, 11, 0x7F, 0x80, 0xFF) against an
independent restatement of the rule on raw bytes (0x00..0x0F in range,
0x10..0x7F wrap to 0, 0x80..0xFF wrap to 15; out of range on map 0 or 10
returns 0 without a read), with the reported offset always inside
`0x200..0x2FF`; every map id 0..255 at (16, 0) (only 0 and 10 return 0,
all others read cell (0, 0)) and in range (map id irrelevant, nothing
wraps); pinned cells including (3, 2) → offset 0x223, (16, 3) → 0x230,
(0xFF, 3) → 0x23F, (0x80, 0x80) → 0x2FF, (0x7F, 0x10) → 0x200, (5, 0xF0)
→ 0x2F5; the other 768 bytes of the buffer inverted without any result
changing; null state and null result (`ARGUMENT`, result untouched); a
null buffer on the zero path (`OK`, 0) and on both read paths
(`ARGUMENT`, result untouched); optional report.

Measured: `cc -std=c11 -Wall -Wextra -Wpedantic -Werror` (gcc) builds
the library and test with no diagnostics and prints `unit 0x3bd entry 4
translation tests passed`; the same with `-fsanitize=address,undefined`
passes; CMake build has no warnings; `ctest` passes 35 of 35 targets on
main 7871b65 (the existing targets plus
`poolrad_overlay_u3bd_cell_tests`).

## Limitations, stated plainly

- The leaf `+0x031F` is translated as a static helper because this entry
  needs it; its other four callers (entries 2, 3 and 5) are not modelled,
  and no count is claimed for the leaf.
- The meaning of the byte returned (what the third quarter of the entry 7
  buffer encodes) and the reason map ids 0 and 10 do not wrap are not
  established here; the callers are described as decoded, not modelled.
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
cmake --build build --target poolrad_overlay_u3bd_cell_tests
ctest --test-dir build -R '^poolrad_overlay_u3bd_cell_tests$' --output-on-failure
```

| Credit | Exercised by |
| --- | --- |
| `OVL_03BD_057A` | the whole `poolrad_overlay_u3bd_cell_tests` executable |

Measured on `ee07bd68`: `poolrad_overlay_u3bd_cell_tests` passed (1/1 under ctest).
