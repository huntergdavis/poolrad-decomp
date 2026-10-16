# U19 row-14 candidate store to expected-crop map

This source-only map relates each candidate EGA drawer byte-store PC in the
fixed U19 row-14 print to a text cell, one eight-pixel source row, and the
expected doubled RGB24 crop. It does not find the first value-changing plane
byte. A bounded `deja` query for the four stores, first change and crop timed
out after 20 seconds without a usable result. Reused leads are the merged
[CPU source model](U19_ROW14_CPU_STORE_SOURCES.md),
[candidate-stream capacity gate](U19_ROW14_CANDIDATE_COVERAGE_GATE.md),
[plane-event classifier](COLD_START_CODEWORD_ROW14_PLANE_EVENT_GATE.md), and
[expected crop](U19_ROW14_EXPECTED_RGB_CROP.md). This note adds only the
source-to-pixel coordinate join; it does not alter their collectors or checks.

## Pinned source and four stores

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The U19 caller `+02C0..+02E2`, file `[0x0C6C,0x0C8E)`, is 34 bytes,
SHA-256 `144b7073fef51cd92e155eaf08dd5c679772c7788a989432d2bafc95356647ce`.
The 37-byte Pascal source at `[0x0A55,0x0A7A)` has length 36 and SHA-256
`b30aab4f52eef30b18a13ad80c2eaf017b15cc6a08c70528a97fdc51539e316f`.
The original EGA drawer `+0387..+0471`, expanded START image
`[0x7417,0x7501)`, is 234 bytes, SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.

| Loaded drawer PC | Original image | Next PC | Color-10 source | Page |
| --- | --- | --- | --- | --- |
| `+0412` | `0x74A2` | `+0415` | glyph byte, plane 1 or 3 | first |
| `+0438` | `0x74C8` | `+043B` | glyph byte, plane 1 or 3 | second |
| `+0445` | `0x74D5` | `+0449` | zero, plane 0 or 2 | first |
| `+0451` | `0x74E1` | `+0455` | zero, plane 0 or 2 | second |

Each three-byte glyph-store instruction hashes to
`4aebe178bf3ad2484c62381353ff8bdaf04cf4204f742af578350e642510f58e`;
each four-byte zero-store instruction hashes to
`5c4fa34099b77e7252dd0389cac1d60bd397e30d44577efd894a5326617ced29`.
The color-10 source path takes planes 0, 1, 2, 3 in zero, glyph, zero,
glyph order. Both page stores use the same byte offset but can name
different segments. The retained `+02DD` values `A000/A000` are post-call
values, not a measurement at any of these PCs.

## Ordered numeric map

For a one-based candidate ordinal `n` in **1..2304** on the conditional
complete EGA route, let `i=n-1`, `cell=i/64`, `glyph_row=(i%64)/8`,
`plane=(i%8)/2`, and `page=i%2`, using integer division. The printer column
is `cell+1` (1..36), and the store byte offset is
`(112+glyph_row)*40 + column`. The route therefore covers **288 distinct
byte offsets**: 36 columns in each of eight source rows, between `0x1181`
and `0x12BC`. Each offset has eight conditional store-PC visits; each of
the four PCs has **576** visits, for **2,304** total. These are source-route
counts, not native observations. The merged p5 capacity gate already checks
the ordered candidate stream and explains why the older 128-hit cap cannot
reach the selected witness.

For bit position `b` from 7 down to 0 in a mapped byte, the source pixel is
`x=8*column + (7-b)`, `y=112+glyph_row`. Its four doubled host pixels are
`(2*x..2*x+1, 2*y..2*y+1)`. Within the 576×16 expected crop, their origin
is `crop_x=16*cell + 2*(7-b)`, `crop_y=2*glyph_row`. A set font bit has
source EGA index 10 and expected RGB `(85,255,85)`; a clear bit has index 0
and expected RGB `(0,0,0)`. The caller supplies the original 36 source
bytes and decoded 1,416-byte font to the merged expected-crop adapter;
neither is copied into this note. Its 27,648-byte private crop hashes to
`e6f630a6abf12a030e92387a2966f7ab15abff63f8a8a57b9b8988e516dff980`:
**2,304** source pixels, **802** color-10 and **1,502** black.

At column 15, glyph row 0, the mapped offset is `0x118F`. Ordinals
**897..904** visit that offset in plane/page order
`0/0, 0/1, 1/0, 1/1, 2/0, 2/1, 3/0, 3/1`; the corresponding PCs are
`+0445,+0451,+0412,+0438,+0445,+0451,+0412,+0438`. The `0x20` bit
maps to source `(122,112)`, host `x=244..245,y=224..225`, and crop
`x=228..229,y=0..1`. Its source-derived expected RGB is `(85,255,85)`.
An earlier cell can have the first actual plane-byte change; ordinal 897
is only the earliest candidate at this selected byte if the relevant page
segment is `A000` at the instruction.

## Boundary for a measured first change

The merged plane-event classifier can report which of four **supplied**
before/after plane bytes changed and which changed the selected `0x20` bit.
This map converts that event's authenticated candidate ordinal, offset,
plane, page and byte-bit position to source and host coordinates. It cannot
derive a changed plane from the CPU source byte: Graphics Controller mode,
latches, masks and the prior plane values determine the result. A packet
with only a selected pair identifies at most its first *observed* change;
calling it the first guest writer requires complete same-run candidate
coverage and nonperturbing plane samples from the p5 collector, plus the
pC route join. A different page segment or source route stops this fixed
map rather than silently reusing its ordinal.

The retained `+02DD` host crop differs from the source crop at **1,908**
host pixels. At U19 `+0302`, the supplied completed RGB24 row-14 crop has
the expected SHA-256 above and **zero** differing host pixels. The complete
host frame at `+0302` hashes to
`b01aed35573d243847675d969712a3606336ca28dd9e2d34885e965c8040a683`.
This is a host presentation endpoint, not a store-PC trace or guest page
identity. A separate completed-frame RGB24 comparator contract must consume
owner-authenticated same-run p5/pR packets and the pC join without copying
their provenance checks into this source map.

Only addresses, hashes, counts and numeric coordinates are published.
Original bytes, text, font, RAM, planes and RGB stay under ignored
`resources/`. No native, page, first-writer, full-frame, C parity or game
function count follows from this map.
