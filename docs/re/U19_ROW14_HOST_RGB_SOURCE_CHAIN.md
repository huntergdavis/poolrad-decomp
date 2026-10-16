# U19 row-14 source path from page return to host RGB

This is a source-only map for the fixed row-14 witness `A000:118F`, bit
`0x20`. It identifies a **candidate** emulator consumption path after a guest
EGA store. It does not show which host handler or page was selected in the
saved run, when the host displayed that byte, or which guest instruction
first changed it. No emulator was started for this audit.

A bounded `deja` lookup for the U19 row-14 host RGB consumer and page-helper
return timed out after 20 seconds. The merged
[page-return handoff](COLD_START_CODEWORD_ROW14_PAGE_RETURN_RGB_HANDOFF.md),
[page-selector audit](COLD_START_CODEWORD_ROW14_PAGE_SELECTOR.md),
[CPU source model](U19_ROW14_CPU_STORE_SOURCES.md), and
[retained display boundary](COLD_START_CODEWORD_ROW14_BOUNDARY.md) supplied
the prior guest addresses and four paired endpoints. The new audit pins the
original bytes and the ignored DOSBox-X source windows independently.

## Guest byte and page-return boundary

`START.EXE` and `GAME.OVR` have the previously published SHA-256 values
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The bounded source windows are:

| Original window | Size | SHA-256 | Boundary |
| --- | ---: | --- | --- |
| START image `[0x7101,0x7130)` | 47 | `26854c57a4d438a0e37a6c00ab8a7a6b9bff9567da09f681ee4247aeccd42b77` | RES_0709 active-page helper +0071..+009F |
| START image `[0x7130,0x7147)` | 23 | `6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d` | RES_0709 draw-page helper +00A0..+00B6 |
| START image `[0x7417,0x7501)` | 234 | `3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4` | RES_0709 +0387..+0470 drawer and four candidate stores |
| START image `[0x7A73,0x7BF6)` | 387 | `822b4aeac02e3396c177fbdb18187c6040862ec3595d7ffde16b8eb221565baf` | RES_0709 +09E3..+0B65 line input |
| U19 +02D8..+0301 | 42 | `518c6f63fa4c0ed6ff7dcf6b986fb4748ecf3e51a366f9005685d7f7e630a00c` | Printer call, reached return interval, input call |

The active helper's relocated runtime call at +0087 returns at +008C,
then +0099 writes DS:4A16 and +009D executes `RETF 2`. The draw helper
writes DS:4A18 at +00B0 and returns at +00B4 without a BIOS call. Both
use the low argument byte to calculate `A000 + (page << 9)` modulo 16 bits.
Their return IP/CS and any intervening BIOS effect require a same-run
helper frame. The bounded U19 suffix and line-input body contain no exact
direct far call to either helper; nested, indirect, and same-value helper
effects remain possible.

The printer at U19 +02D8 can enter the glyph drawer. Its candidate CPU
stores at resident +0412/+0438/+0445/+0451 load first/draw page segments
from DS:4A16/4A18. A CPU source event alone is not a changed EGA plane
byte. At the retained +02DD, +02E2, +02FD, and +0302 stops, active/draw
words are A000/A000 and page indexes 0/0; the row-14 mismatch is
477/477/477/0 source pixels. Host RGB changes by 0/0/6612 pixels across
those intervals. The four captures have no synchronized EGA planes or
page-helper instruction stop, so the display-change interval remains the
complete +02FD line-input call, not an identified writer or page flip.

## Conditional host consumption path

The ignored heavy DOSBox-X executable hashes to
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The private source checkout is commit
`5a9c7c53481c2e933948039c33256776653f4faf`. Its pinned files have
SHA-256 `f506017948503fa22ce40715205293c987473a938247cbd09ba409ffbb652c14`
(`vga_memory.cpp`),
`f9b798311cffb633754145cfc95b559f180a01b7e6f9f111ef47b7f56e79518d`
(`vga_draw.cpp`), and
`2c0d47c3c7d73fe092009a7d883316a6e40cf5620723f952c8278e7d93bac1d6`
(`render.cpp`). The audit checks ten exact line-window hashes, without
publishing source or frame data.

1. If the saved guest store uses the **unchained** handler, private
   `vga_memory.cpp:331-385` resolves the plane address, combines the CPU
   byte with latches/masks, and writes a four-plane word at line 383.
   Neither handler selection nor a value change at `0x118F` is measured.
2. For EGA display, `vga_draw.cpp:7740-7762` selects a two-bit, odd/even,
   or normal planar decoder according to machine and register state. The
   normal path reaches `EGA_Planar_Common_Line` at lines 1155-1190; the
   odd/even path uses lines 1192-1245, and the two-bit path uses lines
   1266-1333. Their shared palette translation begins at lines 1092-1122.
   The actual saved branch, scan address, and palette state are unmeasured.
3. `VGA_DrawEGASingleLine` at lines 3956-4024 passes a decoded line to
   `RENDER_DrawLine`. `render.cpp:320-362` can suppress an unchanged cached
   line; lines 400-440 choose the line handler, and lines 472-525 call
   `GFX_EndUpdate` when an update is produced. These source gates permit a
   guest plane change to precede a later host RGB observation. They do not
   establish that such a delay happened in this run.

The existing `pr_u19_row14_fixed_sources` C routine can emit candidate CPU
store operands, and `pr_slot_a_printer_page_apply` can project a supplied
page under its stated EGA assumptions. The existing Python
`slot_a_rgb_palette_audit.ega_rgb_frame` maps four supplied planes plus
captured AC/DAC state to RGB. A portable C row-14 RGB route would need a
measured pre-printer four-plane page, Graphics Controller and Sequencer
state, selected page/scan address, palette, and the immediate post-store
planes before comparison. None is present at the retained U19 endpoints.

Reproduce this read-only audit from the isolated checkout:

```sh
python3 tools/replay/cold_start_codeword_row14_host_rgb_source_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/codeword-row14-p5-001 \
  --host-source resources/dosbox-x-16a1-private-src \
  --heavy resources/dosbox-x-heavy-src/src/dosbox-x
```

Only addresses, hashes, counts, and candidate function names are public.
Original media, emulator binaries, RAM, EGA planes, and RGB remain ignored
under `resources/`. This adds no live writer, page/RGB timing, C parity,
strict translation, or full-frame credit.
