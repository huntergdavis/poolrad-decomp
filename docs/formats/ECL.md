# ECL container and text records

This C layer accepts one **already unpacked** ECL DAX block. It does not walk
or run ECL instructions. The [DAX reader](README.md) supplies the block bytes,
and the caller retains the original DAX directory ID and stored file offset.
The ECL record and text descriptors retain offsets relative to the start of
the unpacked block, including the two-byte prefix.

## Byte contract

The DOS v1.3 block is at most 65,535 bytes and begins with a little-endian
`0x1388` prefix of unknown meaning. Five four-byte entry instructions follow
at offsets 2, 6, 10, 14, and 18. Each is `01 01 lo hi`: opcode `0x01`, word
operand tag `0x01`, and a little-endian virtual target. Entry positions have
the names run, search location, pre-camp check, camp interrupted, and initial
entry in the reference interpreter. The C API exposes their positions and
target values; it does not depend on the names or interpret subsequent code.

The byte at unpacked offset 2 has virtual address `0x9900`:

```text
unpacked offset = virtual address - 0x9900 + 2
```

`pr_ecl_open` verifies the prefix, entry shapes, and that all five translated
targets are within the block. `pr_ecl_address_to_offset` maps only addresses
within this record. Other VM addresses may refer to engine memory and return
`PR_ECL_ADDRESS`; that is not a claim that the VM target is invalid.

At a **caller-supplied operand offset**, `pr_ecl_text_at` accepts `80 LL`
followed by exactly `LL` packed bytes. Its descriptor preserves the tag
offset, payload offset, packed byte count, and end offset. A byte containing
`0x80` elsewhere in the script is not thereby a text operand. The decoder
reads complete six-bit codes MSB-first, four per three packed bytes. It stops
at the first zero code, if present, or at the declared packed-byte boundary.
Codes 1 through 31 map to ASCII `code + 0x40`; codes 32 through 63 are their
ASCII values. `pr_ecl_text_decode` writes a NUL-terminated result to a caller
buffer. On `PR_ECL_OUTPUT_SIZE`, `written` reports the character count without
the terminator; capacity must be at least `written + 1`. Neither API searches
the instruction stream or prints decoded text.

The header and operand framing follow lane A's byte-checked
[ECL contracts in PR #6](https://github.com/huntergdavis/poolrad-decomp/pull/6)
and the prior [DOS audit](../FABLE_AUDIT.md). The six-bit layout was adapted
from the attributed Gold Box Explorer reference cited there; its
[MIT notice](GoldBoxExplorer-MIT.txt) is retained. The initial `deja` query for
prior ECL implementation sessions returned no relevant reusable session.

## Build, tests, and local audit

From the repository root:

```sh
cmake -S . -B /tmp/poolrad-ecl-build
cmake --build /tmp/poolrad-ecl-build
ctest --test-dir /tmp/poolrad-ecl-build --output-on-failure
/tmp/poolrad-ecl-build/poolrad_ecl_audit resources/DOS/POOLRAD/ECL*.DAX
```

The audit reads the owner's ignored DOS v1.3 archives into memory. It fully
decodes each DAX block, validates each ECL header and entry target, and checks
every in-bounds `0x80` candidate with the text-frame decoder. The candidate
count is **not** a count of actual text operands: bytecode and inline data can
contain `0x80` without starting a text frame. The audit writes no decoded
content and reports only aggregate counts or errors.

On 2026-10-10, the eight local `ECL1.DAX` through `ECL8.DAX` inputs returned:

```text
archives=8 records=29 entry_targets=145 unpacked_bytes=176265 bounded_text_candidates=6256 errors=0
```

This independently matches lane A's 29-record, 145-target, 176,265-byte
measurements. The target DOS executable SHA-256 checksums were:

```text
START.EXE  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
GAME.OVR  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614
```

Open contract questions from lane A concern operand counts, out-of-block VM
targets, and loader placement. They do not change this bounded record layout.
