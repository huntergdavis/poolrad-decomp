# Saved slot-A first-flush dirty pixels in the native first view

The accepted bounded video-state-1 transfer model for `RES_0464:1B52`
now composes the post-flush `A000` page from **pre-flush RAM, staging planes
and page planes**, then applies the existing portable
`pr_ega_planar_byte` decoder to the complete 88-row display rectangle.
The first flush's **7,744 dirty indexed pixels** match all 7,744
corresponding pixels in a same-run native first-view screenshot. The
other **20,416 pixels** in those rows retain their pre-flush values as
model inputs. No original pixel array or screenshot is embedded in the
probe or public tests.

This is an indexed-output gate for the reached first flush. It is not a
full translation of `RES_0464:1B52`, a whole-frame parity claim, or
strict full-C credit. The branch remains local under the owner's
no-GitHub-write hold.

Before debugging, `deja "Poolrad saved slot A post flush A000 dirty rows
indexed pixels screenshot pairing row 16 103"` and a shorter row-64
query found no indexed session. This work reuses and independently
checks the [accepted all-dirty transfer model](SLOT_A_U40C_ALL_DIRTY_TRANSFER_MODEL.md)
(local base `dedabe6`), the [row-64 screenshot pairing and palette check](SLOT_A_FIRST_EGA_ROW64.md),
and the [first indexed byte](SLOT_A_FIRST_EGA_INDEXED_BYTE.md).

## Exact boundary and same-run pairing

The ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The flush file span `[0x61CB,0x64D5)`, loaded
`RES_0464:1B52..1E5B`, is 778 bytes / 305 aligned instructions,
SHA-256
`7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c`.
Its runtime Move dependency `0AF8:1685..16A8`, file
`[0xC2FB,0xC31E)`, is 35 bytes / 16 instructions, SHA-256
`1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972`.

The new ignored run is
`resources/oracle-runs/slot-a-u40c-dirty-indexed-p5-001/`.
At first flush entry `0C88:1B52`, `DS=14A0`, `SS:SP=1D16:3E94`,
and the return is `2D14:05FB`. The entry RAM SHA-256 is
`226acf93f958f039e488692b28ec608eb8ef5d454f11ffa66779f1cbe9cce3a4`.
The immediate return RAM SHA-256 is
`144e32c8c8842a0e7f897f4c2e9bdc193fd0ec8a66f623227c77c01384246cbd`.
Live Graphics Controller register reads 0/1/3/4/5/8 were
`00/00/00/00/00/FF`; the normalized private record is SHA-256
`dbcd522682e32bc73ef8cb22d93140b9811f93aa6920d0d5feddafcb85585d73`.

Before first-view return, the debugger encountered another
`0C88:1B52` entry at `SS:SP=1D16:3EBE`; its sampled dirty flags
again mark rows 16..103 (private SHA-256
`148adce341b0db5cf45c92f8e8c0f1fda11884eafe67b14c079b6a7b4fc6df28`).
The paired screenshot was taken at first-view return
`3582:3387`, `SS:SP=1D16:3EF6`, **before** selecting any EGA
read-map plane there. Its private SHA-256 is
`6f6b65475848ab63e35bc4fb55549c04a5957e8007cd6633fe1d4be377e61b7e`;
same-run return RAM SHA-256 is
`9277ad6edbd01972ad237bfb9672465c238998627ff1304f840f437eb528b34a`.
The hash-locked checker pins all 20 private 3,520-byte plane captures:
four `A200` and four `A000` before and immediately after the first
flush, plus four `A000` at view return. The immediate-return window
image lagged the page and is not used as screen evidence.

## Bounded display geometry and comparison

The first-flush dirty rows are exactly 16..103, each with 11 page-byte
offsets from `0x03C3 + 40*(r-16)` through the next 10 bytes. In the
mode-0D 40-byte scanline, that is display `y=r+8`, `x=24..111`.
The probe holds the full `A000:[0x03C0,0x1180)` rectangle as input:
88 rows, `y=24..111`, `x=0..319`, or 28,160 indexed pixels. It
updates only the reached dirty spans and decodes every page byte.
The untouched clean portions account for 20,416 pixels; their
pre-flush plane bytes are supplied to the C model, not inferred from
the screenshot.

| Check | Exact result |
| --- | ---: |
| First-flush object bytes to native `A200` dirty plane bytes | 3,872/3,872 |
| Native `A200` to `A000` dirty plane bytes | 3,872/3,872 |
| C-composed post-flush page to immediate native page, full rectangle | 28,160/28,160 indexed pixels |
| C-composed first-flush dirty pixels to same-run first-view screen | 7,744/7,744 |
| First-flush clean pixels retained at immediate return | 20,416/20,416 |
| Native view-return page to same-run screenshot, full rectangle | 28,160/28,160 |
| Unknown palette or nonuniform doubled pixels | 0/28,160 |

Of the dirty pixels, 7,667 changed between pre-flush and immediate
return and 77 already held their resulting index. None changed again
before the first-view return. Of the clean pixels, zero changed during
the first flush, then **1,862** changed before first-view return,
corresponding to 1,404 `A000` plane bytes outside the first-flush
dirty spans. The first-flush C page therefore matches 18,554/20,416
clean pixels in the later screenshot; the other 1,862 are observed
intervening native page changes, not unknown colors or a claim about
the first flush. The instruction or call that writes those later
clean bytes has not been isolated here.

The probe and checker publish no indexed pixel values. The checker
uses the established 16-colour EGA normalization only to compare the
private 640×400 screenshot's uniform 2×2 pixel squares against the
model output. The screenshot is never a predictor. Synthetic tests
cover dirty bytes in two rows and different retained clean neighbors.
The remaining 35,840 screen pixels are outside this rectangle; other
video modes, later page changes and full-procedure behavior remain
unproved.

```sh
cmake -S . -B resources/build-u40c-dirty-indexed \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u40c-dirty-indexed \
  --target poolrad_slot_a_ega_dirty_indexed_probe \
           poolrad_resident_464_dirty_indexed_tests \
           poolrad_resident_464_dirty_transfer_tests poolrad_ega_tests
ctest --test-dir resources/build-u40c-dirty-indexed \
  -R '^(poolrad_resident_464_dirty_indexed_tests|poolrad_resident_464_dirty_transfer_tests|poolrad_ega_tests)$' \
  --output-on-failure
python3 tools/replay/slot_a_ega_dirty_indexed_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --run resources/oracle-runs/slot-a-u40c-dirty-indexed-p5-001 \
  --probe resources/build-u40c-dirty-indexed/poolrad_slot_a_ega_dirty_indexed_probe
```

All original executables, RAM, plane captures, indexed outputs and
screenshots remain under ignored `resources/`.
