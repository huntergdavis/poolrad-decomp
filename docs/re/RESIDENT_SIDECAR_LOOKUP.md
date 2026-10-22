# Full C translation: resident 9-byte sidecar lookup


`pr_rec_res_00ba_2199` translates DOS v1.3 `START.EXE` resident routine
`RES_00BA_2199`. Its loaded entry is `0x00BA:0x2199`, expanded-image
`0x2D39`, with first packed source byte at START.EXE file `0x2E15`.
The [hash-locked loader map](RESIDENT_LOADER_MAP.md) supplies the expanded
image; the [callable ledger](resident_callable_ledger.csv) lists this exact
Pascal prologue and 39 fixup-confirmed GAME.OVR callers across nine units.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Why this is a game routine

The [DOS character trace](../formats/DOS_CHARACTER.md) identifies
`+0x7F/+0x81` as an in-memory far-pointer field in the 285-byte character
object and establishes 9-byte `.SPC` sidecar units. The executable load
path in GAME.OVR unit `0x6F` clears that field at `+0xC72..0xC7B`,
allocates nine bytes at `+0xDB6..0xDBF`, stores the returned pointer into
character `+0x7F/+0x81` at `+0xDBF..0xDC6`, reads nine bytes into the node
at `+0xE18..0xE28`, and clears the node's `+5/+7` far link at
`+0xE2D..0xE37`. Subsequent nodes are allocated and linked through
`+5/+7` at `+0xE4A..0xE5B`. The candidate reads exactly this character
head and node link, and compares byte zero of each node with a caller
argument. Its 39 overlay callers include units `0x1C`, `0x25`, `0x38`,
and `0x77`. This specific character-sidecar use supports game
classification independently of the routine's short size. The meaning of
the node's byte-zero key remains raw; no spell identifier meaning is
asserted from its position alone.

Two smaller resident candidates were not selected: `RES_00BA_148B`
mutates a byte in a 21-position global table whose role is not yet proved;
`RES_00BA_23B7` tests character byte `+0x10E` but that field's meaning and
caller use remain unverified. The 37-instruction lookup has a stronger
game-specific object and sidecar trace with no external callee to model.

## Complete instruction boundary

A Capstone 5.0.7 path walk over the hash-checked expanded image finds
**37 reachable instruction starts**, five direct branch edges
(`2D63→2D99`, `2D69→2D99`, `2D77→2D7F`, `2D7D→2D97`,
`2D97→2D59`), **zero calls**, and one `RETF 0x000A` at `0x2DA5`.
There is no indirect branch on the decoded path. The first far argument
`[BP+6,+8]` is a caller-supplied cursor cell; the word argument at
`[BP+0xA]` supplies a low-byte key (its high byte is ignored); the last
far argument `[BP+0xC,+0xE]` is the character object. DOS returns
`AL=1` for found and `AL=0` for exhausted or empty lists. The cursor cell
contains the matching node on success or an all-zero pointer on failure.

| Expanded-image span | Observed effect |
| --- | --- |
| `0x2D3F..0x2D55` | Initialize local match flag to zero, read character head at `+0x7F/+0x81`, write both words into caller's cursor cell. |
| `0x2D59..0x2D69` | Re-read both cursor words. Exit if both are zero or the match flag is one; the match path therefore makes one final cursor read. |
| `0x2D6B..0x2D7D` | Re-read the cursor cell, read byte zero of that node, compare it with the argument's low byte, and set the match flag on equality. |
| `0x2D7F..0x2D97` | On mismatch, re-read the cursor cell yet again, read node next at `+5/+7`, write it to the cursor cell, and loop. |
| `0x2D99..0x2DA5` | Copy the local zero-or-one flag to `AL` and return, popping ten argument bytes. |

The C callbacks preserve the individual word and byte access order,
including repeated cursor reads. Far-pointer null requires both words to
be zero; field offsets wrap to 16 bits. The cursor cell can alias the
character head, in which case a failed search also advances that head:
the synthetic alias test checks this observable behavior. Host adapter
failures leave the returned `found` output unchanged, while previous
far-memory writes remain. `max_nodes` is an explicit host guard for
corrupt cycles; the DOS loop has no such bound. Valid acyclic lists with
successful callbacks cover all decoded branches. Invalid pointer behavior
and callback failure are not claimed as DOS parity.

## Verification and prior work

Synthetic tests cover empty head, head and later matches, exhaustion,
the ignored high byte of the word key argument, one-word-zero
non-null pointers, cursor offset wrap, cursor/head aliasing, a cycle guard,
and word/byte callback failures. Expected event traces are fixed from the
instruction order rather than generated from the implementation.

```sh
cmake -S . -B /tmp/poolrad-issue-42-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-42-build
ctest --test-dir /tmp/poolrad-issue-42-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --near --json /tmp/poolrad-issue-42-loader.json
```

The initial strict build passed all nine CTest targets. Before analysis,
`deja "RES_00BA_2199 character spell sidecar lookup"` and `deja "DOS
character 9-byte sidecar pointer 0x7f list"` found no exact prior
translation. The merged [character trace](../formats/DOS_CHARACTER.md),
[resident callable coverage](RESIDENT_CALLABLE_COVERAGE.md),
[loader map](RESIDENT_LOADER_MAP.md), and prior resident [removal](RESIDENT_SIDECAR_REMOVE.md)
and [append](RESIDENT_SIDECAR_APPEND.md) translations supplied the verified
field, mapping, and list context reused here. The owner-supplied binaries
remain ignored under `resources/`; no decoded game strings, saves, or
executable bytes are committed.

## Checker registration (2026-10-28)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py RES_00BA_2199 --require-current-main
```
