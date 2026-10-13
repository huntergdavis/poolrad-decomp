# Saved slot-A first changed EGA byte: eight indexed pixels

At the post-wall flush on the saved slot-A first-view route, active page
`A000:0A08` changes in three of four EGA planes. The portable C four-plane
byte decoder reconstructs the eight corresponding palette **indices**.
Against the same replay's private DOSBox-X view-return capture, all eight
indices match. This covers **8 of 64,000** mode-0D pixels; the other
63,992 remain unknown. The comparison is to palette indices after private
RGB normalization, not to a complete C framebuffer or a recovered palette
setter.

Before debugging, `deja "Poolrad slot A A000 0A08 four EGA planes indexed
pixels DOS oracle"` found no matching indexed session. Reused and checked:
[the offscreen-to-display transfer](SLOT_A_U40C_DISPLAY_TRANSFER.md)
(PR #319), [video mode/page setup](RESIDENT_VIDEO_INIT_4426.md), and
[earlier private first-view screen capture](SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md).
The standard 16-colour EGA RGB normalization is the one used by the merged
[partial static-view comparison](SLOT_A_STATIC_VIEW_FIRST_REGION.md).
No original screenshot, RAM image, VRAM sample, or pixel array is checked in.

## Native address chain

The ignored v1.3 inputs are pinned as `START.EXE` SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and `GAME.OVR` SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The reached `RES_0464:1B52..1E5B` flush has original file range
`[0x61CB,0x64D5)`, 778 bytes / 305 instructions, and SHA-256
`7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`.
PR #319 independently decoded its `+1CC6` staging store and `+1DA0`
runtime `Move` call.

| Route boundary | Address-only observation |
| --- | --- |
| Post-wall flush entry | `0C88:1B52`, `SS:SP=1D16:3E94`; stack return `2D14:05FB`, outer return `2D14:0DAE` |
| Source object / row | `DS:4A20` points to object `0x3DE48`; row 56 dirty, source data index 4,734 within extent 4,712..4,755 |
| Staging/display transfer | Row-56 plane store at `A200:0A08`; runtime `Move` spans `A200:0A03..0A0D` to `A000:0A03..0A0D` |
| View return | `3582:3387`, `SS:SP=1D16:3EF6`; active and draw page 0 at `A000` |

The original row arithmetic uses the 40-byte mode-0D scanline pitch.
Consequently byte offset `0x0A08 = 64*40 + 8` maps to screen row
`y=64` and eight bit positions `x=64..71` (bit 7 is leftmost).
This is also row `56+8` in the flush's object-to-staging mapping. For each
bit position, EGA read plane 0 contributes index bit 0, plane 1 bit 1,
plane 2 bit 2, and plane 3 bit 3. The new `pr_ega_planar_byte` applies
that mapping to arbitrary four-byte input, without choosing a palette.

## Hash-locked same-run comparison

A new ignored run under `resources/oracle-runs/slot-a-u40c-index-p5-002/`
stopped first at the documented post-wall frame and then at the view return.
The 640 KiB RAM dumps have SHA-256
`9cc1961687b4f52bd357cbb2eaf20882772cf35b334cbaf69d3dcdcdde70dff6`
and `6a67bd0fd7d3b027cffe96c7f1261eaf52196657a5a526ac7d613e837dcf4708`,
respectively. Before resuming from the flush, the debugger selected each
EGA graphics-controller read-map plane through ports `3CE/3CF` and dumped
one byte at `A000:0A08`. It restored read map 0 and the original register
selector before `RUN`. At view return, a private screenshot was captured
**before** selecting planes again. Its PNG SHA-256 is
`6f6b65475848ab63e35bc4fb55549c04a5957e8007cd6633fe1d4be377e61b7e`,
exactly the prior first-view return capture hash. Four more one-byte
read-map samples were then taken at the same return stop.

The pre-flush four-plane sample decodes eight indices; the return
sample changes all eight of them. The C probe reads the private
one-byte samples, returns eight index bytes, and the checker maps the eight
corresponding private 2×2 screenshot squares through the established EGA
palette. Result: **8/8 changed, 8/8 compared, zero index mismatches**.
The checker pins the private RAM, screenshot, sample, executable, and flush
span hashes, confirms frame/row/page/address state, and prints only aggregate
counts. No expected original pixel index is embedded in the C decoder or
synthetic test.

## Reproduction and boundary

```sh
cmake -S . -B resources/build-u40c-index-byte \\
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u40c-index-byte \\
  --target poolrad_ega_tests poolrad_ega_planar_byte_probe
ctest --test-dir resources/build-u40c-index-byte \\
  -R '^poolrad_ega_tests$' --output-on-failure
python3 tools/replay/slot_a_first_ega_indexed_byte_audit.py \\
  --exe resources/DOS/POOLRAD/START.EXE \\
  --ovr resources/DOS/POOLRAD/GAME.OVR \\
  --run resources/oracle-runs/slot-a-u40c-index-p5-002 \\
  --probe resources/build-u40c-index-byte/poolrad_ega_planar_byte_probe
```

The C routine is a portable EGA **format conversion**, not a translation
of the 778-byte flush, runtime `Move`, tile selection, or video BIOS.
The screenshot comparison supports only this one byte at this one saved
view-return state. It adds no strict full-C game routine, identity, or
full-frame credit. Palette changes on other routes and all other framebuffer
bytes remain unresolved.
