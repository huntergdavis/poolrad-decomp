# Cold-start codeword selected-record visual effects

This note closes the **selected-record glyph and string source** for one
existing U19 pre-input native stop. Three glyph cells and fourteen dynamic
string cells match that same-run screenshot over all 1,088 source pixels.
The choice, record contents, and screen pixels remain private. This is not a
complete codeword screen, input, or validation model.

The bounded `deja` query recorded in the
[box source note](COLD_START_CODEWORD_BOX_SOURCE.md) timed out. The merged
[codeword fixed-text source](COLD_START_CODEWORD_SOURCE.md),
[font contract](FONT_8X8D_201.md), and
[native key-read branch](NATIVE_COLD_START_U19_KEY_READ_BRANCH.md) supplied
the U19 call sites and existing RAM/screenshot stop. Original spans and the
same-run cell comparison were checked independently.

## Byte and frame provenance

Ignored `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x19` begins at file `0x09AC`; its original glyph-control span
`+0196..+026E` is 216 bytes, SHA-256
`1ffa11c58e23ad6286311b1a2a9b207dd84ab57ebc6589a4b7853ad0716f3a2f`.
The append-and-print span `+028B..+02C0` is 53 bytes, SHA-256
`f6413cdf89d8d57b2151230c05b40230e9a1b73719942b7289e6eaf6686f8cd1`.

At U19 `+0111`, runtime `0AF8:11C0` receives word `13`; `+0116` saves its
returned low byte. The ignored `+02FD` pre-input RAM capture, 655,360 bytes
and SHA-256
`b4dc553331795b63fb6e8fb6678df59b0a72799f4c386ccdc21ce81b1ec892a3`,
has selected index **9** at the U19 local `[BP-0x2A]` (`SS=1D16`,
`BP=3EF8`). The indexed source starts at `DS:(index*0x15)` with
`DS=14A0`; its 21-byte record hashes
`f71d278190a55a274e7c24197bea1c12fc8c1e2981cd92910988e236d9cf84a1`.
This stop identifies one selected entry; it does not establish the RNG's
distribution or the choice on another run.

| U19 far-call site | Source field | Cell | Effect |
| --- | --- | --- | --- |
| `+01BB` | indexed byte `+09`, then WORD `+0x40` | column 8, row 6 | raw glyph, color 12 |
| `+01E5` | indexed byte `+0A`, then WORD `+0x40` | column 9, row 6 | raw glyph, color 12 |
| `+022C` | indexed byte `+0B`, then WORD `+0x40` | column 8, row 9 | raw glyph, color 12 |
| `+0269` | indexed byte `+0C`, unless sentinel `0xFF` | column 9, row 9 | skipped in this capture |

Each call targets `0709:01DF` with count one and upcase flag zero. The
captured font pointer `DS:5E20` resolves to 1,416 bytes, SHA-256
`fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1`.
The three drawn 8×8 cells match **192/192** source pixels of the paired
native screenshot, including clear bits. The fourth call is skipped by the
explicit compare at `+023D..+0244`.

At `+029F`, runtime `0AF8:0B0B` copies the fixed Pascal prefix at U19
`+00A3`; its length is five and its length-plus-content SHA-256 is
`0bdfeb1cc77982d53834973b517f0913830f333b406255761ec7a5f9f3efb299`.
U19 `+02B6` calls runtime append `0AF8:0B98` with the selected table field
at indexed `+0D`. That field has length nine and hashes
`173d2874ff166e11bd32941d15151f92b59598b4c53f8f00fdfa25c2ec85ca24`.
Its final **two** content bytes extend past the 21-byte indexed stride into
contiguous DS memory; a model confined to one 21-byte record would truncate
this observed Pascal source. The captured local at `[BP-0x12B]` equals the
exact 5+9 byte Pascal append and hashes
`59865d6b1da4499f2d8430aeb20cf71c0898ebe1733e9f4f418cf128318c7990`.
U19 `+02BB` prints its fourteen cells at column 1, row 13, color 12 via
`0709:0626`. They match **896/896** source pixels in the paired screenshot.

The screenshot is 640×400, canonical RGB24 SHA-256
`5044229ac07b046aaf144113a90e8fec64edcfef47f978d843319991603dabcb`.
The comparison checks all four doubled host pixels for each source pixel:
**4,352/4,352 host pixels** match across the seventeen selected-record
cells. Five fixed rows also match the same-run stop. The final fixed row at
row 14 differs in **477 source pixels across 19 cells** after the
intervening U19 `+02DD` call to `0899:0104`; without a pre/post pair, the
writer of those differences remains unresolved. The separate stable-frame
fixed-row comparison in the merged [source note](COLD_START_CODEWORD_SOURCE.md)
has a different capture boundary and is not silently substituted here.

Run the focused read-only audit with the hash-pinned DOS inputs and the
ignored paired capture:

```sh
python3 tools/replay/cold_start_codeword_effect_source_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --ram resources/oracle-runs/codeword-source-p5-001/at-u19-02fd.ram \
  --png resources/oracle-runs/codeword-source-p5-001/at-u19-02fd.png
```

The script prints only addresses and counts and writes no output media.
The source bytes, selected record, font, RAM, and images remain under ignored
`resources/`. No heavy native run or full-C/count claim is made.
