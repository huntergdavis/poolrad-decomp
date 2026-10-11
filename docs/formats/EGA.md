# DOS EGA image blocks

The portable C reader accepts one already unpacked DAX block with the verified
17-byte EGA image layout. It returns raw 4-bit palette indices. It does not
choose a palette, render images, or write pixels to disk.

## Input contract

| offset | bytes | field |
| ---: | ---: | --- |
| 0 | 2 | little-endian height in pixels, 1 through 200 |
| 2 | 2 | little-endian width in 8-pixel units, 1 through 40 |
| 4 | 2 | raw x field, preserved without assigning DOS meaning |
| 6 | 2 | raw y field, preserved without assigning DOS meaning |
| 8 | 1 | item count, 1 through 255 |
| 9 | 8 | opaque bytes, preserved exactly |
| 17 | remaining | item payloads |

Each item uses `height * width_units * 4` bytes. The block must have exactly
`17 + item_count * item_bytes` bytes and cannot exceed 65,535 bytes. Each row
uses `width_units * 4` bytes for `width_units * 8` pixels. The high nibble is
the left pixel, the low nibble the right pixel; both are palette indices 0 to
15. `pr_ega_pixel` reads one index, and `pr_ega_decode_item` writes one byte
per pixel in row-major order to caller-owned memory. The latter reports the
required buffer length through `written` on `PR_EGA_OUTPUT_SIZE`. The borrowed
block and output buffer must not overlap.

The field order, size formula, 320-by-200 bound, and nibble order come from
Gold Box Explorer's MIT-licensed
[EgaBlockSpecification.cs](https://github.com/bsimser/Gold-Box-Explorer/blob/eac30abaa6ee66aea6f5d65ebe6d676b10015a8f/src/Common/Plugins/Dax/EgaBlockSpecification.cs)
and [EgaBlock.cs](https://github.com/bsimser/Gold-Box-Explorer/blob/eac30abaa6ee66aea6f5d65ebe6d676b10015a8f/src/Common/Plugins/Dax/EgaBlock.cs),
revision `eac30abaa6ee66aea6f5d65ebe6d676b10015a8f`. The prior
[DOS audit](../FABLE_AUDIT.md) independently checked this 17-byte formula
against selected supplied files. The [MIT notice](GoldBoxExplorer-MIT.txt)
is retained. The upstream reader has a one-extra-item exception for a
different game; this DOS v1.3 reader requires the exact item count in byte 8.
An indexed older-session claim that 8X8D tile blocks have
70/31/45 items was **not reused**; the DOS audit corrects it to 70/70/70 for
blocks 1/3/5 on this build.

The DOS corpus check below independently fits the exact size formula for the
named art families. The raw x/y fields and bytes 9 through 16 are retained;
their gameplay or rendering meaning has not been established by the DOS
oracle. Combat and non-combat files may use different palettes, so this API
does not map palette indices to RGB.

## Supported and unsupported inputs

The exact layout passed for the selected `8X8D`, `BACPAC`, `BODY`, `CBODY`,
`CHEAD`, `COMSPR`, `CPIC`, `DUNGCOM`, `HEAD`, `ICON`, `RANDCOM`, `SQRPACI`,
`TITLE`, and `WILDCOM` archive families. The one `8X8D` font block (ID 201)
is a different format and is counted as unsupported. `FINAL` is mixed (five
of eight blocks fit), so the family is excluded from this supported inventory.
`PIC` and other art-like archives without an exact fit are unsupported by this
reader. No alternative layout is inferred for them.

The audit compares decoded palette indices with a separate direct application
of the upstream high/low-nibble rule for every supported item. It also checks
selected dimensions against the independent DOS audit and upstream reader:
`8X8D` 8×8, `BODY` 88×48, `HEAD` 88×40, and `TITLE` 320×200. No palette
indices, images, or hashes of decoded art are reported.

## Build, tests, and local read-only audit

From the repository root in Bash:

```sh
cmake -S . -B /tmp/poolrad-ega-build
cmake --build /tmp/poolrad-ega-build
ctest --test-dir /tmp/poolrad-ega-build --output-on-failure
art_files=(resources/DOS/POOLRAD/{8X8D*,BACPAC*,BODY*,CBODY*,CHEAD*,COMSPR*,CPIC*,DUNGCOM*,HEAD*,ICON*,RANDCOM*,SQRPACI*,TITLE*,WILDCOM*}.DAX)
/tmp/poolrad-ega-build/poolrad_ega_audit "${art_files[@]}"
```

On 2026-10-10, the ignored owner-supplied DOS v1.3 files returned:

```text
archives=42 blocks=596 image_blocks=595 unsupported_blocks=1 items=3922 decoded_pixels=1220928 dimension_checks=155 errors=0
```

The 155 selected dimension checks and all 1,220,928 decoded-pixel nibble
comparisons passed. Only aggregate counts and errors were printed. Target
executable SHA-256 checksums:

```text
START.EXE  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
GAME.OVR  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614
```
