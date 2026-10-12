# RES_0464:02A5 video-state-0 first-pixel loop continuation

This is a **bounded C model** of the y/x/layer gate after the first
route-0 pixel, composed after the reviewed
[auxiliary prefix](RESIDENT_464_MODE0_PREFIX.md),
[first primary byte](RESIDENT_464_MODE0_PRIMARY.md), and
[second primary byte](RESIDENT_464_MODE0_SECOND.md). A next-pixel result
stops before its four-byte Move at `+0x0328`; the terminal result
reaches the common epilogue at `+0x08CE`. Repeated pixel bodies are not
modeled here. This is not a full parent translation, live LOAD MONSTER
proof, or strict/game-identity count promotion.

## Prior work and provenance

Before implementation, deja query `0464:02A5 +05CB loop` found no
matching prior instruction trace. The three reviewed route-0 slices
provided the first-pixel state and the `+0x05CB` lead. The branch and
backedge instructions below were independently decoded from ignored
`START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Span | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| Gate `+0x05CB..+0x05EF` | `0x04C0B..0x04C2F` | `0x04C44..0x04C68` | 36 | `a186da72ac0767e72fbefab8c2e06829f5d63edb2a6bf35a8beb0de988d9d4d5` |
| Layer backedge `+0x02EC..+0x0328` | `0x0492C..0x04968` | `0x04965..0x049A1` | 60 | `9ec211aeff74791df4c55e89e758139066bf9f278d09a6487835be3abaa4308e` |
| X backedge `+0x0308..+0x0328` | `0x04948..0x04968` | `0x04981..0x049A1` | 32 | `0e579be0b811f9fa79302e6125dec28616dca2204d8bdeb8bd6e21711ed85753` |
| Y backedge `+0x0325..+0x0328` | `0x04965..0x04968` | `0x0499E..0x049A1` | 3 | `836fb65002fd742d43b2733338df4103d534419c2f3a08eed093b6cd5a46d0e0` |
| Common return `+0x08CE..+0x08D4` | `0x04F0E..0x04F14` | `0x04F47..0x04F4D` | 6 | `90f66eae99f51c421d8db68fa3d7a7629f728669d6c3fd06db0bb96e74651939` |

These backedge intervals overlap intentionally. The read-only audit
checks their hashes, contiguous source mapping, complete instruction
bounds, seven gate branch targets, local increments, dimension reloads,
entry count snapshot, and `RETF 0x0E`. It prints only offsets, counts,
and hashes.

## DOS branch order and state

At route entry `+0x02D6/+0x02DA`, object header byte `+8` is copied
to local `[BP-0x29]`; the layer gate compares against that saved byte,
not a fresh header read. The prefix report now retains this count.
Width and height bounds are decremented WORDs, so a zero dimension
becomes `0xFFFF`. The second primary-byte model has already advanced
the data offset to two and source frame offset by four before this gate.

| Gate and taken path | Original order | Model result |
| --- | --- | --- |
| `+0x05CB..+0x05D3`, y differs from saved height end | Jump to `+0x0325`, increment y WORD, then reach `+0x0328`. No object header read. | `Y` branch, next pixel with y incremented. |
| `+0x05D6..+0x05DE`, y ended and x differs from saved width end | Jump to `+0x0308`, increment x WORD, reread object height WORD `+2`, decrement it, reset y to zero, reach `+0x0328`. | `X` branch with a reached height reload. |
| `+0x05E1..+0x05E9`, y and x ended but layer differs from saved count | Jump to `+0x02EC`, increment layer byte, reread/decrement object width WORD `+0`, reset x, reread/decrement height WORD `+2`, reset y, reach `+0x0328`. | `LAYER` branch with ordered width/height reloads. |
| `+0x05EC`, all three match | Jump to `+0x08CE`; epilogue restores SP/BP and returns far, consuming `0x0E` frame bytes. | `RETURN` branch, with no next pixel. |

The comparisons use equality; the model preserves that condition even
for unusual local values. Header bounds are checked only on the
backedges that reread them. A missing height mapping after x increment,
or missing width/height mapping after layer increment, stops with the
earlier local updates visible in the report. Y and return paths need
no object mapping. The host does not emulate source Move or repeated
pixel writes after `+0x0328`, nor far-pointer aliasing or segment-wrap
effects. The live video-state value remains unmeasured.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_mode0_loops_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

Tests use synthetic headers and source bytes; original media remain
under ignored resources.
