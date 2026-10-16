# U19 row-14 scanline-to-crop source boundary

This read-only source audit follows the [page-to-host bridge](U19_ROW14_PAGE_TO_HOST_SOURCE_BRIDGE.md) after `real_start` is latched. It bounds the EGA scan address, planar line expansion, and renderer geometry that must be measured before assigning the [fixed expected row-14 crop](U19_ROW14_EXPECTED_RGB_CROP.md) to a saved-run host image. It does not report a reached page, selected expander, host crop, or RGB match.

Before this audit, bounded `deja "Poolrad U19 row14 DOSBox-X RGB24 channel order SDL surface conversion real_start scan address 640x400 source audit"` timed out without usable recall. The merged page bridge supplied the retrace latch and initial-address lead; the expected-crop adapter supplied only the fixed 2× source geometry. The ignored DOSBox-X source is revision `5a9c7c53481c2e933948039c33256776653f4faf`. Complete `src/hardware/vga_draw.cpp` is 346,548 bytes, SHA-256 `f9b798311cffb633754145cfc95b559f180a01b7e6f9f111ef47b7f56e79518d`; complete `src/gui/render.cpp` is 62,266 bytes, SHA-256 `2c0d47c3c7d73fe092009a7d883316a6e40cf5620723f952c8278e7d93bac1d6`.

## Conditional source chain

The table hashes exact inclusive line windows, preserving line endings. Each consequence is conditional on the runtime branch named in the source; none is a saved U19 observation.

| Source window | Bytes | SHA-256 | Bounded consequence |
| --- | ---: | --- | --- |
| `vga_draw.cpp:6259–6290` | 1151 | `888d758087aa016fa34a920fd9913b6393723e920c22478c128ad5f3e3e9b9a9` | Frame start sets `address_line=hlines_skip` and `address=real_start`; M_EGA/M_LIN4 then adds `bytes_skip`, multiplies by four and by `1 << addr_shift`, and selects a linear mask. |
| `vga_draw.cpp:6843–6860` | 412 | `f387133599c774ae58ee1f8f4c1435b4d9d24a23ee25b46db4fa6e3b6702d1a9` | M_EGA/M_LIN4 address increment per completed address-line group is `scan_len * (8 << addr_shift)`. |
| `vga_draw.cpp:7550–7589` | 1363 | `7972420f8c0d5e7e971f5908c52cdf55cf8d7c7cbec098016fa06d383e4aafcd` | `maximum_scan_line`, doublescan compatibility and architecture set `address_line_total`, possible height halving and `render_max`. |
| `vga_draw.cpp:6414–6434` | 1113 | `e46532a16e0d5e658fd7fda6a79f6e755c190d3348f808b7452717907a000bfd` | Top vblank skip can advance address and address line; a split at or before the first line can reset them before drawing. |
| `vga_draw.cpp:3517–3600` | 3663 | `ba6915cdc5646065fdcc69f2fe1fe45cbea07348aedb423ba688e54573785faa` | Line compare, render maximum and vblank skip determine split line; split resets address and address line, with panning rules dependent on mode control and machine. |
| `vga_draw.cpp:3956–4038` | 3429 | `c3ce297f472f8fd67f0b4b41ce8e9f0927cdb2c5fa8052ee87502487c287c963` | The EGA line callback may skip rendering, draw disabled-attribute black, or call the selected `VGA_DrawLine(address,address_line)` and `RENDER_DrawLine`; it then advances line/address, tests split, and can update panning. Raw-image capture has a distinct call. |
| `vga_draw.cpp:1155–1190` | 1931 | `c9beac86dc4a176a14e50095a14237c3c7a31d48ab629de7ae75d9661a292d92` | The common EGA planar decoder can halve its input line for the doublescan bit, applies line mask, address mask/shift and panning, and returns an offset into its decoded line except for raw snapshots. |
| `vga_draw.cpp:7738–7778` | 1570 | `c135178ee4d3d5d69b455e8e0f69dc4587e3ce824d1c5b461b1c05e656cb6bc2` | M_EGA/M_LIN4 selects 2-bpp, odd/even planar, or regular planar expander from machine and mode bits; EGA and VGA architecture select different output bpp. |
| `vga_draw.cpp:8135–8165` | 1051 | `7fc22dd147115df3d48a06720e082c203a8d7c97a1475c254e33bcaf6346750c` | A changed width, height, ratio, bpp or frame rate invokes `RENDER_SetSize`, unless display override suppresses it. |
| `render.cpp:1054–1168` | 3364 | `435f5e03e676cc74904de97774a1645f6d1fe62556361f01376a497a5035ed0e` | Renderer input geometry chooses horizontal/vertical doubling from dimensions and ratio; debug overlay can enlarge the source geometry. |
| `render.cpp:530–548` | 523 | `aaaf93c9be141dbb7d44e040b072c1516cfc324f73275de2750fd2ff9f0364bd` | Aspect mapping can assign a variable number of output lines per source line. |
| `render.cpp:838–901` | 2408 | `74513021f8063c813fba8b0b128e68ecb63075e2ca731fae0dc877c13674f020` | Scaler and output flags choose different horizontal/vertical scale paths and aspect tables. |

The ideal fixed source crop spans source x `8..295`, y `112..119` (36 cells × eight pixels). If the selected source is exactly 320×200 with no offset, split, panning, skip or overlay, and both output axes use a uniform 2× map, the host crop is x `16..591`, y `224..239`, 576×16 RGB24 bytes. The [expected crop](U19_ROW14_EXPECTED_RGB_CROP.md) hashes those bytes as `e6f630a6abf12a030e92387a2966f7ab15abff63f8a8a57b9b8988e516dff980`. Those conditions were not measured together at the saved U19 stop, so the coordinates and digest remain an expected-image contract, not a host crop observation.

## Runtime join still required

`BLOCKED_SAME_RUN_SCANLINE_AND_SCALER_STATE`: bind the selected guest page-helper frame to the subsequent host latch and completed frame; retain its `real_start`, mode/machine, `bytes_skip`, `addr_shift`, `scan_len`, `hlines_skip`, `maximum_scan_line`, `render_max`, split/line compare, vblank skip, panning, selected expander, palette, `RENDER_SetSize` inputs, scaler/aspect path, and backend RGB24 conversion. Then record which source and host lines covered row 14. The retained `+0302` endpoint cannot replace this same-frame join. Until then, do not infer the selected page, first plane change, live writer, RGB parity, C parity, or full-frame parity from the fixed crop.

Reproduce the source lock without an emulator launch:

```sh
git -C resources/dosbox-x-heavy-src rev-parse HEAD
sha256sum resources/dosbox-x-heavy-src/src/hardware/vga_draw.cpp \
  resources/dosbox-x-heavy-src/src/gui/render.cpp
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
root = Path('resources/dosbox-x-heavy-src/src')
windows = {'hardware/vga_draw.cpp': [(6259,6290),(6843,6860),(7550,7589),(6414,6434),(3517,3600),(3956,4038),(1155,1190),(7738,7778),(8135,8165)], 'gui/render.cpp': [(1054,1168),(530,548),(838,901)]}
for name, spans in windows.items():
    lines = (root / name).read_bytes().splitlines(keepends=True)
    for first, last in spans:
        data = b''.join(lines[first-1:last])
        print(name, first, last, len(data), sha256(data).hexdigest())
PY
```

Original media, source copies, emulator traces and host captures remain ignored under `resources/`. This note adds only address/hash/numeric source evidence and no native or C parity credit.
