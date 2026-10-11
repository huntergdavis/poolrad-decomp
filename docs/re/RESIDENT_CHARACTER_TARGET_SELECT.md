# Full C translation: resident character target selection

`pr_rec_res_00ba_2e8b` translates DOS v1.3 `START.EXE`
`RES_00BA_2E8B` at `0x00BA:0x2E8B`. The hash-locked loader places
its entry at expanded image `0x03A2B`. All **204 body bytes** through
the `RETF 8` at `0x03AF4` have contiguous copy provenance from packed
START.EXE file `0x03AE1..0x03BAC`, with no fill bytes. The next exact
Pascal prologue begins at `0x03AF7`; the return ends at that boundary.
This mapping is proved for the exact DOS files below, not extrapolated
from a segment-wide linear offset.

| Ignored local input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Caller proof and game role

Six GAME.OVR far-call sites have fixups on the segment operand and
target `0x00BA:0x2E8B` exactly:

| Overlay unit:site | GAME.OVR file offset | Caller use |
| --- | --- | --- |
| `0x031:+0x0EC9` | `0x0BA89` | Passes the same character pointer as preceding `RES_00BA_2E03`, plus a local far-pointer output cell; saves AL. |
| `0x038:+0x1045` | `0x0FCA0` | Checks character `+0xCC/+0xCE`, then passes that record and a local output cell; tests AL. |
| `0x05E:+0x0D62` | `0x1355D` | Updates character bytes `+0xA1/+0x113`, calls `2E03` with that character, then calls `2E8B` with a local output cell and tests AL. |
| `0x05E:+0x2B79` | `0x15374` | Passes a record argument and local output cell; tests AL. |
| `0x05E:+0x2D3B` | `0x15536` | Calls `2E03` and then `2E8B` with the same character pointer; tests AL. |
| `0x05E:+0x30D0` | `0x158CB` | Calls `2E03` and then `2E8B` with the same character pointer; tests AL. |

The [character trace](../formats/DOS_CHARACTER.md) establishes the
285-byte character record. The reviewed
[character-link predicate](RESIDENT_CHARACTER_LINK_CHECK.md) proves
`RES_00BA_2E03` uses its `+0xCC/+0xCE` far link and the indexed DS
table. Those exact caller chains make this a **game-specific
character-state selector**, not a routine classified by resident
segment alone. This branch does not edit the shared issue #24 ledgers
or increment a reviewed count. The linked object's type, its `+0x2E`
byte's gameplay meaning, the DS table flags' meanings, and the two
alternate character far slots' meanings remain unresolved.

## Complete bounded behavior

An independent Capstone 5.0.7 path walk found **69 reachable
instruction starts / 204 bytes**, **zero calls**, seven direct
conditional branch edges, and one `RETF 8` at `0x03AF4`. All branch
targets stay inside the body. The conditional edges are
`0x03A5E→0x03AD8`, `0x03A7D→0x03A8F`, `0x03A96→0x03AD8`,
`0x03A9F→0x03AB8`, `0x03ABF→0x03AD8`, `0x03AE2→0x03AEA`, and
`0x03AE8→0x03AEE`.

The routine takes a far output-cell pointer at `[BP+6,+8]` and a far
character pointer at `[BP+0xA,+0xC]`. It writes no other persistent
memory. Accesses and effects occur in this order:

| Expanded-image span | Observed effect |
| --- | --- |
| `0x03A31..0x03A54` | Read character link words `+0xCC/+0xCE` into locals, then write zero to the output cell's offset and segment words, separately and in that order. |
| `0x03A58..0x03A76` | If both link words are zero, skip the linked-object and table reads. Otherwise read linked byte `+0x2E`, multiply its unsigned value by 16, and read DS byte `0x5590 + 16 × index`. |
| `0x03A76..0x03A8F` | If table bit `0x10` is set, write the saved character link to the output cell, offset word then segment word. |
| `0x03A8F..0x03AB8` | If bit `0x08` is clear, skip both alternate slots. Otherwise, if bit `0x01` is set, read character far words `+0xF8/+0xFA` and overwrite both output words. |
| `0x03AB8..0x03AD8` | If bit `0x80` is set under the bit-`0x08` gate, read character far words `+0xFC/+0xFE` and overwrite both output words last. The slots are not dereferenced here. |
| `0x03AD8..0x03AF7` | Reread both output words. AL becomes one when their OR is nonzero or the complete table byte equals `0x0A`; otherwise AL is zero. AH is the high byte of that OR. |

Every far-field addition wraps to 16 bits. An output word may change
between its first and second writes or between final reads; the C
interface exposes each access as a callback in DOS instruction order.
There is no DOS null check for the character or output-cell argument.
On a host callback failure, the C function stops with AX unchanged
and leaves any earlier output writes intact; this is a host error
contract, not an additional DOS path. Successful callbacks cover all
decoded branches and effects without an omitted callee.

## Verification and prior work

Synthetic tests cover a null link, every flag gate in both directions,
link selection, both alternate slots and their last-write priority,
the exact `0x0A` success case with null output, override by a null
pointer, max index `255`, 16-bit wrapped record/link/output offsets,
a partially zero far pointer, the final AH value, all 18 callback
failure positions on the longest path, and invalid host arguments.
The focused test and the full strict CTest suite pass **18/18** on base
`004e97b`. The hash-locked loader audit also completes.

```sh
cmake -S . -B /tmp/poolrad-issue73-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue73-build
ctest --test-dir /tmp/poolrad-issue73-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --near --json /tmp/poolrad-issue73-loader.json
```

Before analysis, `deja "poolrad RES_00BA_2E8B issue 73 callers game role
bounded routine"` found no prior exact session; a broader address
query found only the assignment and a PR #71 progress mention. The
[loader map](RESIDENT_LOADER_MAP.md),
[callable coverage](RESIDENT_CALLABLE_COVERAGE.md),
[character trace](../formats/DOS_CHARACTER.md), and reviewed
`2E03` predicate supplied address and record context. The entry,
source span, all six fixup-backed calls, caller use, branch graph,
and side effects were independently checked against the hash-matched
files. No original executable, media, save, extracted text, or
decoded bytes enter Git.
