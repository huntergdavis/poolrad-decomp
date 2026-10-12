# RES_0464:007B eight-byte nibble selector

This is a **bounded C model** of the resident helper called four times by
the merged [RES_0464:02A5 postprocessor map](RESIDENT_464_POSTPROCESS_MAP.md).
It reproduces all returned values for the caller-proved selector range
`0..15` and readable eight-byte sources. It does not model out-of-range
reads from the DOS stack frame, prove live LOAD MONSTER execution, or
change strict full-C or game-identity accounting.

## Prior work and byte provenance

Required deja queries `Poolrad RES_0464:007B nibble helper runtime Move
frame` and `0464:007B` found no matching prior session. The merged
postprocessor map supplied the four call-site and span leads. The merged
[LOAD MONSTER map](U1C_LOAD_MONSTER_0450_MAP.md) independently identifies
runtime `0x0AF8:0x0A3F` as Pascal `Move`, decoding its `rep movsb` and
`RETF 0x0A`. This helper's frame, source/destination push order, branches,
and caller selectors were decoded afresh from ignored `START.EXE`,
SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Procedure | Loaded image, exclusive end | START file, exclusive end | Bytes | SHA-256 |
| --- | --- | --- | ---: | --- |
| `RES_0464:007B` | `0x046BB..0x0470F` | `0x046F4..0x04748` | 84 | `d16625eb920138f29792a8d6587b923b03737a7d832585b9c0567492fbdce5d7` |

The hash-locked span has contiguous unpacker source provenance and 37
complete instructions. Its two branch sites `+0x0099` and `+0x00AC`
target instructions inside the span. It has one runtime call at
`+0x008F`, two direct writes to local `[BP-1]` at `+0x00A9/+0x00C3`,
and one `RETF 6` at `+0x00CC`. The next independent prologue begins at
`+0x00CF`. The runtime call also writes eight bytes to the local buffer;
there are no persistent object or global writes in this helper.

## Frame and effects

The far frame reads source pointer `[BP+6]` and the low selector byte at
`[BP+0x0A]`. `RETF 6` consumes the far pointer and selector WORD; AL
returns the selected value. The high selector byte is ignored.

| Offset | Original behavior | C model |
| --- | --- | --- |
| `+0x0081..+0x0094` | Push source far pointer, local `SS:[BP-9]` destination, and length 8, then call runtime `Move`. | Copy exactly eight bytes into a local array. Require an eight-byte readable host source first. |
| `+0x0094..+0x009B` | Shift selector byte right once. Carry distinguishes odd from even; `JAE` selects the even branch. | Test selector bit 0, then divide index by two. |
| `+0x009B..+0x00AC` | Odd selector reads local byte `selector / 2`, masks its low nibble, stores it to local result. | Return `byte & 0x0F`. |
| `+0x00AE..+0x00C6` | Even selector reads the same indexed byte, masks its high nibble and shifts right four, stores local result. | Return `byte >> 4`. |
| `+0x00C6..+0x00CC` | Load local result into AL and return. | Write the host result only on success. |

The four calls from `RES_0464:02A5` are at `+0x03F7`, `+0x044D`,
`+0x04F7`, and `+0x0550`. Each passes a far pointer to object header
`+9`. The first and third selectors follow an `AND 0x0F`; the second
and fourth follow `AND 0xF0` and a four-bit right shift. Their selectors
are therefore `0..15`. This is static caller evidence, not proof that a
particular saved-route invocation reaches any call.

For selector values `16..255`, the original index `selector / 2` reads
beyond the eight bytes copied into the local frame. The neighboring
stack contents and runtime fault behavior are not a portable data
contract. The C model rejects that range and sources shorter than eight
bytes before copying; these are **host bounds**, not DOS branches. Null
source/result pointers are also host argument errors. No callback is
needed for the reviewed eight-byte `Move` semantics on a valid source.

Synthetic tests exercise each of the 16 caller-reachable selectors on
distinct source nibbles and verify output remains untouched on short,
null, and out-of-range inputs.

## Reproduction

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 \
  tools/overlay/resident_464_nibble_007b_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE
cmake -S . -B build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build build
ctest --test-dir build --output-on-failure
```

The read-only audit hash-locks the executable and 84-byte span, verifies
source mapping, complete instruction extent, Move argument order, branch
targets, local writes, four calls and their selector producers, then
emits only addresses, counts, and hashes. No original game bytes or
media enter Git.
