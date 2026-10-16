# U19 row-14 alternate page and expected-crop source map

This read-only source gate maps the fixed 36-cell U19 row-14 printer to
hypothetical active/draw page words `A000` and `A200`. It does not assert that
either alternate state occurred. The four retained U19 return-side RAM stops
have `A000/A000`, page indexes `0/0`; none samples a store instruction or the
host display latch. A bounded `deja` query for the U19 event clock and alternate
page/crop map found no matching indexed session. Reused, merged leads are the
[page-selector audit](COLD_START_CODEWORD_ROW14_PAGE_SELECTOR.md),
[conditional 2,304-store model](U19_ROW14_CANDIDATE_COVERAGE_GATE.md),
[store-to-crop map](U19_ROW14_STORE_TO_CROP_SOURCE_MAP.md), and
[expected RGB adapter](U19_ROW14_EXPECTED_RGB_CROP.md).

## Original-byte boundary

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The original active-page helper `RES_0709:+0071..+009F`, image
`[0x7101,0x7130)`, is 47 bytes, SHA-256
`26854c57a4d438a0e37a6c00ab8a7a6b9bff9567da09f681ee4247aeccd42b77`.
It requests BIOS `AH=05` and writes `A000+(page<<9)` to `DS:4A16`. The
separate draw-page helper `+00A0..+00B6`, image `[0x7130,0x7147)`, is 23
bytes, SHA-256
`6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d`;
it writes the same formula to `DS:4A18` without that BIOS call. Thus source
page indexes 0 and 1 yield segment words `A000` and `A200`, respectively;
the latter is `0x2000` bytes above the former by real-mode address arithmetic.

The drawer reads the active word at `+040A/+043D` and the draw word at
`+0430/+0449`, immediately before the glyph stores `+0412/+0438` and zero
stores `+0445/+0451`. Its complete original image
`[0x7417,0x7501)` is 234 bytes, SHA-256
`3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4`.
The original-byte page-selector audit passed against the ignored retained
U19 run: startup helper calls and relocated runtime target matched; all four
post-call page states were `A000/A000`; its bounded input and U19 suffix have
no exact direct far call to either page helper. Those post-call states do not
prove the words' values at each earlier store.

## Conditional page cases at byte offset `0x118F`

The accepted EGA source route has 36 cells, eight glyph rows, four plane
iterations, and two page stores: 2,304 candidate PC visits. Column 15, glyph
row 0 reaches byte offset `0x118F` at ordinals 897–904. Source plane order is
0, 1, 2, 3; color 10 chooses zero, glyph, zero, glyph. The first page uses
`DS:4A16`, the second uses `DS:4A18`. This table filters only the exact
effective `ES:DI=A000:118F`; it is not a count of actual value changes.

| Hypothetical active/draw words | Exact A000 witness ordinals and PCs | A200 witness ordinals | Source-only result |
| --- | --- | --- | --- |
| `A000/A000` | 897 `+0445`, 898 `+0451`, 899 `+0412`, 900 `+0438`, 901 `+0445`, 902 `+0451`, 903 `+0412`, 904 `+0438` | none | Both page stores alias the same segment:offset. A later store can overwrite an earlier one. |
| `A000/A200` | 897 `+0445`, 899 `+0412`, 901 `+0445`, 903 `+0412` | 898, 900, 902, 904 | Only the first-page stores can match the exact A000 witness. |
| `A200/A000` | 898 `+0451`, 900 `+0438`, 902 `+0451`, 904 `+0438` | 897, 899, 901, 903 | Only the second-page stores can match the exact A000 witness. |
| `A200/A200` | none | 897–904 | An exact `A000:118F` target filter must stop as target-page absent. |

The merged candidate iterator reproduced every row above. With an `A200`
segment, `A200:118F` has the same real-mode linear address as `A000:318F`,
but the supplied CPU-frame filter compares exact `ES:DI` fields and must not
silently relabel it as `A000:118F`. Any different page word, changed drawer
path, incomplete candidate stream, or unmeasured Graphics Controller state
stops this mapping. No row in the table identifies a first changed plane.

## Expected crop and display boundary

The independently decoded 1,416-byte font block 201 SHA-256 is
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`;
the original 37-byte length-prefixed U19 source at `[0x0A55,0x0A7A)` has
length 36 and SHA-256
`b30aab4f52eef30b18a13ad80c2eaf017b15cc6a08c70528a97fdc51539e316f`.
A strict C11 private smoke rebuilt from the public expected-crop adapter,
using those ignored inputs, reproduced the 27,648-byte expected RGB24 crop
SHA-256
`e6f630a6abf12a030e92387a2966f7ab15abff63f8a8a57b9b8988e516dff980`.
It represents 36 cells at source `(8..295,112..119)`, doubled to host
`x=16..591,y=224..239`: 802 color-10 and 1,502 black source pixels.

The *desired* crop has the same relative screen coordinates for either page
if that page's printed planes are actually selected, decoded with the same
palette, and presented in a completed frame. CPU source bytes alone cannot
establish those plane contents. A write to a draw page while a different
active page remains latched cannot be assigned to the visible RGB crop.
`DS:4A16/4A18`, BIOS request, CRTC `display_start`, latched `real_start`,
renderer page, palette, and completed host frame are distinct observations.
The retained `+0302` RGB crop matching the expected hash establishes a host
endpoint only; it does not choose a store page, first writer, or alternate
page route.

Reproduce the source and private-input gates without an emulator launch:

```sh
python3 tools/replay/cold_start_codeword_row14_page_selector_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/native-codeword-read-005
python3 -m unittest tests.replay.test_cold_start_codeword_row14_candidate_coverage_gate
```

Only addresses, hashes, counts and numeric coordinates belong in this public
note. Original media, text, font, RAM, planes and RGB remain ignored under
`resources/`. There is no live alternate-page, first-writer, page-selection,
host-frame, native parity, or C-native parity claim.
