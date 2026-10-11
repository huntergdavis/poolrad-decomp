# ECL container and tagged operands

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

At a **caller-supplied operand offset**, `pr_ecl_operand_at` sizes only these
evidenced tag forms:

| tag | bytes at the operand | value bytes |
| --- | --- | ---: |
| `00` | `00 BB` | 1 |
| `01`, `02`, `03` | `tag LO HI` | 2 |
| `80` | `80 LL` followed by `LL` packed bytes | `LL` |
| `81` | `81 LO HI` | 2 |

The borrowed descriptor returns the tag, source and payload offsets, payload
size, and exclusive end offset. A complete operand may end exactly at the
record boundary. An unknown tag returns `PR_ECL_OPERAND_TAG`; a truncated
operand or offset outside the record returns `PR_ECL_OPERAND_FRAME`. The
reader uses remaining-byte checks before indexing or adding lengths. It
neither resolves address values nor infers instruction boundaries.

`pr_ecl_text_at` retains the existing `80 LL` framing contract and returns
`PR_ECL_TEXT_FRAME` for a non-text or truncated frame. A byte containing
`0x80` elsewhere in the script is not thereby a text operand. The decoder
reads complete six-bit codes MSB-first, four per three packed bytes, through
the declared packed-byte boundary. It **skips zero codes** and emits later
nonzero codes, including the final fourth code of a full three-byte group.
Codes 1 through 31 map to ASCII `code + 0x40`; codes 32 through 63 are their
ASCII values. `pr_ecl_text_decode` writes a NUL-terminated result to a caller
buffer. On `PR_ECL_OUTPUT_SIZE`, `written` reports the character count without
the terminator; capacity must be at least `written + 1`. A 255-byte packed
frame can produce up to 340 characters, so the format decoder does not impose
the provisional 255-character Pascal string cap of the DOS runtime.
Neither API searches the instruction stream or prints decoded text.

The header and operand framing follow lane A's byte-checked
[ECL contracts in PR #6](https://github.com/huntergdavis/poolrad-decomp/pull/6)
and the prior [DOS audit](../FABLE_AUDIT.md). The zero-code behavior is from
the DOS unpacker disassembly in the merged
[PR #11 function ledger](https://github.com/huntergdavis/poolrad-decomp/pull/11).
The 255-character Pascal string cap in that ledger remains provisional. The
six-bit layout was adapted
from the attributed Gold Box Explorer reference cited there; its
[MIT notice](GoldBoxExplorer-MIT.txt) is retained. The initial `deja` query for
this issue returned no exact match; a broader `deja "ECL"` recalled the
[prior Codex ECL container implementation in PR #7](https://github.com/huntergdavis/poolrad-decomp/pull/7),
which supplied the bounded record and text APIs extended here.

## Build, tests, and local audit

From the repository root:

```sh
cmake -S . -B /tmp/poolrad-ecl-build
cmake --build /tmp/poolrad-ecl-build
ctest --test-dir /tmp/poolrad-ecl-build --output-on-failure
/tmp/poolrad-ecl-build/poolrad_ecl_audit resources/DOS/POOLRAD/ECL*.DAX
```

The audit reads the owner's ignored DOS v1.3 archives into memory. It fully
decodes each DAX block, validates each ECL header and entry target, sizes
every bounded raw tag candidate, and cross-checks each bounded `0x80` span
with the text-frame decoder. Candidate counts are **not** counts of actual
operands: bytecode and inline data may contain these tag bytes. The audit
writes no decoded content and reports only aggregate counts or errors.

On 2026-10-10, the eight local `ECL1.DAX` through `ECL8.DAX` inputs returned:

```text
archives=8 records=29 entry_targets=145 unpacked_bytes=176265 bounded_text_candidates=6256 errors=0
```

On 2026-10-10, after adding operand spans and zero-code skipping, the same
eight ignored archives returned:

```text
archives=8 records=29 entry_targets=145 unpacked_bytes=176265 bounded_operand_candidates=49649 bounded_text_candidates=6256 errors=0
```

A separate read-only comparison used the existing `tools/overlay/ecl_walk.py`
reference opcode table to locate text frames reached by its linear sweep. It
found 1,582 frames, including 359 with zero codes, and **zero frames whose
decoded output changes** when later nonzero codes are retained. The sweep
stops at the first undecodable byte in 28 of 29 blocks, so this result applies
to reached frames only. Its opcode operand counts come from an upstream table,
not verified whole-engine grammar; [PR #11's ledger](https://github.com/huntergdavis/poolrad-decomp/pull/11)
reports the same reached-frame counts. Frame reachability through the full
corpus therefore remains provisional.

This independently matches lane A's 29-record, 145-target, 176,265-byte
measurements. The target DOS executable SHA-256 checksums were:

```text
START.EXE  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd
GAME.OVR  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614
```

Open contract questions from lane A concern operand counts, out-of-block VM
targets, and loader placement. They do not change this bounded record layout.
