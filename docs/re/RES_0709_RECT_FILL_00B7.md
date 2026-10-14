# RES_0709:00B7 bounded planar rectangle fill on slot-A PRINTCLEAR

**Classification:** bounded C model of the complete 186-byte, 51-instruction
`RES_0709:+00B7..+0171` routine. It covers inclusive rectangles inside the
40×25 display and EGA page spans supplied by the caller. The enclosing
`U1C_PRINT_0A09` → `RES_0709:+0768` → `+0DD3` route remains outside this
translation. No full-C routine count or end-to-end one-Up C parity is claimed.

Before decoding, `deja "Poolrad RES_0709 00B7 rectangle fill page 0 page 1"`
timed out after 20 seconds without a reusable result. The earlier
[row-five write watch](SLOT_A_SECOND_CYCLE_IMMEDIATE_RETURNS.md) and
[second-cycle page composition](SLOT_A_SECOND_CYCLE_PERSISTENT_PAGE.md)
provided the original-body hash and a value-changing native page pair. The
[U1C PRINT note](U1C_PRINT_0A09.md) supplied the overlay handler lead. This
work independently decoded the complete rectangle body, its local callees,
and the new one-Up frames. SYSTEM `+0B25` belongs to a separate worker; it is
not duplicated here.

## Original and loaded bounds

The DOS v1.3 START.EXE SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
GAME.OVR is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The owner slot-A save is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X binary is
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
Original game media and all captures remain ignored under `resources/`.

| Span | Bytes | SHA-256 |
| --- | ---: | --- |
| Expanded START `[0x7147,0x7201)`, `RES_0709:+00B7..+0171` | 186 | `f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5` |
| Loaded `0F2D:00B7..0171` | 186 | `02ba954ed5bc0baa194fa7e74eeaceba27b09386107ac51df7df27a09c6aaa5f` |
| Local EGA reset `+0000..+0071`, original and loaded | 113 | `5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5` |
| Local page selector `+00A0..+00B7`, original and loaded | 23 | `6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d` |
| Loaded SYSTEM FillChar `131C:16A8..16BC` | 20 | `f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7` |

The rectangle ends in `RETF 0x0C` at `+016E`; `+0171` begins other data. Its
frame reads low bytes `(color,page,bottom,right,top,left)` at
`SS:[BP+06..+10]`. The full words' upper bytes are not stable inputs.
It computes `top*320+left`, `(bottom-top+1)*8` scanlines, and inclusive
`right-left+1` bytes per scanline. The local page selector sets `DS:4A18` to
`A000+(page<<9)` paragraphs. The reset callee selects all four planes,
sets the graphics-controller bit mask to `FF`, and the rectangle sets EGA
write mode 2. SYSTEM FillChar writes each row at a 40-byte stride. The
rectangle returns write mode to zero and restores `DS:4A18` from the
unchanged `DS:4A1A` page byte.

## Reached one-Up frames and page result

The fresh hash-pinned slot-A run starts at visible `15,4 N 10:50` and sends
exactly one Up. At the reached U1C PRINTCLEAR call `3582:0A91`, the original
GAME.OVR five-byte call hashes
`505ad54ab9e5390450c330a9decab8bab5ee9f07d44dc475f2414fa8afc6f9ea`;
the loaded call hashes
`9d5cce1a3faf38548317d311f935802eab5da2a7485708f6afe43e48d1da1b07`.
The resident `+07E1` call toward `+0DD3` hashes
`00a6fe385950a2f1ebfdcf643d224c90385570cd16696eefa51e9b11ccfa81e5`
in original and loaded images. With `DS:4A1E=1`, the decoded `+0DD3` mode-1
branch has direct near calls at `+0E8E` and `+0EA8`, hashes
`22a02fb9bd68d98657e6e8c09df5ba1fe39c89511380a1c7cf4737f70b8960e6`
and `808e7dd970fcb77fa9495f410ea96a7eb011b8bd207fe400a5868792e958bf8b`
in both images. The private RAM captures independently assert the two
callee return frames at `SS:SP=1D16:3C7C`:

| Entry | Return `CS:IP` | Low-byte arguments | Page changed bytes |
| --- | --- | --- | ---: |
| First `0F2D:00B7` | `0F2D:0E91` | `(0,0,22,38,17,1)` | 0 |
| Second `0F2D:00B7` | `0F2D:0EAB` | `(0,1,22,38,17,1)` | 0 |

At the pre-call, both direct entries/returns, and the enclosing
`3582:0A96` return, page 0 is **32,000 bytes**, SHA-256
`f963d2bcdb1d0f0cf1a0fdc32e285e5299993dd7f9a7b474cfbbd6978b4cbbe1`;
page 1 is **32,000 bytes**, SHA-256
`7af052afad72a3c8938f567d2bd57fe9914947d2a70e519fd7bace6e4d75fc90`.
`DS:4A1A=0` and `DS:4A18=A000` at all six capture points. These are page
value results; they do not count same-value hardware writes or prove every
intermediate CPU branch from a persisted instruction trace.

The independent earlier row-five call has low-byte arguments
`(0,0,5,38,5,17)`. Its native page changes **154** plane bytes from SHA-256
`3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738`
to `570ad46e976182f1227b76bdb367fa0303bd789c42bfa095770a74cb5f7aa52f`.
This gives a value-changing check of the C fill in addition to the two
one-Up same-value checks.

To recapture the one-Up windows, prepare a fresh ignored run with
`tools/oracle/prepare_run.sh slot-a-u1c-print-rect-p9-096`, launch the pinned
heavy DOSBox-X with `DEBUGBOX START`, advance the title, select load and
slot A, and verify visible `15,4 N 10:50`. Use Alt+Pause to enter its
debugger, set `BP 3582:0A91`, resume, and send one Up. At that stop capture
640 KiB RAM with `MEMDUMPBIN 0000 0000 A0000`; for each EGA plane 0..3,
select graphics-controller register 4 using `OUTP 3CE 04`, set read maps 0
through 3 with `OUTP 3CF 00` through `OUTP 3CF 03`, then
`MEMDUMPBIN A000 0000 3F40`. Copy each overwritten
`MEMDUMP.BIN` into the ignored run before selecting the next plane. Restore
read map 0 and graphics-controller selector 5. Replace the breakpoint with
`BP 0F2D:00B7`, `BP 0F2D:0E91`, `BP 0F2D:0EAB`, and `BP 3582:0A96`; capture
the same RAM and plane windows at the two entries, two direct returns, and
outer return. The private audit filenames use `preprint`, `rect1-entry`,
`rect1-return`, `rect2-entry`, `rect2-return`, and `postprint`. This recipe
observes stop points; its public audit asserts stored frames and bytes rather
than inventing a persisted instruction transcript.

## Portable boundary and verification

`pr_res0709_rect_fill_00b7` uses four caller-owned planar buffers, the
`DS:4A1A` active page, and EGA register state. It implements the complete
decoded rectangle body for coordinates `0..39` by `0..24` and a fully
supplied page. Unsupported coordinates or insufficient buffers return
`BOUNDARY` before mutation. The C model's target count is derived from its
loop and is **not** a measured native hardware-write count. The synthetic
test exercises page isolation, color bit selection, inclusive bounds,
same-value results, final register state, and rejected inputs.

The private audit hash-locks the images, saved input, six RAM checkpoints,
all six two-page EGA windows, direct call instructions, and the row-five
before/after page. It compares the C output to the native result for all
three calls: **161,536/161,536 plane-window bytes**. The one-Up calls each
match 64,768/64,768 captured bytes with zero value changes; the row-five
call matches 32,000/32,000 with 154 value changes. The source text, save,
RAM, and page bytes are never printed or tracked.
The focused synthetic test passed **1/1**, the complete strict CMake/CTest
suite passed **197/197** on the rebased main, and the public repository
hygiene checks passed.

```sh
cmake -S . -B resources/build-u1c-print-rect-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u1c-print-rect-p9 --target \
  poolrad_resident_0709_rect_fill_00b7_tests \
  poolrad_resident_0709_rect_fill_probe
ctest --test-dir resources/build-u1c-print-rect-p9 \
  -R '^poolrad_resident_0709_rect_fill_00b7_tests$' --output-on-failure
ctest --test-dir resources/build-u1c-print-rect-p9 --output-on-failure -j4
python3 tools/replay/slot_a_u1c_print_rect_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --save resources/DOS/POOLRAD/SAVGAMA.DAT \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-u1c-print-rect-p9-096 \
  --row5-run resources/oracle-runs/slot-a-ui-second-cycle-p9-023 \
  --probe resources/build-u1c-print-rect-p9/poolrad_resident_0709_rect_fill_probe
```

This model does not resolve `RES_0709:+0768`, `+0DD3`, the separate
SYSTEM `+0B25` adapter, or U1C's full print path. It makes the reached
planar rectangle effect available to a later composition without wiring
an empty print hook.
