# U19 row-14 same-frame scanline and scaler capture requirements

This read-only contract specifies the missing **runtime fields** between a latched display start and a completed canonical RGB24 frame. It extends the [scanline source boundary](U19_ROW14_SCANLINE_CROP_SOURCE.md) and uses the [presentation event boundary](U19_ROW14_PRESENTATION_EVENT_BOUNDARY.md) for frame order. It neither supplies a collector nor changes the existing witness or packet APIs. The completed-backend packet and same-frame join belong to their separate owners.

Before this work, bounded `deja "Poolrad U19 row14 same-frame scanline scaler runtime field capture real_start EGA render selected page host frame contract"` timed out without usable recall. The merged scanline note supplied the VGA address, split, expander and scaler leads; the merged visual witness supplied run/frame/event identity but no source-to-host line map. This note uses ignored DOSBox-X revision `5a9c7c53481c2e933948039c33256776653f4faf`. Complete `src/hardware/vga_draw.cpp` is 346,548 bytes, SHA-256 `f9b798311cffb633754145cfc95b559f180a01b7e6f9f111ef47b7f56e79518d`; complete `src/gui/render.cpp` is 62,266 bytes, SHA-256 `2c0d47c3c7d73fe092009a7d883316a6e40cf5620723f952c8278e7d93bac1d6`.

## Source-locked observation points

The inclusive line windows and byte counts below are from that ignored revision. The windows identify **where a future passive collector would observe fields**, not events observed in the saved run.

| Source window | Bytes | SHA-256 | Observation point |
| --- | ---: | --- | --- |
| `vga_draw.cpp:6190–6220` | 825 | `7f576f607defb5f76e47c1a62b3cf5d6a4d6adf56681f0e21a08f49150bdf74b` | Vertical timer can end or abort an old update, zero `lines_done`, then return before a new frame if override is set or `RENDER_StartUpdate` fails. Preserve both results and their distinct frame identities. |
| `vga_draw.cpp:6259–6290` | 1151 | `888d758087aa016fa34a920fd9913b6393723e920c22478c128ad5f3e3e9b9a9` | New frame seeds address/line from `real_start` and EGA mode fields; sample the seed after the selected branch. |
| `vga_draw.cpp:3956–4038` | 3429 | `c3ce297f472f8fd67f0b4b41ce8e9f0927cdb2c5fa8052ee87502487c287c963` | For each EGA line event, sample pre/post `render_step`, `lines_done`, address, address line, panning, skipped/blanked/decoded path, `RENDER_DrawLine` call, and any split. |
| `render.cpp:397–445` | 1959 | `23875191a5488b3bbddae494a95aa0a668ea3025c9cb5c1f29e11bba9fabda5b` | A successful start resets `inLine`, `outLine`, cache and output pointers; early returns and failed `GFX_StartUpdate` cannot seed a presented frame. |
| `render.cpp:280–342` | 2075 | `74e0dd38e72e20bd19ecd1c491e2d807ce884fd4f1129c001779d970027495ff` | Different line-handler/cache paths consume `Scaler_Aspect` through different counters (`outLine` in the conditional countdown wait, `inLine` in the start handler); record the reached handler and its pre/post counters. |
| `render.cpp:530–548` | 523 | `aaaf93c9be141dbb7d44e040b072c1516cfc324f73275de2750fd2ff9f0364bd` | The aspect table may assign zero or multiple output lines to one source line. |
| `render.cpp:838–912` | 2747 | `3d24b5d4f1b0de6c51d5a05fb1fcb74fccc7ee55cca7870bc77c1e16f15f8888` | Scale flags, hardware path and `GFX_SetSize` determine output geometry; capture the selected path and resulting size rather than assuming 2×. |

## Minimum same-frame field set

These are requirements for a future **numeric-only ignored capture**, not a new public packet format. Each observation must carry one run token, monotonic host event index, and the completed-frame identity supplied by the separately owned backend packet. Guest CPU ordinals and host event indices remain different clocks.

1. At the host latch and new-frame start, retain the latch assignment event and `real_start` actually used by that frame. Record whether the preceding `RENDER_EndUpdate` completed, aborted or skipped an **old** frame, then whether the new `RENDER_StartUpdate` succeeded. Do not attach old-frame completion to the new latch.
2. At frame setup, retain machine, display mode, selected `VGA_DrawLine` expander identity, `draw.address`, `address_line`, `bytes_skip`, `hlines_skip`, `addr_shift`, `linear_mask`, `scan_len`, `address_add`, `address_line_total`, `maximum_scan_line`, `render_max`, `line_compare`, `split_line`, `vblank_skip`, panning controls, source width/height/bpp, and palette identity. The [scanline source boundary](U19_ROW14_SCANLINE_CROP_SOURCE.md) pins their conditional setup and mode branches.
3. For **every reached line event of that frame**, retain event order and line ordinal, pre/post address, address line, `render_step`, `lines_done`, panning and split state. Mark skiprender, the zero-filled disabled-attribute path, actual expander call and actual `RENDER_DrawLine` call separately. A skipped EGA callback is not a decoded source line. The zero-filled line's RGB meaning depends on the selected output format and palette; no RGB black is observed here. Retain the selected decoder's line input and effective panning so source y `112..119` is derived from events rather than presumed from text-row number.
4. For each line accepted by the renderer, retain the reached line handler, pre/post `inLine` and `outLine`, the `Scaler_Aspect` index/value consumed by that handler, cache-hit/miss and `GFX_StartUpdate` result, output pitch, and actual output-row interval or a typed inability to resolve it. Capture the selected software/hardware scaler, `GFX_SetSize` result, aspect path, and any debug-overlay geometry. Configuration alone cannot prove which host rows received a source line.
5. Link the last accepted line, renderer end, completed backend event and canonical RGB24 capture only through the separately owned same-frame packet. Require a continuous line-event sequence through the completed frame or mark coverage incomplete. Do not infer completion from `RENDER_EndUpdate` entry or from a screenshot digest.

The [expected row-14 crop](U19_ROW14_EXPECTED_RGB_CROP.md) has an ideal uniform-2× location x `16..591`, y `224..239`; this contract does **not** assert that those host coordinates were selected. If any selected frame, expander, source line, scaler destination, palette or backend conversion field is missing, report `BLOCKED_SAME_FRAME_SCANLINE_SCALER_MAPPING`. A future accepted mapping must still pass the separate page/plane and completed-backend RGB24 joins before any live crop or C parity claim. The retained `+0302` endpoint does not satisfy this same-frame requirement.

Reproduce this source lock without launching DOS:

```sh
git -C resources/dosbox-x-heavy-src rev-parse HEAD
sha256sum resources/dosbox-x-heavy-src/src/hardware/vga_draw.cpp \
  resources/dosbox-x-heavy-src/src/gui/render.cpp
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
root = Path('resources/dosbox-x-heavy-src/src')
windows = {'hardware/vga_draw.cpp': [(6190,6220),(6259,6290),(3956,4038)], 'gui/render.cpp': [(397,445),(280,342),(530,548),(838,912)]}
for name, spans in windows.items():
    lines = (root / name).read_bytes().splitlines(keepends=True)
    for first, last in spans:
        data = b''.join(lines[first-1:last])
        print(name, first, last, len(data), sha256(data).hexdigest())
PY
```

Original media, private source, traces, planes and frames remain ignored under `resources/`. This note contains only address/hash/numeric source conditions. It adds no native writer, page, RGB, C, or full-frame parity credit.
