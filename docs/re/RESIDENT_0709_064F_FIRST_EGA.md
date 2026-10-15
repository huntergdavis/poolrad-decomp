# RES_0709:064F first value-changing EGA store

This bounded model continues the merged
[post-copy control gate](RESIDENT_0709_0626_POSTCOPY_CONTROL.md) from the
valid `0709:064F` stop. The one-character path builds the `0709:01DF`
glyph-drawer frame, executes the original upcase call's effects, enters
mode-one drawing, and stops immediately after the first value-changing
`0709:0412` page store at `0709:0415`. It does not claim complete
`0709:01DF` or printer parity, strict full-C credit, or playable-game
credit. The separate `0709:0768` gate is untouched.

## Original-byte bounds

| Ignored input or span | Size | SHA-256 |
| --- | ---: | --- |
| `resources/DOS/POOLRAD/START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `resources/DOS/POOLRAD/GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Expanded glyph drawer `[0x726F,0x767B)` | 1,036 | `a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840` |
| SYSTEM upcase helper START `[0xC4A0,0xC4B3)` | 19 | `738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c` |

The original-byte harness also checks the pinned eight overlay prefixes,
`RES_00BA:1816`, printer `[0x76B6,0x7737)`, and SYSTEM string-copy
`[0xB79B,0xB7BF)` spans inherited from the earlier gates. These bytes
are executed through the `064F` entry in each case. The synthetic RAM
sets an EGA mode-one page at `A000`, a far font pointer, four plane masks,
and a one-character Pascal source. Uppercase and lowercase source cases
both exercise the original `0AF8:182A` upcase helper. Three node
attributes select the three colors that `1816` can emit: `0C`, `0E`,
and `0B`.

The first selected plane is respectively 2, 1, or 0. Earlier clear-plane
stores write zero to an already zero page byte, so the first changed byte
is written by `0709:0412`; its next IP is `0415`. The eight caller routes
reach page byte offsets `08D7`, `08C4`, `0141`, `1A41`, `0141`, `0141`,
`0141`, and `17C1`, in the caller order pinned by the test. The first
change follows five, three, or one mapped `A000` stores according to the
color. The C slice rejects a preceding clear-plane byte that is already
nonzero; such an input would change earlier and needs a separate stop.

## Differential and return evidence

`tests/recovered/check_resident_0709_064f_first_ega.py` compares all
14 CPU fields and every byte of 1 MiB flat RAM at glyph entry and after
the first changed store. AF is excluded only because the reached `XOR`
instructions leave it undefined; other meaningful flags are compared.
**48/48** eight-site × three-color × two-case original-byte/C comparisons
pass. Three representative original-byte executions continue from that
store to the `0709:0694` glyph return and the `00BA:1858` printer return
in the same run. Those later returns are native-route evidence only; the
new C transition stops at `0415`.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_0709_064f_first_ega.py
```

Unicorn provides flat mapped `A000` memory and does not emulate EGA
hardware planes. The measured mutation here is a changed flat-memory
byte under the original instruction stream and synthetic plane masks.
The independent saved-DOS
[clean-page write watch](SLOT_A_FIRST_CLEAN_PAGE_GLYPH_WRITE.md)
established `0709:0412` as a value-changing hardware EGA writer in a
different caller route; that prior address mapping was checked again
against the original bytes here. Before implementation, bounded
`deja "Poolrad RES_0709 064F glyph EGA mutation printer"` timed out;
no indexed session assertion was reused.

The next glyph-copy boundary and full glyph/printer C return are separate
follow-ons. Original game bytes and any capture files stay under ignored
`resources/`; the test keeps synthetic full RAM in memory, and this public
note records only addresses, sizes, hashes, and counts.
