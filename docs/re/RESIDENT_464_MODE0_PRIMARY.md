# RES_0464:02A5 video-state-0 first primary byte

This is a **bounded C model** of the first primary-byte group after the
merged [first-pixel prefix](RESIDENT_464_MODE0_PREFIX.md). It stops at
`+0x04CE`, before the second primary-byte group and subsequent pixel
loops. It is not a full parent translation or proof of live LOAD MONSTER
execution. No strict full-C or game-identity credit is proposed.

## Prior work and byte provenance

Before debugging, deja queries `Poolrad RES_0464:02A5 route 0 primary
nibble +03D1 +04BE helper table` and `0464:02A5 route 0 primary`
found no matching prior instruction trace. The reviewed
[route-0 prefix](RESIDENT_464_MODE0_PREFIX.md) supplied the
`+0x03D1` entry and live local-state lead. The merged
[nibble helper](RESIDENT_464_NIBBLE_007B.md) supplied the reviewed
eight-byte source/selector/AL contract. The first group was separately
decoded from ignored `START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Span | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| First primary-byte group | `0x04A0E..0x04B0B` | `0x04A47..0x04B44` | 253 | `b3f223427391b4fc1a5fd4b2d8564ce8363e5c1d5e9623f431c294b01bf04cfe` |

The interval includes the `+0x03CE` index increment used as a loop
backedge. Initial entry is `+0x03D1`; `+0x04C9` jumps to `+0x04CE`.
The hash-locked audit verifies contiguous START source provenance,
103 complete instructions, six branch sites, the two near helper calls,
both private lookup reads, three direct ES write sites, the auxiliary
read, data-offset increment, and continuation. It also rechecks the
full route and parent procedure hashes.

## Instruction and side-effect order

| Unit offset | Original behavior | C model |
| --- | --- | --- |
| `+0x03D1..+0x03EC`, `+0x0427..+0x0442` | For indices 0..3, choose high nibble for even indices and low nibble for odd indices from the four-byte local copy. Pass that nibble and the object header `+9` far pointer to `RES_0464:007B`. | Read the prefix's copied bytes; call merged `pr_r464_nibble_007b` once for each reached index. |
| `+0x03F7/+0x044D` | The two static near call sites return AL in `0..15`; each executes its own eight-byte runtime Move inside the helper. | Preserve four dynamic calls across indices 0..3 and report successful calls. |
| `+0x0405/+0x045B` | Read a private DS byte at `+0x25FA + 4*AL + index`. | Require only the reached entry of a host-supplied synthetic lookup. |
| `+0x0421/+0x0477` | OR that byte into primary object data at header `+0x17` plus the current data offset, which is zero at first-pixel entry. | Write after each successful lookup, retaining earlier OR effects on a later host stop. |
| `+0x0484..+0x04BE` | Reread auxiliary far-pointer WORDs at header `+0x13/+0x15`. If nonzero, read auxiliary byte at WORD index `auxiliary_index-2` and AND the assembled primary byte with its complement. | Perform the optional read and AND after all four OR writes. |
| `+0x04C2..+0x04C9` | Increment primary data offset to one, set next index to four, jump to second-group entry `+0x04CE`. Source frame offset remains unchanged. | Report `data_offset=1`, `primary_index=4`, and `next_offset=+0x04CE`. |

The two static OR sites execute twice each in a complete first group.
The third direct write at `+0x04BE` is conditional. The private table's
contents and any live saved-route value remain unknown.

## Boundary and host stops

`pr_r464_mode0_first_primary_byte` requires a successful prefix report
at `+0x03D1` and the same nonaliasing object/auxiliary arrays. It
checks the eight-byte helper source at object `+9`, each reached lookup
entry, primary data byte, and optional auxiliary mask byte. Missing
host memory stops at that access and retains earlier writes. The model
does not emulate far-offset wrapping, aliasing, or mutation of the
snapshot between calls. The second primary group and next-pixel
control flow remain outside this C boundary.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_mode0_primary_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

Tests use synthetic sources, headers, auxiliary bytes, and lookup
entries. Original media and private lookup values remain ignored.
