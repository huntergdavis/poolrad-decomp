# Full C translation: resident character link predicate

`pr_rec_res_00ba_2e03` translates DOS v1.3 `START.EXE` routine
`RES_00BA_2E03`. Its loaded entry is `0x00BA:0x2E03`, expanded-image
offset `0x39A3`, and first packed source file offset `0x3A59`. The
[hash-locked loader map](RESIDENT_LOADER_MAP.md) confirms 67 contiguous
copied bytes through the next prologue at `0x39E6`.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Character role and callers

The [DOS character trace](../formats/DOS_CHARACTER.md) establishes the
285-byte character record and in-memory object accesses. At GAME.OVR
unit `0x5E:+0x0D23..+0x0D4E`, the caller updates fields `+0x113` and
`+0xA1` through its character argument, then passes that same far
pointer to `RES_00BA_2E03`. At unit `0x5E:+0x1B38..+0x1B4F`, the
caller passes its character argument, tests this routine's `AL` result,
then follows the same character `+0xCC/+0xCE` far pointer and reads the
linked record's `+0x2E` byte. Unit `0x31:+0x0DC1..+0x0DDB` passes its
character argument to this predicate and to `RES_00BA_2E46` in
sequence. These exact character-state uses establish a game-specific
role; the link's object type, the `+0x2E` field meaning, and the DS
table's gameplay meaning remain unknown.

The [callable ledger](resident_callable_ledger.csv) records ten
fixup-confirmed overlay far callers: unit `0x2F:+0x0D51` (GAME.OVR
file `0x0A558`), `0x2F:+0x0E5D` (`0x0A664`), `0x31:+0x0DC7`
(`0x0B987`), `0x31:+0x0EB5` (`0x0BA75`), `0x31:+0x0F6F`
(`0x0BB2F`), `0x5E:+0x0D4E` (`0x13549`), `0x5E:+0x1B3E`
(`0x14339`), `0x5E:+0x2B50` (`0x1534B`), `0x5E:+0x2D27`
(`0x15522`), and `0x5E:+0x30BC` (`0x158B7`). The neighboring
resident `RES_00BA_2E46` also near-calls this entry at expanded
`0x39F3`.

## Complete instruction path and effects

A Capstone 5.0.7 walk of the hash-checked expanded image finds **23
instruction starts**, direct branch edges `0x39B6→0x39D9`,
`0x39D1→0x39D9`, and `0x39D7→0x39DD`, **zero calls**, and a single
`RETF 4` at `0x39E3`. The only argument is the character far pointer
at `[BP+6,+8]`. The routine returns `AL=1` exactly when a non-null
character link leads to an indexed DS table byte above one; otherwise
`AL=0`. It makes no DOS memory writes.

| Expanded-image span | Observed access and branch |
| --- | --- |
| `0x39A9..0x39B6` | Read character words `+0xCC` and `+0xCE` separately. If both are zero, return zero without following the link. |
| `0x39B8..0x39D1` | Re-read both character link words through `LES`; read linked byte `+0x2E`, zero-extend it, shift left four, and read DS byte `0x558E + 16 × index`. |
| `0x39D1..0x39E6` | Set local byte to zero for table value `0` or `1`, or to one for values `2..255`; copy it to `AL` and return. |

The C callbacks preserve the read order, including the second link read,
and wrap far-field offsets to 16 bits. The first far-pointer null test
requires **both** words to be zero. An adapter failure returns a host
status and leaves the result unchanged; invalid far memory is outside
the DOS parity claim. Every successful DOS branch and external effect
is translated. No callee or heap behavior is omitted.

## Verification and prior work

Synthetic tests cover empty link, either single-word nonzero form,
table values zero, one, two, and 255, changed link contents between the
first and second reads, wrapped character and linked offsets, maximum
byte index, each callback failure, and invalid host arguments. Expected
callback traces derive from the instruction order; no game records are
included.

```sh
cmake -S . -B /tmp/poolrad-issue-57-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-57-build
ctest --test-dir /tmp/poolrad-issue-57-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --near --json /tmp/poolrad-issue-57-loader.json
```

Before implementation, `deja` queries for the next party/character
resident target, `RES_00BA_2228`, and an exact `RES_00BA_2E03`
character/link query found no earlier translation. The exact query
recalled this session's candidate selection only. Reused the merged
[character trace](../formats/DOS_CHARACTER.md), [resident callable
coverage](RESIDENT_CALLABLE_COVERAGE.md), and [loader map](RESIDENT_LOADER_MAP.md)
for record and address context, then checked the role and paths against
the hash-locked executables. No original media, saves, text, or
executable bytes enter Git.
