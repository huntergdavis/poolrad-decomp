# Full C translation: resident party-record counter


`pr_rec_res_00ba_23db` translates DOS v1.3 `START.EXE` resident routine
`RES_00BA_23DB`. Its loaded entry is `0x00BA:0x23DB`, expanded-image
`0x2F7B`, with first packed source byte at file `0x303F`. The
[hash-locked loader map](RESIDENT_LOADER_MAP.md) verifies copied-source
provenance. The [callable ledger](resident_callable_ledger.csv) records
seven fixup-backed overlay callers.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Selection and game role

GAME.OVR unit `0x6F:+0x1446` is the character-record party insertion
routine identified in the [DOS character trace](../formats/DOS_CHARACTER.md).
When the party list is empty, it stores the supplied character far pointer
into DS globals `0x5D96/0x5D98` at `+0x145E..0x1467`. For a nonempty list,
it follows each record's far link at `+0x104/+0x106` at
`+0x1487..0x14A9`, then stores the new character pointer into that link
at `+0x14AB..0x14B9`. The loader separately clears this in-memory link
after the 285-byte disk read. `RES_00BA_23DB` starts at that exact global
head and follows that exact character link. It clears two DS bytes and
counts records whose byte `+0x10D` is nonzero, using the signed value of
byte `+0x10E` as a DS counter offset. This party-record and character-field
behavior establishes a game-specific role; the two byte fields' gameplay
meanings remain unknown and are kept raw.

The seven fixup-confirmed callers are unit `0x2F:+0x00B3` (GAME.OVR file
`0x098BA`), `0x2F:+0x0932` (`0x0A139`), `0x33:+0x19FB` (`0x0DFCC`),
`0x38:+0x04AC` (`0x0F107`), `0x5E:+0x1333` (`0x13B2E`),
`0x5E:+0x2A5B` (`0x15256`), and `0x9E:+0x191A` (`0x2C804`).
The smaller `RES_00BA_23B7` also reads `+0x10E`, but only returns a
zero/nonzero predicate; its field meaning and caller use are not yet
settled. The selected routine has the independent loader-proved party
head/link path and measurable global counter effects.

## Complete instruction boundary and contract

A Capstone 5.0.7 walk over the hash-checked expanded image finds **29
reachable instruction starts**, three direct branch edges (`2F9E→2FCF`,
`2FA9→2FBA`, `2FCD→2F98`), **zero calls**, and one `RETF` at `0x2FD2`.
It takes no stack arguments and defines no result register value. DS
global accesses and ES:DI character accesses remain distinct callbacks.

| Expanded-image span | Observed effect |
| --- | --- |
| `0x2F81..0x2F95` | Write zero to DS bytes `0x6814` and `0x6815` in that order. Read the party head's offset and segment words from DS `0x5D96` and `0x5D98`. |
| `0x2F98..0x2FA9` | Exit if both far-pointer words are zero. Otherwise read character byte `+0x10D`; zero skips the increment. |
| `0x2FAB..0x2FB6` | Read byte `+0x10E`, sign-extend it to a word at the one-byte, no-operand-override instruction `0x2FB3`, and form DS address `0x6814 + signed_index` with 16-bit wrap. |
| `0x2FB6..0x2FBA` | Increment that DS byte modulo 256. The DOS body constrains neither the index to `0/1` nor the destination to the two initialized bytes. |
| `0x2FBA..0x2FD2` | Read the character's next far pointer at `+0x104/+0x106`, then repeat or return. |

The C function preserves individual byte/word read and write order, both
far-pointer words in null tests, signed index arithmetic, byte overflow,
and 16-bit field-offset wrap. On a host callback failure, it reports a
host status without rolling back prior writes. `max_nodes` is a host guard
for corrupt cyclic lists; the DOS loop has no bound. Valid acyclic lists
with successful callbacks cover every decoded branch and state effect.
The unknown meanings of `+0x10D/+0x10E` are not inferred from this
arithmetic, and invalid-pointer host outcomes are not DOS parity claims.

## Verification and prior work

Synthetic tests cover empty and skipped records, mixed-party counts,
nonzero selection, index `0`, `1`, `2`, and `-1`, byte overflow,
one-word-zero non-null pointers, far field wrap, cycles, and failures at
each callback boundary. Expected event traces are fixed from the
instruction order, including the two initialization writes before any
head read and the read/modify/write counter operation.

```sh
cmake -S . -B /tmp/poolrad-issue-48-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-48-build
ctest --test-dir /tmp/poolrad-issue-48-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --near --json /tmp/poolrad-issue-48-loader.json
```

The strict build on base `259aebe` passed all ten CTest targets. The
project-wide reviewed strict count on that base is **11** (eight overlay,
three resident); this full resident translation would raise it to **12**
after review and merge. Before analysis,
`deja "resident segment 0xBA character sidecar next game function
candidate"`, `deja "resident callable ledger unresolved game specific
short routine"`, and exact queries for `RES_00BA_23DB` and
`RES_00BA_23B7` found no prior path translation. The merged
[character trace](../formats/DOS_CHARACTER.md), [resident callable coverage](RESIDENT_CALLABLE_COVERAGE.md),
and [loader map](RESIDENT_LOADER_MAP.md) supplied framing and address
context; their role claims were checked against the cited DOS instructions.
No original files, save bytes, decoded text, or executable bytes enter Git.
