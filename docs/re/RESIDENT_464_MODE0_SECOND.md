# RES_0464:02A5 video-state-0 second primary byte

This is a **bounded C model** of the second primary-byte group for the
first route-0 pixel, composed after the reviewed
[auxiliary prefix](RESIDENT_464_MODE0_PREFIX.md) and
[first primary byte](RESIDENT_464_MODE0_PRIMARY.md). It stops before the
y/x/layer continuation at `+0x05CB`. It is not a full translation of
the parent procedure or proof of a live LOAD MONSTER route, and it
adds no strict full-C or game-identity credit.

## Prior work and byte provenance

Before implementation, deja queries `Poolrad RES_0464:02A5 route 0
second primary byte +04CE +05C7` and `0464:02A5 second primary`
found no matching instruction trace. The two reviewed route-0 slices
supplied the exact input state; the merged
[nibble helper](RESIDENT_464_NIBBLE_007B.md) supplied its bounded
eight-byte source/selector/AL contract. This group's instructions
were independently decoded from ignored `START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Span | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| Second primary-byte group | `0x04B0B..0x04C0B` | `0x04B44..0x04C44` | 256 | `e72925f180113d69b94255931a7db9152030c3e4d858a74f2861708108d1b497` |

Initial entry is `+0x04CE`; the interval also contains the
`+0x04CB` backedge increment for indices 5..7. It ends after the
source-offset update at `+0x05C7`, so the next instruction is
`+0x05CB`. The audit rechecks the full route and first-primary-byte
hashes, contiguous START source mapping, 103 complete instructions,
five branch sites, two near calls, two private lookup reads, three ES
writes, both index-minus-four sites, the auxiliary read, and the two
final offset updates. It prints addresses, counts, and hashes only.

## Instruction and side-effect order

| Unit offset | Original behavior | C model |
| --- | --- | --- |
| `+0x04CE..+0x04EC`, `+0x0527..+0x0545` | For indices 4..7, select high nibble for even indices and low nibble for odd indices from the copied source's third and fourth bytes. Pass that nibble and object header `+9` to `RES_0464:007B`. | Use the retained first-pixel copy and merged helper for four dynamic calls. |
| `+0x04F7/+0x0550` | Two static near sites return helper AL in `0..15`. | Report each successful helper call. |
| `+0x0505/+0x055E` | Read DS lookup byte at `+0x25FA + 4*AL + (index-4)`. Both path arms explicitly subtract four from the index. | Read only the reached synthetic host table entry. |
| `+0x0521/+0x057A` | OR the lookup byte into the second primary object byte, at header `+0x17` plus data offset one. | Preserve the four OR effects in DOS order. |
| `+0x0587..+0x05C0` | Reread auxiliary pointer presence; if nonzero, read auxiliary byte at WORD index `auxiliary_index-1`, complement it, and AND the second primary byte. | Perform this optional read and write after all four ORs. |
| `+0x05C4/+0x05C7` | Increment data offset to two, then add four to the mutable source frame offset. Both are WORD operations. | Update the report only after all reached writes, with WORD wrap. |

The two OR write sites execute twice each in a complete group. The
third ES write at `+0x05C0` is conditional. The private DS table
contents and live video-state value remain unknown.

## Boundary and host stops

`pr_r464_mode0_second_primary_byte` requires the successful first
primary-byte report at `+0x04CE` and its preceding prefix report.
Source, object, auxiliary, and table mappings must represent the same
nonaliasing snapshot. The host checks the reached helper source,
lookup byte, primary destination, and optional auxiliary mask byte
in instruction order. Missing host memory stops there without undoing
earlier writes. The model does not emulate wrapped far offsets or
unresolved runtime state. At success, source offset has advanced by
four and data offset by one; the y/x/layer loop, repeated pixels, and
route return remain outside this C boundary.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_mode0_second_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

Tests use synthetic data only. Original media and the private DS lookup
remain under ignored resources.
