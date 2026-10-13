# Saved slot-A first-view EGA row 64: 320 indexed pixels

A fresh private replay captured all four 40-byte EGA planes for one active-page
scanline, `A000:0A00..0A27`, at the post-wall flush entry and at the saved
first-view return. The portable C `pr_ega_planar_byte` from PR #322 was
applied to all 40 byte positions. Its **320 indexed pixels match all 320**
palette-resolved pixels in the same-run DOSBox-X view capture. No pixel in
this row was unresolved or mismatched. This covers **320 of 64,000** mode-0D
pixels; **63,680** other frame pixels remain unexamined.

Before debugging, `deja "Poolrad slot A EGA y64 40-byte row four planes 320
indexed pixels"` found no direct indexed session. Reused and independently
checked: [the post-wall transfer](SLOT_A_U40C_DISPLAY_TRANSFER.md) (PR #319),
[the first byte's eight-pixel parity](SLOT_A_FIRST_EGA_INDEXED_BYTE.md)
(PR #322), [video mode/page setup](RESIDENT_VIDEO_INIT_4426.md), and the
[earlier view-return screenshot observation](SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md).
This branch stays local under the owner's no-GitHub-write instruction.

## Address and replay boundary

The ignored DOS v1.3 inputs retain `START.EXE` SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and `GAME.OVR` SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The original `RES_0464:1B52..1E5B` flush occupies file
`[0x61CB,0x64D5)`, 778 bytes / 305 instructions, SHA-256
`7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`.
Its `+1CC6` staging store and `+1DA0` runtime `Move` call were decoded
in PR #319; this note does not translate the complete flush or `Move`.

| Native stop | Measured boundary |
| --- | --- |
| Pre-flush `0C88:1B52` | `SS:SP=1D16:3E94`; stack return `2D14:05FB`, outer return `2D14:0DAE`; row 56 dirty |
| View return `3582:3387` | `SS:SP=1D16:3EF6`; row 56 dirty flag cleared; active/draw page 0 at `A000` |

The original row table gives `DS:[0x5050+2*(56+8)]=0x0A00`.
The mode-0D pitch is 40 bytes per 320-pixel scanline, so `0x0A00=64*40`
maps all 40 bytes to screen `y=64, x=0..319`. Within that row, the
prior first changed source index 4,734 maps to `A000:0A08`, and the
reached transfer covers `A000:0A03..0A0D`. Sampling the whole row does
not attribute its other changes to that one source byte or one tile call.

The new ignored replay directory is
`resources/oracle-runs/slot-a-u40c-row64-p5-001/`. Its pre-flush and
view-return 640 KiB RAM hashes are, respectively,
`129b45a994d3a1a9263b572b5b22e379022f61d1d82deec535bbbf8120261aff`
and `4027d48d95a8910ec631859fb2b445b7f939d55d0b312d334246e74e2117c20a`.
The private return screenshot SHA-256 is
`6f6b65475848ab63e35bc4fb55549c04a5957e8007cd6633fe1d4be377e61b7e`,
identical to the prior pinned first-view return screenshot. It was captured
before sampling return-state VRAM.

For each stop, the debugger selected read-map planes 0..3 with EGA
`3CE/3CF`, dumping `A000:0A00` for 40 bytes per plane. It restored read
map 0 and the original selector before resuming from the flush. The eight
private row files have these SHA-256 values in plane order:

| Stop | Plane 0 | Plane 1 | Plane 2 | Plane 3 |
| --- | --- | --- | --- | --- |
| Pre-flush | `2c34ce1df23b838c5abf2a7f6437cca3d3067ed509ff25f11df6b11b582b51eb` | `2c34ce1df23b838c5abf2a7f6437cca3d3067ed509ff25f11df6b11b582b51eb` | `d86fbf3f5491f417912ed0b27bff4e7abb3147ccf6bfc44083f4155dca318ed3` | `1a944a711bd2364040f83c60228a59f1341269d7c071f3e48e00e9ef64950836` |
| View return | `c931e183e5ae2064c1d20ad14e7b0e3bc181075d2a780e6c47cc31c3ac855563` | `8fcbdf290d674ef92d4edb6fd3e10f47c6c2a9026921d6f81881f34950c88d30` | `e730d203e4328142a9dd837381893d6649bb6846fab568a6124f0a399efe450c` | `ac8dd0dd945b65adea7bb0a314d9503133da608d42a8f8d13fbc3aac391718d1` |

## Portable C comparison and limits

`poolrad_ega_planar_row_probe` reads four exact-length 40-byte private
plane rows and invokes the generic `pr_ega_planar_byte` 40 times. It emits
320 index bytes only into the private checker's captured process output.
The checker pins all input hashes, verifies the route frame and row/page
arithmetic, and examines each corresponding 2×2 square in the private
640×400 DOSBox-X window. Every square is uniform and maps through the
established 16-colour EGA normalization. The measured aggregate is:

```text
A000:0A00..0A27 y=64 x=0..319 matched=320 mismatched=0 unknown=0 nonuniform=0 changed=138
```

The `changed=138` comparison is between the pre-flush and view-return
planar rows; it is not an attribution of 138 changes to item 13 or to one
instruction. The other 182 row pixels also match at view return. Neither
the C probe nor checker embeds an expected original pixel array. This
result establishes one row at one saved-state return, with the observed
palette normalization; it does not establish the remaining frame,
palette-setting call history, or full-C game-routine parity.

```sh
cmake -S . -B resources/build-u40c-row64 \\
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u40c-row64 \\
  --target poolrad_ega_tests poolrad_ega_planar_row_probe
ctest --test-dir resources/build-u40c-row64 \\
  -R '^poolrad_ega_tests$' --output-on-failure
python3 tools/replay/slot_a_ega_row64_audit.py \\
  --exe resources/DOS/POOLRAD/START.EXE \\
  --ovr resources/DOS/POOLRAD/GAME.OVR \\
  --run resources/oracle-runs/slot-a-u40c-row64-p5-001 \\
  --probe resources/build-u40c-row64/poolrad_ega_planar_row_probe
```

All original media, row-plane bytes, full RAM and screen captures remain
under ignored `resources/`. No DOS-only screenshot is published or shown.
