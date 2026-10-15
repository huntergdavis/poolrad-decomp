# RES_00BA copied-suffix first glyph at its first EGA change

This bounded gate takes the nonzero `RES_00BA:1816` display suffix after
its first printer and SYSTEM copy, follows the second printer's original
bytes to `RES_0709:064F`, and compares a C composition through the
suffix's first glyph drawer and first value-changing A000 store. The C
transition stops at `0709:0415`, immediately after the reached
`0709:0412` store. It does not execute the rest of the glyph or printer.

## Dependency and original-byte pins

This branch is based on approved, published `cce4bd8fef23dbb8e30cfa4cf22b3c2d1ff86bd8`.
Fetched main `217e809448be096fcc48b2413714856f5b165284` does not yet
contain the display and first-EGA fixture/API dependencies. PM explicitly
approved this stacked base; no rebased integration or duplicate CMake
registration is claimed.

| Input or original span | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Expanded display body `[0x23B6,0x2432)` | 124 | `645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175` |
| Private suffix source `[0x23B3,0x23B6)` | 3 | `4bd4cdfadef19a0803e203a5c05b37a6a3e79e76d25454fa559743b912409844` |
| SYSTEM B0B copy `START[0xB781,0xB79B)` | 26 | `4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f` |
| Printer `[0x76B6,0x7737)` | 129 | `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed` |
| SYSTEM B25 printer copy `[0xB79B,0xB7BF)` | 36 | `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` |
| Glyph drawer `[0x726F,0x767B)` | 1,036 | `a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840` |
| SYSTEM upcase `[0xC4A0,0xC4B3)` | 19 | `738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c` |

The test also rechecks all eight pinned overlay prefixes through the
existing first-printer fixture. It starts at the two flag-one sites,
executes the first printer to `1858`, copies the private Pascal source,
enters the second printer, and reaches `064F` through original bytes.
The copied suffix length is two. At `064F`, the test supplies one
synthetic nonzero font byte for the first copied character; it never
publishes the private character or source bytes. The C wrapper composes
the existing `pr_064f_first_glyph_entry` and
`pr_064f_first_ega_change` transitions on that live frame.

## Measured boundary

The two flag-one overlay sites × three colors × upper/lowercase node
input yield **12/12** original-byte/C comparisons at `0709:0415`.
Every case compares the complete 1 MiB RAM image and 14 CPU fields,
using the existing comparer for architecturally defined flag bits.
The first changed flat-memory byte is always `A000:0142`, written by
`0709:0412`. The selected colors yield one, three, or five ordered A000
stores before that stop. Earlier clear-plane stores leave zero page
bytes unchanged; exactly one A000 byte differs between the `064F`
input and `0415` output. The test checks the C report's page address,
plane, color, store count, and glyph index against the original write
and live frame. C compilation uses strict C11 warnings.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_00ba_suffix_first_glyph.py
```

This is synthetic original-byte parity for the copied suffix's first
glyph boundary. No saved-DOS natural flag-one reach, full second-printer
translation, hardware EGA plane emulation, strict full-C/game identity,
or playable-game credit is claimed. Original media and captures stay
under ignored `resources/`. Bounded `deja "Poolrad RES_00BA second printer
copied suffix first glyph first EGA changed store"` timed out without a
result; no prior-session assertion was reused. The existing
[first-EGA gate](RESIDENT_0709_064F_FIRST_EGA.md) and
[second-printer call/return gate](RESIDENT_00BA_SECOND_PRINTER_RETURN.md)
were rechecked as dependencies.
