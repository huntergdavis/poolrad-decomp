# Saved slot-A first view: one reached printer call and its clean-page delta

## Scope and prior work

This note isolates one complete native string-printer invocation between the
first `RES_0464:1B52` flush and the first-view return. It measures its
contribution to the 1,404 clean `A000` plane bytes that changed in that
interval. The other 1,036 changed clean plane bytes remain a separate display
gate. This is page-output parity for the reached, in-bounds EGA call, not a
whole-view model or a new full-routine translation.

Before debugging, `deja "Poolrad slot A RES_0709 string printer glyph drawer
invocation clean plane bytes 1404"` found no matching indexed session. This
work reuses the [first-flush indexed-page audit](SLOT_A_U40C_DIRTY_INDEXED_VIEW.md),
the [first clean glyph byte watch](SLOT_A_FIRST_CLEAN_PAGE_GLYPH_WRITE.md)
(PR #325), the [merged font and printer recovery](FONT_8X8D_201.md), and
the [native debugger watch protocol](SLOT_A_ONE_UP_NATIVE_WRITE_WATCH.md).
No merged font source or another lane's files were changed.

## Hash-locked native call

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The expanded `RES_0709:0626` string-printer span
`[0x076B6,0x07737)` is 129 bytes, SHA-256
`b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`;
the live loaded span `0F2D:0626..06A6` hashes to
`8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164`.
The five-byte resident caller at `RES_00BA:080F`, expanded image
`[0x013AF,0x013B4)`, hashes to
`04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23`.
At runtime `08DE:080F`, its relocated far call targets
`0F2D:0626`; the immediate return is `08DE:0814`.

At the callsite, `SS:SP=1D16:3EAA`; the pushed words are far source
`449A:0008`, color 15, row 4, column 17. The private Pascal source is
14 characters, SHA-256 over length plus bytes
`6c2e68d9723b65bacedc6effe4898a0fb1a437e902f4a08b5d0dc030221f6feb`.
The live 1,416-byte font buffer `DS:[5E20]` hashes to
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
`DS:[4A1E]=1` selects EGA; both output page segment words
`DS:[4A16]` and `DS:[4A18]` are `A000`. The call passes the
printer's row/column guards and the glyph drawer's stricter row guard:
14 cells occupy columns 17..30, byte rows 32..39.

At `08DE:0814`, the retained printer frame at `SS:BP=1D16:3EA4`
has copied length 14, loop index 14, loop bound 14, and its private
column byte advanced from 17 to 31. The return frame is `08DE:0814`.
These observed loop values, caller ABI and immediate return establish
one complete in-bounds invocation. They do not establish how the
preceding game routine chose the text.

## Same-run page measurements

The fresh hash-verified private run is
`resources/oracle-runs/slot-a-printer-call-p5-002/`. Each page state is
four 3,520-byte EGA plane windows `A000:[0x03C0,0x1180)`,
concatenated in plane order (14,080 bytes). The immediately returned first
flush entered at `0C88:1B52` with `SS:SP=1D16:3E94` and returned to
`2D14:05FB`. The next stops were `RES_00BA:080F`,
`RES_00BA:0814`, and first-view return `3582:3387`.

| Stop | 14,080-byte four-plane SHA-256 | Full RAM SHA-256 |
| --- | --- | --- |
| immediate first-flush return | `bd12654fb632ba139bc7ca8aaefff1c8253133d3751c39454896a2559ddcf044` | `239b2d982b76ebf1c23b2e540c26d2b4f4d7d0d5b9637518c95c26f747f5d351` |
| before printer far call | `bd12654fb632ba139bc7ca8aaefff1c8253133d3751c39454896a2559ddcf044` | `fa135bbaa31c53f482f7eb693dc95f1b1a051573adb9b3a12106b0a9269fb88e` |
| immediately after printer return | `144f74d0e9111b44175354a75f6cf0ed462be479c70fbb23fb5452c60dd842c0` | `371f123afd0fe1a54dffd38797254c0b2a9fed93dc9806f40c12d5a83af5301a` |
| first-view return | `d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b` | `9b7ea330309dafff495b2a91b2060c1580f18b3be1d09a7ec3a3422243575ebc` |

The first-flush dirty page spans are byte columns 3..13 in each of the
88 measured rows. The printer's cells are in the clean columns 17..30.
The private audit pins each of the 16 individual plane hashes and checks:

| Interval in this one run | Value-changing plane bytes | Where |
| --- | ---: | --- |
| post-flush to pre-printer | 0 | entire 14,080-byte rectangle |
| pre-printer to post-printer | 368 | all within the 14 cells, all clean |
| post-printer to first-view return | 1,036 | all outside those 14 cells, all clean |
| post-flush to first-view return | 1,404 | 368 from this call + 1,036 later |

The call covers 14 cells × 8 scanlines × 4 planes = 448 plane-byte
targets. The native before/after values differ at 368 of them; 80 already
equal the resulting value. All 368 changed bytes persist at first-view
return. The snapshot delta does not count same-value machine stores.

## Bounded C page parity

`pr_slot_a_printer_page_apply` composes the merged
`pr_font201_print_ega` EGA path with one opaque, cell-aligned page
projection. Its supported path requires the input text length at most
255, row at most 24, all cells inside columns 0..39, and all target
page bytes in the supplied span. It refuses other bounds without changing
the supplied planes. It models the `A000` page bytes only; the original
printer's temporary stack copy and other video modes are outside this
adapter.

Synthetic tests cover two cells with upcase and selective color planes,
preservation outside the cells, a repeated call with zero value changes,
empty text, row/column and page-span guards, and invalid inputs. The
private probe feeds the live hash-pinned font, source length/content and
pre-call page into the C composition. It predicts **14,080/14,080**
native post-call plane bytes exactly, including the 368 changed bytes
and unchanged neighbors. Its aggregate output SHA-256 equals the native
post-call page hash above. This parity is for this reached call and these
inputs; it is not strict full-C credit for the resident printer.

```sh
cmake -S . -B resources/build-printer-page \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-printer-page --target \
  poolrad_slot_a_printer_page_tests poolrad_slot_a_printer_page_probe
ctest --test-dir resources/build-printer-page \
  -R '^poolrad_slot_a_printer_page_tests$' --output-on-failure
python3 tools/replay/slot_a_printer_page_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --run resources/oracle-runs/slot-a-printer-call-p5-002 \
  --probe resources/build-printer-page/poolrad_slot_a_printer_page_probe
```

All original executables, text, font bytes, RAM, EGA pages and any
screenshots remain under ignored `resources/`.
