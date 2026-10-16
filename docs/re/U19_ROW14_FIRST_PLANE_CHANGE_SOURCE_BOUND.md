# U19 row-14 first EGA plane-change source bound

This independent source audit bounds possible plane-byte changes during the
36-cell U19 row-14 print. It does **not** identify the first value-changing
store: the original CPU instructions and source model do not contain the
prior host backing bytes or active EGA write state. A bounded `deja` query
for U19 row-14 selected stores, host planar first change, and page returned
no indexed session. Reused leads are the merged [CPU source model](U19_ROW14_CPU_STORE_SOURCES.md),
[ordered store-to-crop map](U19_ROW14_STORE_TO_CROP_SOURCE_MAP.md),
[host resolver](U19_ROW14_INTERNAL_PLANE_RESOLVER.md), and
[plane capture integrity gate](U19_ROW14_PLANE_CAPTURE_INTEGRITY.md).

## Original-byte and conditional route lock

Ignored DOS v1.3 `START.EXE` hashes to
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` hashes to
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The expanded START drawer `[0x7417,0x7501)` is 234 bytes, SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
Independent per-instruction hashes and 16-bit decoding give:

| Loaded site / image offset | Length / SHA-256 | CPU operand / next PC |
| --- | --- | --- |
| `RES_0709:+0412` / `0x74A2` | 3 / `4aebe178bf3ad2484c62381353ff8bdaf04cf4204f742af578350e642510f58e` | `ES:[DI] ← DL`; `+0415` |
| `+0438` / `0x74C8` | 3 / same digest | `ES:[DI] ← DL`; `+043B` |
| `+0445` / `0x74D5` | 4 / `5c4fa34099b77e7252dd0389cac1d60bd397e30d44577efd894a5326617ced29` | `ES:[DI] ← 0`; `+0449` |
| `+0451` / `0x74E1` | 4 / same digest | `ES:[DI] ← 0`; `+0455` |

The fixed color-10 branch gives planes 0,1,2,3 the CPU operands
zero,glyph,zero,glyph, first page then second page for each plane. Each
instruction is a write; none reads the prior destination. The accepted
EGA path has 36 cells × 8 source rows × 4 planes × 2 pages = **2,304**
conditional candidate visits and 288 distinct byte offsets. Candidate
ordinal 1 is `+0445`, column 1, source row 0, plane 0, first-page
`ES:[0x1181]`; ordinal 2 is `+0451` to the second page at that offset.
Ordinals 3/4 then use the two glyph sites for plane 1. At the selected
`0x118F` offset, the first possible visits are ordinals 897/898, both zero
sites; 899/900 are glyph sites. These are source order and address bounds,
not observed native visits or plane changes. The page segments at these
instructions are unmeasured; retained post-call A000/A000 words cannot be
promoted to store-time `ES`.

## Why the first changing byte remains unresolved

The pinned private DOSBox-X source revision is
`5a9c7c53481c2e933948039c33256776653f4faf`. The exact
`vga_memory.cpp:331–385` window hashes to
`bdeed38164872c251e714d1a3b778a5f6c3a836e932f78e72a0209fb07e15efc`;
the `580–665` window hashes to
`c68333a8194bdd04e78c09ed187dd3279ca4e17bd6585f18fc84ba37abb1b667`.
For the generic unchained handler, the effective mask derives from live
Sequencer/GC state; the write combines `ModeOperation(CPU operand)` with
the **prior** four-plane backing word. The fast handler uses a separate
masked replacement path. Handler choice, paging translation, bank, map
mask, latch, write mode, and prior backing bytes are not encoded by the
four guest store instructions. A zero CPU operand can change a nonzero
plane, leave a zero plane unchanged, or be suppressed by the mask. A glyph
operand likewise need not produce a changed backing byte. A CPU-source
ordinal is therefore not an EGA plane-change ordinal.

The earliest *possible* change within the conditional fixed-row source
route is ordinal 1, if the first page maps to writable EGA backing and its
effective result differs from the prior plane-0 byte. If its mask is zero,
the same original guest instruction makes no plane-byte change. Both
outcomes fit the current source evidence. No source-only rule can select a
first actual change, even if an eventual `A000:118F` pair is supplied:
earlier offsets and both pages must be ruled out by complete same-run
coverage. The selected witness bit `0x20` maps to source `(122,112)` and
crop `(228..229,0..1)`; that coordinate does not make ordinal 897 a first
writer.

The required native packet is a complete, authenticated same-run candidate
stream from U19 `+02D8` through `+02DD`, with exact `ES:DI`, site and
successor at each candidate; a nonperturbing resolved host backing index
and four plane bytes immediately before/after each reached store; and the
actual handler/mask/latch/write-mode state used for the operation. The
source-locked resolver accepts measured `uvga`/`uvga_fast` state but cannot
infer it from DOS mode alone. Guest A000 reads can load the latch and cannot
serve as passive proof. The p5 version-2 selected-event adapter still
returns a source-only unauthenticated identity and does not establish
coverage or value change. The pR passive packet, pC version-2 join, and
common host phase clock must be validated before any first-change claim.

Current status: **BLOCKED_FIRST_PLANE_CHANGE_NATIVE_STATE**. This gate
adds no live writer, page/RGB timing, native or C parity, or full-frame
credit. Read-only reproduction:

```sh
python3 -m tools.replay.cold_start_codeword_row14_candidate_coverage_gate \
  --source resources/dosbox-x-16a1-private-src
python3 tools/replay/cold_start_codeword_row14_internal_plane_resolver.py \
  --source resources/dosbox-x-16a1-private-src
```

Original executable bytes, source text, RAM, plane snapshots, controller
state, and host RGB remain ignored under `resources/`; the public result
contains only addresses, hashes, counts, and conditional boundaries.
