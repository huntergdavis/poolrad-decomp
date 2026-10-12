# RES_0464:02A5 video-state-0 first-pixel prefix

This is a **bounded C model** of the first pixel's auxiliary prepass on
the raw video-state-0 route. It stops before the primary nibble-helper
paths. It is not a full translation of route 0 or the parent procedure,
and it does not establish live LOAD MONSTER execution or game-specific
identity.

## Prior work and provenance

Before this implementation, deja query `Poolrad RES_0464:02A5 route 0 Move
helper branch postprocessor` found no match. The shorter `0464:02A5 route 0`
recalled this worker's prior route-1 handoff, but no route-0 instruction
proof. The merged [postprocessor map](RESIDENT_464_POSTPROCESS_MAP.md)
supplied the parent extent and call-site leads; the merged
[bitmask helper](RESIDENT_464_BITMASK.md) and
[nibble helper](RESIDENT_464_NIBBLE_007B.md) supplied independently reviewed
callee contracts. The route below was separately decoded from ignored
`START.EXE`, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Span | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| Complete route 0 | `0x04913..0x04C2F` | `0x0494C..0x04C68` | 796 | `5b3cd8261e021741533286f4acbcd7c6bedb71d5fd13b3363698ac067f9ffc66` |
| Implemented prefix | `0x04913..0x04A0E` | `0x0494C..0x04A47` | 251 | `62cab58be4313e124ebd1ae7778e543cfc60d51f22f6bc9865d36c2ba8dc9b68` |

The route contains 313 complete instructions and 34 branch sites;
the prefix contains 94 instructions and 16 branch sites. The read-only
audit also checks the parent procedure hash, contiguous START source
mapping, all six route call sites, seven direct ES write sites, the
auxiliary gate, pattern-table read, source-offset update, and prefix
continuation. It emits addresses, counts, and hashes only.

## Route-0 branch and effect map

The caller frame and nonzero-object gate are documented in the parent
map. The route starts only when DS byte `[0x4A1E]` is zero. The parent
initializes data offset `[BP-0x1C]` and auxiliary index `[BP-0x1E]` to
zero before dispatch. Byte `[BP+0x0C]` remains unread.

| Unit offset | Proven effect |
| --- | --- |
| `+0x02D3..+0x0325` | Count byte at object `+8` gates layer loop; zero count goes to common return. Nonzero count enters width/height WORD loops at x=y=0. Each dimension is decremented as a WORD, so zero wraps and still reaches the first pixel. |
| `+0x0328..+0x033B` | One runtime `0x0AF8:1685` Move copies four source bytes at the mutable frame offset to a local stack buffer. |
| `+0x0340..+0x03CC` | Nonzero auxiliary far-pointer WORD pair at object `+0x13/+0x15` calls `RES_0464:0000` once on the copied bytes and raw selector. The eight mask bits gate low-byte reads from four private DS WORDs indexed 4..7. Selected values OR into auxiliary byte 0 for bits 7..4 and byte 1 for bits 3..0, with one index advance after each group. A zero pointer skips this work. Both paths initialize the primary nibble index and jump to `+0x03D1`. |
| `+0x03D1..+0x0481` | Primary indices 0..3 select low/high nibbles of the copied bytes, call `RES_0464:007B` at `+0x03F7/+0x044D`, read a private lookup indexed by helper AL and index, and OR primary object bytes. If auxiliary is present, the first resulting primary byte is ANDed with the complement of auxiliary byte 0 at `+0x04BE`. |
| `+0x04C2..+0x0584` | Primary data offset advances one WORD byte; indices 4..7 similarly call `RES_0464:007B` at `+0x04F7/+0x0550` and OR primary bytes. |
| `+0x0587..+0x05EC` | If auxiliary is present, the second primary byte is ANDed with complement of auxiliary byte 1 at `+0x05C0`. Data offset advances; source frame offset advances by four at `+0x05C7`, then nested y/x/layer loops continue or jump to the common return. |

Static route-0 direct ES writes are
`+0x03AC,+0x0421,+0x0477,+0x04BE,+0x0521,+0x057A,+0x05C0`.
The first is an auxiliary OR; the remaining sites write primary data.
This map does not assert any saved-route branch value or decode private
lookup contents.

## C boundary and host stops

`pr_r464_mode0_first_pixel_prefix` represents one invocation from the
route-0 gate through the first pixel's auxiliary prepass. Zero count
reports the common return `+0x08CE` without reading source or auxiliary
fields. A nonzero count reaches x=y=0; after the four-byte Move, it
tests the auxiliary pointer. On a present pointer it calls merged
`pr_r464_bitmask`, processes the two four-bit groups in DOS order,
and reports continuation at `+0x03D1`. The source and data offsets have
not advanced at this boundary.

Source, object, and auxiliary far allocations are separate host arrays.
The four low bytes of the private pattern WORDs are supplied as a
synthetic host table. A source span, header, table entry, or auxiliary
byte is checked when its DOS instruction is first reached; a missing
mapping stops there and retains earlier Move and OR effects. A source
span crossing the 16-bit segment boundary is also a host stop. The
model assumes the far arrays do not alias and does not emulate a
wrapped far offset. The primary lookup table, four nibble-helper
call sites, later primary writes, repeated pixels, and native runtime
state remain outside this C boundary.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_mode0_prefix_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

Tests use only synthetic bytes. Original game media and the private DS
table remain under ignored resources.
