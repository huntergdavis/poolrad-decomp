# Cold-start loading frame: source contract

The stable loading frame at cold-start sample slots **133–215** is text drawn
from the program's own Pascal string and the shared `8X8D1.DAX` font. A
source-derived 640×400 RGB24 reconstruction matches the saved native frame
byte for byte. This is an asset and draw-call contract, not a portable C
startup route or an instruction-level stop at the printer.

Before this audit, bounded `deja "Poolrad cold start loading screen slots 133
215 RGB be0ff6f TITLE DAX asset EGA palette source"` timed out after 20
seconds with no result. The [cold-start screen ledger](NATIVE_COLD_START_SCREEN_GAPS.md),
[startup map](GAMEPLAY_PATH.md), and [font audit](FONT_8X8D_201.md) supplied
the route and format leads. All offsets and hashes below were checked again
against ignored DOS v1.3 inputs. No original string, glyph, or pixel bytes
are in this note.

## Pinned sources and caller

| Private source | Bound or SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 bytes; `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `8X8D1.DAX` | 9,711 bytes; `cce9313f23a600200fec60b5d46cd08aa9535a6143eee5d5d768a1d4e0415021` |
| decoded block ID 201 | 1,416 bytes, 177 eight-byte glyphs; `fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1` |
| saved native frame | 640×400 RGB24; `be0ff6f19aa525049a12a3794c3fe4475ce7549030ed6d5e8f7d7cb41635100f` |

The startup program body calls unit `0x36` entry 1 at resident image
`+0x00FD` before the later unit `0x17` picture call at `+0x011D`.
Unit `0x36` has code at `GAME.OVR` file `0x0E5D3`, 1,562 bytes,
SHA-256 `6e830341892bfe5f50089e3001c3d69b335f567dd801f3d0a0cf5712a0987060`.
Its entry alias `+0x082D` resolves to the exact prologue `+0x002D`.

At unit `0x36:+0x0481`, a five-byte far call (SHA-256
`610b44e72e8ee8c82afedf0b15544d6967f8b05c45a736de351f82173893cd09`)
reaches resident `0709:017C`. The merged font audit establishes that this
callee requests `8X8D1.DAX` block 201 and copies its whole decoded payload
into the font buffer at `[0x5E20]`.

Unit `0x36:+0x048B..+0x04A7` is 29 original bytes, SHA-256
`7c131c0aade15d6faf368f6fba4b2faf34c0274b5e888e3f1e63a4f9390c82f4`.
It pushes low-byte column `0`, row `24`, and color `0x0A`, copies the
code-segment Pascal string at `+0x0000` through runtime `0AF8:0B0B`, then
calls the resident string printer at `0709:0626` from `+0x04A3`.
The source string is 21 characters, stored as 22 length-plus-content bytes at
file `0x0E5D3..0x0E5E8`, SHA-256
`3268e5ee46c67c0bb66ab90e8f2c5b34da5aa23c6ab52d5b19fb80ceba9071af`.
Two other copies of the same 21-character content exist in units `0x25` and
`0x6F`; this particular print call explicitly passes the unit `0x36` copy.
Only the low bytes of the three pushed words are needed by the printer;
their unexamined high bytes are not assigned values here.

The [printer and glyph contract](FONT_8X8D_201.md) maps each character
through `UpCase(ch) mod 64` to eight font rows, with bit 7 at the left.
A set bit receives EGA color index `10`, a clear bit index `0`. The saved
frame uses exactly RGB `(85,255,85)` for its 1,916 nonblack host pixels and
RGB `(0,0,0)` for the other 254,084. Every adjacent horizontal and vertical
pixel pair is equal. Its nonblack rectangle is host x `2..335`, y
`384..399`; row `24` means source y `192..199` and host y `384..399`.

## Read-only source-to-frame comparison

The private check used the hash-verified decoded block 201, the unit `0x36`
length-plus-content bytes above, and the saved PNG from the screen ledger.
It began with a black 320×200 index frame, applied the documented printer
glyph mapping at `(column,row,color)=(0,24,10)`, assigned standard EGA RGB,
and replicated each source pixel 2× in both axes. The 21 cells contain
**479 set source pixels**. The resulting 768,000 RGB bytes hash to
`be0ff6f19aa525049a12a3794c3fe4475ce7549030ed6d5e8f7d7cb41635100f`:
**0 differing bytes and 0 differing pixels** versus the saved native frame.
The comparison was in memory; no new media was created or tracked.

With the decoded block 201 produced by the existing DAX reader and the
native survey's saved frames under ignored `resources/`, run the public
read-only auditor from the repository root:

```sh
python3 tools/replay/cold_start_text_source_audit.py \
  resources/DOS/POOLRAD/GAME.OVR "$FONT_BLOCK_201" \
  "$NATIVE_STABLE_FRAMES/0133-0215.png" \
  "$NATIVE_STABLE_FRAMES/0599-0838.png"
```

The two shell variables are paths to ignored private inputs. The auditor
checks their hashes and geometry, emits counts and hashes only, and does not
write files.

The separate native gate capture showed this loading image before resident
`0824:011D`, but did not stop at unit `0x36:+0x04A3` or the printer. The
static call path and exact frame match establish the source contract; they
do not prove guest draw timing, the hardware palette instruction, or all
startup side effects. Original files, decoded font, and screenshots remain
under ignored `resources/`.
