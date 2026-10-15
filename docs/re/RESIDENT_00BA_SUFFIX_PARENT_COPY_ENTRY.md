# Copied-suffix overlay parents at SYSTEM copy entry

This gate continues the two flag-one `RES_00BA:1816` overlay parents from
their first length-plus-four push. It models their remaining argument
setup and far call to SYSTEM copy `0AF8:0B0B`, stopping before the helper
executes. The two destinations and source offsets differ, so both paths
are measured separately.

## Pinned sources and frame

The branch parent is the PM-approved, published parent-control head
`bdf43c7ad4690306f5fc89f3c81c656d81295224`. The ignored
`GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The two exact original spans are:

| Parent entry | OVR file window | Size | SHA-256 | Far-call return |
| --- | --- | ---: | --- | --- |
| `077:+11DD` | `[0x22217,0x2222C)` | 21 | `ee56de0069d02bf80a4b9ab6de60d740118a03a15226e0c5cad90c6289614a41` | `077:+11F2` |
| `077:+2AC8` | `[0x23B02,0x23B18)` | 22 | `59065f75702ed9b3e10159ee88b93766dcd60f597fe264f8754c6b621d3aec4c` | `077:+2ADE` |

For same-run provenance, the test also pins each complete parent return
through the far-call instruction: 33 bytes from OVR `0x2220B`
(SHA-256 `6891457405da398cb810c30bf810ad0036545e2691a5de51cb31ad7328560a34`)
and 35 bytes from `0x23AF5`
(SHA-256 `bce60c40e40aaa7fd51cc52052b9c27040a3e1ccc91b8ddb813f38d91a80712b`).
The `START.EXE` and expanded display body hashes, private suffix, printer,
glyph drawer, SYSTEM helpers, and eight overlay prefixes are rechecked by
the preceding fixtures. Original bytes and private text remain ignored.

Both paths push row and color words after `MOV AL` writes that preserve
AH from the preceding length-plus-four AX value. They then push a local
destination pointer, a CS-relative source pointer, and the far-call
return. At SYSTEM entry, `SS:SP` holds nine words in order: return IP,
return CS, source offset, source segment, destination offset,
destination segment, color, row, and length-plus-four. The first path
uses destination `BP-47` and source `CS:0EB2`; the second uses
destination `BP-12A` and source `CS:2994`. Both enter `0AF8:0B0B`.

## Differential result

Original bytes reach the second copied glyph's `0709:0694` return from
the two flag-one overlay sites. The C loop matches the second printer
return at `00BA:188C`; the C display/parent model matches the first
parent push. From that same state, the new C transition matches the
original far-call entry in **12/12** reached cases. Another **12/12**
cases set the node length after printer return to `0F` or `FC`. The `FC`
variants specifically verify the retained AH in the row and color words.
Each comparison covers the full 1 MiB RAM and 14 CPU fields with defined
flags, and the test checks the complete nine-word frame. C compilation
uses strict C11 warnings.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_00ba_suffix_parent_copy_entry.py
```

The length variants are synthetic late mutations, not natural saved-DOS
routes. This gate does not execute SYSTEM copy or the later parent calls;
the complete glyph/printer effects, hardware EGA planes, natural
saved-DOS flag-one reach, and strict full-C/game identity remain open.
Original media and captures stay under ignored `resources/`. Bounded
`deja "Poolrad overlay 077 11DD 2AC8 parent length plus four SYSTEM
B0B B98 copied suffix caller"` timed out without a result; no
prior-session assertion was reused. The
[parent-control gate](RESIDENT_00BA_SUFFIX_PARENT_CONTROL.md) and
[overlay caller-route map](RESIDENT_UNEQUAL_DISPLAY_1816.md) were
rechecked as dependencies.
