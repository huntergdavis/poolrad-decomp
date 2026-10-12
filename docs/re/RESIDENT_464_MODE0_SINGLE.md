# RES_0464:02A5 video-state-0 one-pixel composition

This is a **bounded C composition** of four reviewed slices of one
original resident routine, `RES_0464:02A5`. It supports only a
destination header with count=1, width=1, and height=1, after the
parent's nonzero-object and raw video-state-zero gates. On that shape,
the path reaches the common `RETF 0x0E`. It does not model repeated
pixels, layers, other video states, or live LOAD MONSTER execution.
It adds no original routine identity and no strict full-C credit.

## Provenance and route coverage

Before implementation, deja query `Poolrad RES_0464:02A5 route 0
single pixel composition prefix primary loop` found no matching
prior session. The reviewed
[auxiliary prefix](RESIDENT_464_MODE0_PREFIX.md),
[first primary byte](RESIDENT_464_MODE0_PRIMARY.md),
[second primary byte](RESIDENT_464_MODE0_SECOND.md), and
[loop continuation](RESIDENT_464_MODE0_LOOPS.md) supplied the separate
instruction and C contracts. The composition audit reruns their
hash-locked checks against ignored `START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Slice | Unit interval, exclusive end | Bytes |
| --- | --- | ---: |
| Auxiliary prefix | `+0x02D3..+0x03CE` | 251 |
| First primary byte | `+0x03CE..+0x04CB` | 253 |
| Second primary byte | `+0x04CB..+0x05CB` | 256 |
| Loop gate | `+0x05CB..+0x05EF` | 36 |

The intervals are contiguous and cover all 796 route-0 bytes at loaded
`0x04913..0x04C2F`, SHA-256
`5b3cd8261e021741533286f4acbcd7c6bedb71d5fd13b3363698ac067f9ffc66`.
The gate then jumps to the separately checked common return at
`+0x08CE..+0x08D4`. The parent dispatch prologue remains outside this
composition; the raw video-state-zero gate is a caller assumption.

## Supported path and effects

`pr_r464_mode0_single_pixel` checks the singleton shape before
executing any DOS-modeled effect. This is an explicit **host scope
gate**, not a DOS branch. Unsupported shapes return without changing
source, object, or auxiliary storage. For the supported shape, it
calls each reviewed slice in instruction order:

1. One four-byte runtime Move is modeled into the local copy. A
   nonzero auxiliary pointer invokes the merged bitmask helper, then
   conditionally ORs synthetic private-pattern bytes into two
   auxiliary bytes.
2. Four nibble-helper calls and private lookup reads OR the first
   primary byte. An optional auxiliary complement AND follows.
3. Four more nibble-helper calls and lookup reads OR the second
   primary byte, followed by its optional complement AND. Only then
   do data offset and mutable source frame offset advance to two and
   initial-offset-plus-four.
4. The y, x, and layer locals all match their saved bounds for this
   shape. The gate reaches `+0x08CE`, whose epilogue returns far.

The report retains each component's stop site, call/write counts,
source/data offsets, and partial effects. A missing reached source
span, private-table entry, primary destination, or auxiliary mapping
stops at that component without rolling back prior writes. Tests
exercise both auxiliary-presence branches and late stops after
partial OR work. All tables and input bytes in tests are synthetic.
The private DS lookup values and original media remain ignored.

Far allocations are represented by distinct, nonaliasing host arrays
from offset zero. Segment-offset wrapping, aliasing, and native
runtime state are outside the model. Repeated-pixel and layer
composition remains necessary before a full procedure claim.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_mode0_single_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```
