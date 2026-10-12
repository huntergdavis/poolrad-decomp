# RES_0464:02A5 route-0 pixel iterator

This is a **bounded C model** of the raw video-state-zero pixel loop
within `RES_0464:02A5`. It composes the reviewed Move, auxiliary,
primary-byte, and loop slices repeatedly across y, x, and layer
backedges. The caller supplies a pixel budget and report storage; the
model can stop before the next Move when that budget is exhausted. The
parent video-state dispatch, far-allocation aliasing, live LOAD MONSTER
route, and source-owner classification remain open. This adds no strict
full-C or game-identity credit.

## Provenance and binary bounds

Before implementation, `deja "Poolrad RES_0464 mode0 general iterator y x
layer backedges pixel budget"` found no matching session. This iterator
reuses the reviewed [prefix](RESIDENT_464_MODE0_PREFIX.md),
[primary groups](RESIDENT_464_MODE0_PRIMARY.md),
[second group](RESIDENT_464_MODE0_SECOND.md), and
[loop gate](RESIDENT_464_MODE0_LOOPS.md). It also preserves the earlier
[singleton](RESIDENT_464_MODE0_SINGLE.md) and
[two-y](RESIDENT_464_MODE0_TWO_Y.md) APIs.

The hash-locked read-only audit checks ignored `START.EXE` SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The route-0 body at loaded `0x04913..0x04C2F` is 796 bytes, SHA-256
`5b3cd8261e021741533286f4acbcd7c6bedb71d5fd13b3363698ac067f9ffc66`.
The gate jumps to the following reviewed backedges or common return:

| Gate outcome | Unit offset span, exclusive end | SHA-256 | Next |
| --- | --- | --- | --- |
| y differs from height end | `+0x0325..+0x0328` | `836fb65002fd742d43b2733338df4103d534419c2f3a08eed093b6cd5a46d0e0` | `+0x0328` Move |
| x differs from width end | `+0x0308..+0x0328` | `0e579be0b811f9fa79302e6125dec28616dca2204d8bdeb8bd6e21711ed85753` | `+0x0328` Move |
| layer differs from saved count | `+0x02EC..+0x0328` | `9ec211aeff74791df4c55e89e758139066bf9f278d09a6487835be3abaa4308e` | `+0x0328` Move |
| all match | `+0x08CE..+0x08D4` | `90f66eae99f51c421d8db68fa3d7a7629f728669d6c3fd06db0bb96e74651939` | far return |

The intervals overlap because each longer backedge contains the shorter
suffix. The existing slice audits verify instruction coverage and the
parent's entry count copy. The iterator audit verifies the hashes and
gate targets; it prints only addresses, counts, and hashes.

## Iteration and host stops

`pr_r464_mode0_iterate` uses caller-owned `pr_r464_mode0_pixel_report`
slots, one per reached prefix. The first iteration executes the entry
prefix, including count snapshot and dimension setup. Every later
iteration starts at `+0x0328` with the loop report's source offset,
primary data offset, auxiliary index, x, y, layer, saved count, and
dimension ends. It runs the same prefix and two primary groups, then
the reviewed y/x/layer gate. On the x backedge the gate rereads height;
on the layer backedge it rereads width then height. The iterator does
not replace those reads with precomputed shape arithmetic.

The explicit pixel budget is a host limit. Budget zero stops at
`+0x02D3` before any DOS-modeled access. After a completed pixel whose
loop continues, budget exhaustion stops at `+0x0328`, before the next
Move, retaining the gate's local updates. With a positive budget and
zero entry count, the prefix reaches the common return without a
pixel Move. `records_used` includes that entry probe, while
`completed_pixels` remains zero. A missing reached source, table,
destination, or auxiliary mapping stops in that slice with earlier
writes intact. Per-pixel reports hold each reached slice's stop site,
indices, calls, and writes. The summary records the current pixel,
completed count, stage, and continuation on budget/return paths.

Synthetic tests cover one y, x, and layer backedge; a four-pixel
y/x/y sequence; auxiliary index carry across a layer; zero count;
zero and one-pixel budgets; insufficient report storage; and a
second-pixel lookup stop after two OR writes. They use different
synthetic source nibbles and lookup rows on consecutive pixels, so
later writes require the carried source and data indices. Original
media and private lookup contents remain under ignored `resources/`.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_mode0_iterator_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```
