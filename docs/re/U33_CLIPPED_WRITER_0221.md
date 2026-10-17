# Unit 0x33 +0x0221 clipped grid writer

`pr_u33_clipped_writer_0221` translates the complete call-free
`+0x0221..+0x02B9` routine. It forms byte coordinates from two signed
bytes in an inherited caller frame and two unsigned argument bytes.
It clips the resulting signed coordinates to a 50-column, 25-row
grid, then stores the low byte of glyph+1 through DS `0x6716/0x6718`.
Ordered RAM accesses, both local bytes, arithmetic flags and the far
return frame are included.

## Prior work and game ownership

The bounded recall query
`timeout 30s deja "Poolrad U33 0221 full C grid glyph signed clipping native 6716"`
ended 124 without results. Inspection of current main found the adjacent
[U33 +08A4 recovery](U33_BYTE_FLAGS_08A4.md), ending at `+0x0B96`,
and no independent implementation of this helper. The separately
reviewed [U33 +0B97 pair writer](https://github.com/huntergdavis/poolrad-decomp/blob/0712614e7426339c1c0d9579e6ab47889259367a/docs/re/U33_PAIR_WRITER_0B97.md) uses the same
grid pointer, stride and displacement; this leaf has separate source,
header, test and oracle files. Neither routine is counted here.

U33 public entry 1 has the descriptor-seeded static call path
`+0x1EDE -> +0x1244`, `+0x12D4 -> +0x0810`,
`+0x087E -> +0x02BA`, `+0x02E1 -> +0x0221`.
In the owning `+0x1244` function, `+0x12A1` requests `0x04E9`
(1,257) bytes from the runtime allocation call at `+0x12A5`.
It stores returned AX/DX to DS `0x6716/0x6718` at
`+0x12AA/+0x12AD`, then initializes header bytes 4, 5 and 6.
The allocation matches seven header bytes plus 50*25 cells.
The [U9E mode-5 game-state scan](U9E_0160.md) independently uses this
same far pointer. These observations establish a game-owned grid;
they do not assign a gameplay name to each glyph value.

The oracle pins all 22 aligned near calls. Each immediately preceding
pair is PUSH word SS:[BP+6], PUSH CS: the callee receives an inherited
frame offset rather than its immediate caller's BP.

| Owning function | Call sites |
| --- | --- |
| `+0x02BA` | `02E1, 0315, 0327, 033C, 035E, 036E` |
| `+0x0377` | `0394, 03A4, 03B4, 03C4, 03D6, 03E6, 03F6, 0406` |
| `+0x040F` | `05D9, 05EA, 05FB, 060C` |
| `+0x0615` | `07D4, 07E5, 07F6, 0807` |

The path is a static callable-ownership observation. The runtime
allocation and other caller dependencies are outside this call-free
leaf; there are no unresolved calls inside its complete extent.

## Exact original and ABI

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 bytes; SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Unit 0x33 | GAME file base `0x0C5D1`, public stub segment `0x003B`, entry 1 `+0x1E31` |
| Function | GAME file `0x0C7F2..0x0C88A`; **153 bytes, 64 instructions, zero calls** |
| Function SHA-256 | `9a97dca1163b4aabd6193cea3a81427143709390e7b4aa2dec1a2a727236cd06` |
| Exit | `RETF 8` at `+0x02B7`; end `+0x02BA` exclusive |

The API enters after CALL, with return IP/CS at SS:entry-SP.
Its four word arguments are the inherited frame offset at entry-SP+4,
glyph at +6, row delta at +8 and column delta at +10.
Only the low bytes of glyph and deltas matter; their high bytes are
ignored. The routine saves BP and reserves two local bytes.
Final SP is entry SP + 12 modulo 65536.

For disjoint state, let a and b be the signed bytes at SS:[frame-2]
and SS:[frame-1], and dx/dy the unsigned low argument bytes:

```text
column byte = (5*a + 6*b + 21 + dx) modulo 256
row byte    = (5*a + 10 + dy) modulo 256
```

The column is stored at SS:[new-BP-1] before rereading the row delta,
frame word and a byte. The row is then stored at SS:[new-BP-2].
The clipping order is column<0, column>49, row<0, row>24, using signed
byte comparisons. Consequently an arithmetic coordinate that wraps
into the accepted byte range is accepted.

On a clipped path, AX retains the full wrapped row calculation, CX
the zero-extended column delta, DX the zero-extended row delta, SI
the last sign-extended a, and DI the inherited frame word. BX and ES
are unchanged. All six returned arithmetic flags follow the first
failing CMP8. The early exit still writes both local bytes.

On an accepted path, BL becomes glyph+1 modulo 256; BH is preserved.
AX becomes row*50, DX becomes zero, CX the column, SI retains the last
a, and ES:DI becomes the reloaded grid pointer plus row*50+column.
The byte store uses ES:[DI+7], wrapping the effective offset at 16 bits.
All six returned arithmetic flags follow the final ADD DI,CX.
No undefined arithmetic flag survives either kind of exit; no flag
mask or normalization is used. Other flag bits are preserved.

Ordered aliases are retained. Local writes can alter an inherited
frame byte or the grid pointer before its later load. The grid write
can alter locals, saved BP, return IP/CS or argument bytes. Saved BP
and the far return frame reload from live RAM. DS and SS are preserved;
the leaf has no callbacks or source loops.

Effective offsets wrap at 16 bits; word/LES/RETF reads cross offset
boundaries contiguously with physical A20-off wrapping at 1 MiB.
The instruction bytes must remain immutable, following the adjacent
U33 contract; the original-byte audit checks this before calling C.
Null RAM/CPU or a RAM size other than exactly 1 MiB is rejected before
mutation.

## Validation and parity review

The independent C11 test covers **524,288 cases**: all 65,536 signed
frame-byte pairs under eight column/row delta combinations, dirty
high argument bytes, every computed coordinate byte, all five exits
and all 256 glyph bytes on accepted stores. It derives coordinates
and flag results mathematically, checks every CPU word, local/saved
frame bytes, far return and invalid host arguments. Both the new
target and adjacent U33 byte-flags target pass with
`-Wall -Wextra -Wpedantic -Werror`.

The original-byte oracle covers **821 cases**, comparing the entire
1 MiB RAM and all 14 CPU words. It covers all 64 instruction starts,
all 67 internal CFG edges and all eight clipping outcomes. Fixtures
include signed coordinate corners, random inputs and grid offsets,
16-bit stack and A20 boundaries, local/inherited-frame aliases,
local/grid-pointer aliases, grid/saved-BP/return-frame aliases,
word-add carry, incoming flags and preserved DF. Every original case
returns and leaves the instruction stream unchanged before C runs.

The DOSBox-X normal-core/386 harness checks **512 synthetic cases**:
256 clipping-boundary fixtures and 256 accepted stores covering every
glyph byte, including FF to 00. It compares all 14 CPU words, 32
frame/sentinel bytes, four grid-pointer bytes, both inherited-frame
bytes and an eight-byte grid/sentinel window. No flag or compared
RAM byte is masked. The native harness primes the observed window;
the separate original-byte oracle compares full RAM.

The synthetic native entry frame uses SP `0xEFF4`: return IP/CS at
EFF4/EFF6, inherited frame `0x9002` at EFF8, glyph at EFFA,
row delta at EFFC, column delta at EFFE. Saved BP is at EFF2 and
the locals at EFF0/EFF1; final SP is F000. Code, grid window,
inherited frame and stack are proven disjoint. Interrupts are disabled
from fixture priming through capture, with entry FLAGS `0x0002`;
the harness restores interrupts for DOS output. The native result
SHA-256 is
`c5a7e5a7e938f28911705052f7e1417777e87c8864fce4d8604956e5737c9e7e`.
Its header contains
a DOS allocation segment, so another setup can change the result
hash while retaining parity.

The oracle adapts the separately reviewed U33 +0B97 whole-state and
native harness, [party reorder compilation/CPU primitives](U6A_PARTY_BEFORE_14A9.md),
[unit 0x5E RAM/CPU helpers](U5E_PARTY_VALUE_2879.md), and the
[U7D native interrupt boundary](U7D_CARRY_02F2.md).
Original bytes, generated assembly/COM, results/logs and build files
remain under ignored `resources/`; public fixtures are synthetic.
Static ownership and synthetic native parity do not establish a
naturally reached saved-game frame, loaded game-unit segment or
full-game replay parity.

```sh
cmake -S . -B resources/build-u33-0221 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u33-0221 --target \
  poolrad_overlay_u33_clipped_writer_0221_tests poolrad_overlay_u33_byte_flags_08a4_tests
ctest --test-dir resources/build-u33-0221 \
  -R 'poolrad_overlay_u33_(clipped_writer_0221|byte_flags_08a4)_tests' --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn timeout 180s \
  python3 tools/overlay/u33_clipped_writer_0221_audit.py --unicorn --native
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | Unit `0x33:+0x0221..+0x02B9` |
| `source_candidates` | `src/recovered/overlay_u33_clipped_writer_0221.c` |
| `signals` | Exact hash/CFG, 22 aligned inherited-frame callers, mapped game allocation/grid, independent coordinate/flag cases, original full RAM/CPU and native execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented immutable-code, A20-off RAM/CPU contract |
| `review_notes` | Signed byte clipping after modular coordinate formation; ordered local/frame/grid aliases; inherited frame; natural route unmeasured |

No classifier or count ledger is changed. Strict and identity credit
remain PM review decisions. U33 `+0x1403` is a separate queued fallback
requiring its own complete caller/ownership audit.
