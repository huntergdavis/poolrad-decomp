# Native cold-start title asset and comparison boundary

This note identifies the first distinct game-title frame in the DOS v1.3
no-argument cold-start capture. It records asset and RGB24 evidence, not a
translation of the startup calls or a claim that the entire startup sequence
has C/native parity. The original media, frames, and rendered outputs stay in
ignored `resources/` directories.

## Pinned private inputs

The read-only capture lives in the native replay worker's ignored
`resources/oracle-runs/native-cold-start-gap-004/`. The original asset is
`POOLRAD/TITLE.DAX`, SHA-256
`7944ab87afbe0002d39479d138c78c7b6b219bd6bbc3fb6844d361f5cd4dba2b`.
The stable reference is `stable-frames/0359-0433.png`, PNG-file SHA-256
`cea3d2c06f84c4c2dffaae493442649fe00bbe2bdb8e49324d46a883fda0bb99`.
It is a 640×400 RGB PNG. Its decoded row-major RGB24 bytes (768,000 bytes)
have SHA-256
`3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8`.
The private `frame-groups.json` identifies 75 consecutive slots, 359–433,
with that RGB24 hash. This is the game-title group identified separately
from the publisher splash and credits in
[NATIVE_COLD_START_SCREEN_GAPS.md](NATIVE_COLD_START_SCREEN_GAPS.md).

## Asset-to-frame observation

The existing [DAX reader](../../src/formats/dax.c) and
[EGA block contract](../formats/EGA.md) provide the archive and 17-byte
image-block layouts. Independent read-only parsing of the pinned `TITLE.DAX`
found two directory entries:

| Block ID | Stored bytes | Unpacked bytes | Image | Best per-index RGB agreement with native frame |
| ---: | ---: | ---: | --- | ---: |
| 1 | 9,258 | 32,017 | one 320×200 item | 29,024 / 64,000 pixels |
| 2 | 24,620 | 32,017 | one 320×200 item | 64,000 / 64,000 pixels |

All 64,000 native 2×2 cells have a single RGB color. Block 2's packed
high/low-nibble indices correspond to every 320×200 cell with zero color
conflicts. The observed nine-color mapping is the usual EGA RGB table for
indices `0, 1, 2, 5, 6, 8, 9, 13, 14`; there are no margins in the 640×400
reference. This establishes the sampled frame's exact pixel relation to
block 2. It does not by itself locate the instruction that selects block 2.

The earlier [unit-0x17 startup picture route](NATIVE_COLD_START_U17_PICTURE_ROUTE.md)
measured a resident call into the startup picture unit and a return after a
publisher-splash sample. It did not capture the title at an instruction stop,
so title-block selection remains a separate route-attribution question.

## Deterministic same-state comparison

Given a candidate 640×400 frame from the same title state, compare its raw
RGB24 bytes with the private native stable PNG. The public tool accepts a
candidate raw RGB24 file or an RGB-mode PNG. It validates the native
reference's decoded RGB24 hash, rejects wrong dimensions or byte lengths,
prints only hashes and difference counts, and returns 0 on an exact match,
1 on a pixel mismatch, or 2 on invalid input. Both input paths must resolve
inside an ignored `resources/` directory.

```sh
python3 tools/replay/cold_start_title_rgb24_compare.py \
  --reference resources/oracle-runs/native-cold-start-gap-004/stable-frames/0359-0433.png \
  --candidate resources/oracle-runs/title-candidate/frame.rgb \
  --candidate-format rgb24
```

The reference currently resides in the native replay worker's private clone;
substitute that read-only absolute path when running from another clone.
The candidate path above is illustrative and must be produced by a title
renderer at the same state. A zero-difference result establishes frame parity
for that state only. The comparator neither starts a native emulator nor
reads or writes tracked game media.

The primary worker generated a private 768,000-byte `review.rgb24` using the
title renderer merged via PR #572. Its SHA-256
is the pinned native RGB24 hash above. The comparison tool read that file
from the primary worker's ignored `resources/oracle-runs/cold-title-rgb24-pm/`
and the independently captured PNG from the native replay worker's ignored
run. It returned exit status 0 with `channel_bytes_different=0`,
`pixels_different=0`, and `bbox_xyxy_exclusive=none`. This is a measured
same-state RGB24 frame comparison across two private worker outputs.
