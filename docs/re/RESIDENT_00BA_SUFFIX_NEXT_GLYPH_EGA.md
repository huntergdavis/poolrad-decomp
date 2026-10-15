# RES_00BA copied-suffix second glyph at its first EGA change

This bounded gate starts at the second glyph drawer entry `RES_0709:01DF`,
reached from the copied suffix's first glyph return and printer loop. It
compares a C composition with original bytes through the first
value-changing A000 store, stopping at `0709:0415` immediately after the
`0709:0412` store. The C entry checks that the printer's one-based glyph
index and copied length are both two before invoking the existing
`pr_064f_first_ega_change` model.

## Original inputs and route

The branch parent is the PM-approved and published loop-control head
`c5352c53bfe2ec9f170155d620812a71c4a0d4c3`. The test rechecks the
ignored `START.EXE` (65,231 bytes; SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`)
and `GAME.OVR` (206,105 bytes; SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`).
It rechecks the expanded display body `[0x23B6,0x2432)` (124 bytes;
`645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175`),
private suffix source `[0x23B3,0x23B6)` (three bytes;
`4bd4cdfadef19a0803e203a5c05b37a6a3e79e76d25454fa559743b912409844`),
and SYSTEM B0B copy `[0xB781,0xB79B)` (26 bytes;
`4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f`).
The existing fixture also verifies the printer `[0x76B6,0x7737)`
(129 bytes; `b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed`),
glyph drawer `[0x726F,0x767B)` (1,036 bytes;
`a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840`),
SYSTEM upcase `[0xC4A0,0xC4B3)` (19 bytes;
`738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c`),
and all eight overlay prefix hashes. Only addresses, lengths, hashes, and
synthetic values are public; source bytes and text remain ignored.

For each case, original bytes run from one of the two flag-one overlay
sites through the first printer, SYSTEM suffix copy, second printer,
first glyph, and `0709:0694` loop control to `0709:01DF`. The C loop
transition independently reaches and matches that same drawer entry.
The test uses the live second drawer frame to locate its character's
font glyph, then supplies one synthetic `0x3F` font byte. Original bytes
and the C wrapper run from that identical state to `0709:0415`.

## Differential result

Two flag-one sites × three colors × two node-case inputs give **12/12**
second-glyph first-change comparisons. Each compares all 1 MiB RAM bytes
and 14 CPU fields with defined flags. The first changed flat-memory byte
is `A000:0143`; one, three, or five ordered A000 stores precede the stop
depending on color. The test checks the reported glyph index, color,
plane, store count, page address, and exact changed byte. C compilation
uses strict C11 warnings.

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn \
  python3 tests/recovered/check_resident_00ba_suffix_next_glyph_ega.py
```

The earlier first glyph and loop-control gates are dependencies, not new
full-printer claims. This model stops at the second glyph's first changed
store; its remaining drawer, later printer return/display effects,
hardware EGA planes, natural saved-DOS flag-one reach, and strict full-C
game identity remain open. Original media and captures stay under ignored
`resources/`. Bounded `deja "Poolrad RES_00BA copied suffix next glyph
0709 01DF 064F 0694 drawer frame"` timed out without a result; no
prior-session assertion was reused. The
[loop-control gate](RESIDENT_00BA_SUFFIX_GLYPH_CONTROL.md) and
[first-EGA model](RESIDENT_0709_064F_FIRST_EGA.md) were rechecked as
dependencies.
