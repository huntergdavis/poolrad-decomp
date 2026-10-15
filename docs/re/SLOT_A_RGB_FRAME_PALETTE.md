# Saved slot-A first-view RGB palette and complete frame

At the stable saved slot-A first-view prompt, a directly queried native
attribute-controller (AC) map and 6-bit DAC convert the already recovered
four-plane C page into a **complete 320×200 RGB frame**. The 64,000 C RGB
pixels equal the native screenshot at every location. Each of the native
screen's 256,000 display pixels also equals the corresponding 2× C pixel.
This is parity for one measured static frame, without a new claim about which
instruction programmed the palette or about another gameplay state.

Before debugging, `deja "Poolrad saved slot A EGA attribute controller DAC
palette register native screenshot RGB 64000 first view"` found no matching
session. This audit reuses the merged
[video-init state](RESIDENT_VIDEO_INIT_4426.md),
[postflush C page](SLOT_A_POSTFLUSH_PAGE_CHANGE.md), and
[static-view screen normalization](SLOT_A_STATIC_VIEW_FIRST_REGION.md), and
the merged [first-view indexed-frame gate](SLOT_A_SCREEN_PALETTE_JOIN.md)
(PR #457). This follow-on reads the hardware palette values that the
indexed-frame gate left unresolved. The meaning of the DOSBox-X
debugger's AC and DAC reports is documented in its
[video debug reference](https://github.com/joncampbell123/dosbox-x/blob/59915c1/README.video-debug-overlay.md)
and [debugger implementation](https://github.com/joncampbell123/dosbox-x/blob/59915c1/src/debug/debug.cpp).

## Measured native display state

The ignored run is
`resources/oracle-runs/slot-a-rgb-palette-p5-001`. It uses the owner save
and heavy DOSBox-X whose SHA-256 values are
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`
and `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The stabilized and paused screenshots are byte-identical (PNG SHA-256
`2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134`).
The finalized native debugger transcript hashes to
`578a577f4891dcb29d8e28eb84b12229727bba9a3989b8520baba3c0f83b5326`.
This is one paused native state, before any read-map selector changes made to
dump the four planes.

| Direct query | Saved first-view value |
| --- | --- |
| `VGA MODE` | planar `M_EGA`, display and real start `0`, scanline stride `0x20`, pixel pan and byte skip `0` |
| `VGA AC` | mode control `0x09`, enabled planes `0x0F`, color select `0`, AC entries `00..07,10..17` |
| `VGA DAC` | 6-bit channels, pixel mask `0xFF` |
| `VGA DACPAL` | all 256 DAC triples queried; the AC-selected entries resolve to the standard 16 EGA colors |

The debugger had also reported a startup text-mode AC map before the game
loaded. The audit selects the **last** AC map at the saved view and rejects a
change to its expected structure. Four same-run native `A000` read-map dumps,
8,000 bytes each, have SHA-256 values:

| Plane | SHA-256 |
| --- | --- |
| 0 | `68638729c3ac5a483f4fb023cf839dd4fea8432b51ec8e96d96d1fb3a8b35da1` |
| 1 | `975cb7c40471ebb947734deae142c526bf37e0a756f3cb49c409492d432add35` |
| 2 | `8c29a55dbd09cc924a3295263172b6336dd2d1c69caf3524f800af4929920404` |
| 3 | `d1f05b349fd8db17bbf4739a7869448c4e8252f093db9c9592b597dfbb83c351` |

The C page reproduced by the merged prompt-row model hashes to
`09926b914ef24ca2c77eca35c49fd526b54fc27cafbc2d985eefecbcdc61500a`.
It equals the concatenated planes of this new native run, byte for byte.
Thus the C page and hardware registers used for RGB conversion describe the
same observed frame even though the C page was generated from the earlier
hash-locked prompt-row run.

## Whole-frame parity

The audit decodes each pixel from the four plane bits, most significant bit
first, maps its index through the observed AC entry and DAC pixel mask, then
expands each 6-bit channel with `(value << 2) | (value >> 4)`. It compares the
result to the centered screenshot crop `(320,312)..(960,712)`, requiring exact
2×2 replication. The image contains 11 used EGA indices.

```text
same-state C/native A000 page: 32000/32000 bytes equal
same-state C/native RGB frame: 64000/64000 pixels equal; 11 used indices
C RGB SHA-256 ed2cc92edf4cf7ac11f9700f53fc16355259cdcaa748970d1159f6a5c6dd5d4e
native 2x RGB crop SHA-256 8b302d814bda9f7b1f0af9305655ecae7c91491f2e6880d7ba1d689708e17f8b
```

## Reproduction and limits

Regenerate the C page with the command in
[the postflush note](SLOT_A_POSTFLUSH_PAGE_CHANGE.md), then run:

```sh
python3 -m unittest -v tests/replay/test_slot_a_rgb_palette.py
python3 tools/replay/slot_a_rgb_palette_audit.py \
  --run resources/oracle-runs/slot-a-rgb-palette-p5-001 \
  --save resources/DOS/POOLRAD/SAVGAMA.DAT \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --c-page resources/oracle-runs/slot-a-postflush-plane-change-p5-001/postflush-prompt-probe-output-private.bin
```

The audit hash-locks private inputs, parses the directly observed palette and
mode, checks all four same-run native planes against the C page, and checks
the full RGB crop. The synthetic tests cover AC remapping, DAC masking,
six-bit conversion, plane bit order, 2× expansion, and malformed inputs.
Original media, RAM, planes, RGB output, and screenshots remain in ignored
`resources/`; only addresses, state values, counts, and hashes are public.

Two disjoint next gates are (1) attribute/DAC *write attribution* on the
first-view program and BIOS path, since this snapshot proves the values but
not their writer or timing, and (2) a separate post-input view frame using
the same RGB audit contract once its native display state and C page are
available. Neither is counted as a translated game routine here.
