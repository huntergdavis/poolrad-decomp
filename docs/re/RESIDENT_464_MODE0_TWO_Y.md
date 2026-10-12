# RES_0464:02A5 route 0, two y pixels

This is a **bounded C composition** for count=1, width=1, height=2 after
the parent's nonzero-object and raw video-state-zero gates. It executes
the first pixel, the y backedge, exactly one more pixel, and the common
`RETF 0x0E` return. It is one path of the original routine, with no new
routine identity, strict full-C count, or live LOAD MONSTER claim. The
one-pixel API keeps its separate count=width=height=1 scope.

## Evidence and prior work

Before implementation, `deja "Poolrad RES_0464 mode0 count 1 width 1
height 2 y backedge second pixel composition"` found no matching
session. This work reuses the locally reviewed
[prefix](RESIDENT_464_MODE0_PREFIX.md),
[first primary byte](RESIDENT_464_MODE0_PRIMARY.md),
[second primary byte](RESIDENT_464_MODE0_SECOND.md),
[loop gate](RESIDENT_464_MODE0_LOOPS.md), and
[one-pixel composition](RESIDENT_464_MODE0_SINGLE.md). Their hash-locked
audits identify the contiguous 796-byte route-0 body at loaded image
`0x04913..0x04C2F`, SHA-256
`5b3cd8261e021741533286f4acbcd7c6bedb71d5fd13b3363698ac067f9ffc66`.
The source is ignored `START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The y backedge at `+0x0325..+0x0328` is three bytes, SHA-256
`836fb65002fd742d43b2733338df4103d534419c2f3a08eed093b6cd5a46d0e0`.
The separate common return at `+0x08CE..+0x08D4` has SHA-256
`90f66eae99f51c421d8db68fa3d7a7629f728669d6c3fd06db0bb96e74651939`.
The read-only two-y audit verifies these identities and the gate targets.

## Reached path and state

The shape check is a host scope gate before effects. For the supported
shape, the first pixel calls the reviewed Move, auxiliary bitmask and OR,
both primary nibble/lookup groups, and y gate in order. At `+0x05CB`,
local y=0 differs from saved height end=1; `+0x05D3` jumps to `+0x0325`,
increments y to 1, and reaches `+0x0328` without rereading the header.

| State | First pixel completion | Second pixel entry | Second pixel completion |
| --- | ---: | ---: | ---: |
| Source WORD `[BP+0x0E]` | initial +4 | initial +4 | initial +8 |
| Primary data WORD `[BP-0x1C]` | 2 | 2 | 4 |
| Auxiliary WORD `[BP-0x1E]`, pointer present | 2 | 2 | 4 |
| Auxiliary WORD, pointer absent | 0 | 0 | 0 |
| x, y, layer | 0, 0, 1 | 0, 1, 1 | 0, 1, 1 |

The resumed prefix starts at `+0x0328`: it copies four new source bytes,
reuses the bitmask and optional auxiliary OR path, and advances the
auxiliary index twice when present, even for a zero mask. It does not
repeat entry setup. The first and second primary groups use destination
offsets `object+0x17+2` and `object+0x17+3`, with the same nibble helper
and private lookup semantics as the first pixel. Their optional ANDs
use the carried auxiliary index minus two and minus one. Only after the
second group succeeds do source and data indices advance. At the final
gate, y=1 equals height end=1, x=0 equals width end=0, and layer=1
equals saved count=1; `+0x05EC` reaches the common return.

Each resumed operation uses the reviewed slice implementation; the
composition does not assign expected pixel bytes. Synthetic tests use
different first and second source nibbles and lookup rows. They cover
both auxiliary-presence paths, shape rejection, the second Move bound,
a partial second-row lookup, an absent second destination, and a late
auxiliary read. Reached host failures leave first-row and partial
second-row writes intact. Host arrays model separate, nonaliasing far
allocations; segment wrapping and live runtime state remain outside
this bounded path. Wider x/layer loops and other video states remain
untranslated.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_mode0_two_y_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

All test bytes are synthetic; the original executable stays under
ignored `resources/`.
