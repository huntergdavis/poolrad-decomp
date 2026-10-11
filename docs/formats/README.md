# Portable DAX and GEO readers

This directory documents the bounded C format layer. The code builds without
game data and uses synthetic records in tests.

## Build and test

From the repository root:

```sh
cmake -S . -B /tmp/poolrad-formats-build
cmake --build /tmp/poolrad-formats-build
ctest --test-dir /tmp/poolrad-formats-build --output-on-failure
```

To audit the owner's ignored DOS v1.3 archives locally, without extracting or
writing decoded blocks:

```sh
/tmp/poolrad-formats-build/poolrad_format_audit resources/DOS/POOLRAD/*.DAX
```

The command checks every directory and fully decodes every block. It checks
that every block in `GEO[digits].DAX` parses as a 1,026-byte record and reports
only counts or errors. Supply paths explicitly; it does not walk directories.

## Input contract

`pr_dax_open` borrows a buffer of 11 to 1,048,576 bytes. Keep it alive and
unchanged until all reads finish. The first little-endian word is the length
of a directory of one to 256 nine-byte entries. Each entry contains an ID,
a little-endian 32-bit offset relative to the end of the directory, a
little-endian unpacked length, and a little-endian stored length. IDs must be
unique and stored ranges must fit in the input. An unpacked length of zero
means the stored bytes are copied directly. Otherwise the block uses signed
control RLE: controls 0 to 127 copy the following `control + 1` bytes, while
controls 128 to 255 repeat the following byte `256 - control` times. A valid
stream consumes all stored bytes and produces exactly the declared length.

`pr_dax_read` takes a directory **position**, not an ID. The caller owns an
output buffer of at least `entry.unpacked_size` bytes; passing a smaller buffer
returns `PR_DAX_OUTPUT_SIZE` and sets `written` to the required size. Output
must not overlap the borrowed input. `pr_dax_open` checks directory and block
ranges; `pr_dax_read` checks the RLE stream. The API does not infer archive
type from a block ID.

`pr_geo_parse` copies exactly 1,026 decoded bytes. The first two bytes remain
opaque. The next four 256-byte planes are indexed by `y * 16 + x`: north/east
wall nibbles, south/west wall nibbles, event bytes, and four two-bit
directional door fields. Accessors return the raw values. They do not merge
opposite edges or decide whether movement is possible; neighboring cells can
disagree. Coordinates must be from 0 through 15.

## Evidence and limits

The directory and RLE layout were adapted from the attributed
[Java DAX reference](../prior-research/macmaps/code/DaxReader.java), and the
GEO accessors from its [GEO reference](../prior-research/macmaps/code/GeoMap.java).
Both are from a Mac research snapshot. The same DAX/RLE layout and GEO plane
sizes were independently checked on the supplied DOS files in the
[DOS kit audit](../FABLE_AUDIT.md). The local C audit verifies archive and
record counts independently; its command and result are recorded below after
execution. Wall appearance, door state names, and movement rules remain
behavioral questions for the DOS code workstream. The adapted reference code
retains the [Gold Box Explorer MIT notice](GoldBoxExplorer-MIT.txt).

## Local DOS v1.3 audit

On 2026-10-10, the command above completed successfully on all 113 ignored
`resources/DOS/POOLRAD/*.DAX` inputs:

```text
archives=113 blocks=1245 geo_records=29
```

All 1,245 blocks decoded to their declared lengths. All 29 blocks from the
eight `GEO*.DAX` archives parsed at exactly 1,026 bytes. These measurements
agree with the earlier [DOS kit audit](../FABLE_AUDIT.md) but come from the C
reader here. The target DOS executable SHA-256 checksums were:

```text
START.EXE  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
GAME.OVR  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614
```

No original file or decoded content is part of this change.
