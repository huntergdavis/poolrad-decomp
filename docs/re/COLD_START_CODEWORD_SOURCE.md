# Cold-start codeword prompt: text and border source boundary

The stable prompt at cold-start sample slots **599–838** is composed by
overlay unit `0x19` entry 1 from resident box effects, fixed Pascal strings,
selected-record glyphs, and an interactive input call. Its six fixed text
rows match the native frame pixel for pixel over their complete cell regions.
The border/background and selected-record effects remain separate from that
match; this note does not claim a complete prompt renderer.

Before this audit, bounded `deja "Poolrad codeword prompt unit 0x19 03F1
02E8 box border font 201 slots 599 838 source composition"` timed out after
20 seconds with no result. The [native codeword route](NATIVE_COLD_START_CODEWORD_MENU_ROUTE.md),
[key-read trace](NATIVE_COLD_START_U19_KEY_READ_BRANCH.md), and
[font contract](FONT_8X8D_201.md) supplied leads. The original offsets,
calls, hashes, and fixed-cell comparison below were checked again. No
original text, glyph, or screen pixels are in this note.

## Pinned caller and composition order

The native route observes resident `0824:0122` calling unit `0x19` entry 1
and returning at `0824:0127` with a codeword prompt visible. The ignored
`GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x19` code begins at file `0x009AC`, is 862 bytes, and hashes
`f93ae8c41b823421115bf6d882588ac3577095d1f9ddace6f487a3bcae33291b`.
Its public entry-1 `+0x08FD` is the documented alias of prologue `+0x00FD`.
The 512 original bytes from `+0x00FD` up to, but excluding, the input call
at `+0x02FD` hash
`cdecd5c9eb6c56eca29a601c4db51ef4f136f45c4e162efa5df097f7a10ea7b6`.

1. At `+0x0119..+0x013E`, six pushed byte-valued words have low bytes
   `(1,1,0x26,0x16,0,0)`. Runtime `0AF8:0B0B` copies the unit's empty
   Pascal string at `CS:+0x0000`; `+0x013A` calls resident `03F1:02E8`.
   The 38-byte caller span hashes
   `42a556be90efe14d010ca2fedb02b7b73756fdca7472ccb3a135a7c89589ef6f`.
   The hash-locked resident box wrapper at expanded image
   `0x041F8..0x04274` is 125 bytes, SHA-256
   `11234f2dd6d838e2a3fc78c94476afb5a4ee0e946440295cf162bfd8e1a077f8`.
   It always calls its local `+0x0137` and `+0x0000` effects before checking
   its copied string length; the empty input skips its optional caption
   printer. The inner visual effects are the first remaining border and
   background dependency. No full-width value is inferred from the six
   `mov al; push ax` pairs.
2. Six fixed Pascal strings are copied with `0AF8:0B0B` and sent to
   resident `0709:0626`, using the same `8X8D1.DAX` block-201 font as the
   [loading frame](COLD_START_LOADING_SOURCE.md). The printer reads low-byte
   column, row, and EGA color, then maps `UpCase(ch) mod 64` through the font.

   | Unit `0x19` print call | String offset | Length | Column | Row | Color index |
   | --- | ---: | ---: | ---: | ---: | ---: |
   | `+0x0157` | `+0x0001` | 37 | 1 | 1 | 10 |
   | `+0x0174` | `+0x0027` | 14 | 1 | 2 | 10 |
   | `+0x0191` | `+0x0036` | 35 | 1 | 5 | 10 |
   | `+0x0202` | `+0x005A` | 33 | 1 | 8 | 10 |
   | `+0x0286` | `+0x007C` | 38 | 1 | 11 | 10 |
   | `+0x02D8` | `+0x00A9` | 36 | 1 | 14 | 10 |

3. Glyph calls at `+0x01BB`, `+0x01E5`, `+0x022C`, and conditionally
   `+0x0269` pass a value from a selected record with `0x40` added, the
   upcase flag clear, color `0x0C`, and one glyph to `0709:01DF`.
   The record selection starts with runtime `0AF8:11C0` at `+0x0111`.
   A later string at `+0x02BB` concatenates a fixed prefix at `+0x00A3`
   with a selected-record field through `0AF8:0B98` and prints in color
   `0x0C`. These record-derived bytes are deliberately left private and
   unmodelled.
4. After a fixed row at `+0x02D8`, the unit reaches resident `0709:09E3`
   at `+0x02FD` for interactive input. The merged native key trace proves
   that a queued Return can reach this call and a BIOS read. This source
   audit does not model keyboard behavior or codeword validation.

## Saved-frame measurements and limit

The saved 640×400 prompt hashes
`a9dc46ae3435d144cb26bb6afdd47e729de7719ce325bcc64e56fedca428aed5`
as canonical RGB24. Adjacent horizontal and vertical pixel pairs are equal,
so it has exact 2× display geometry. Its eight colors are standard EGA RGB
indices `0`, `4`, `6`, `10`, `12`, `13`, `14`, and `15`. Replaying only the six
fixed printer calls with the private 1,416-byte font block (SHA-256
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`)
and comparing every pixel of their **193 complete 8×8 cells** to the stable
native PNG gives **0 mismatches over 12,352 source pixels** (49,408 doubled
host pixels). The check includes both set and clear glyph bits within each
cell, not just foreground pixels.
The same read-only command in the [loading source note](COLD_START_LOADING_SOURCE.md)
reproduces both comparisons from ignored inputs.

The remaining native pixels include the resident box effects and the
selected-record glyph/string effects. The static call chain and saved-frame
colors do not independently prove which inner box instruction writes each
border or background pixel. A full RGB parity claim needs the `03F1:0137`
and `03F1:0000` effects plus the selected record and input/cursor state.
No new heavy native run was started. All original files, font bytes, record
contents, and screenshots remain under ignored `resources/`.
