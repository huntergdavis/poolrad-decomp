# Full C translation: resident character field zero predicate

`pr_rec_res_00ba_23b7` translates DOS v1.3 `START.EXE` routine
`RES_00BA_23B7`. Its loaded entry is `0x00BA:0x23B7`, expanded-image
offset `0x2F57`, and first packed source file offset `0x301B`. The
[hash-locked loader map](RESIDENT_LOADER_MAP.md) establishes copied-source
provenance. The contiguous 36-byte body ends just before the next exact
Pascal prologue at expanded offset `0x2F7B`.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Game role and callers

The [DOS character trace](../formats/DOS_CHARACTER.md) establishes the
285-byte in-memory character record, and the merged [party counter](RESIDENT_PARTY_COUNT.md)
independently establishes a traversal of those records from DS
`0x5D96/0x5D98` through each character's `+0x104/+0x106` link. That
counter reads character byte `+0x10E` as a signed counter index when byte
`+0x10D` is nonzero. `RES_00BA_23B7` takes a character far pointer and
returns whether that same `+0x10E` byte is zero. The byte's gameplay
meaning remains unknown; the specific character-record field and party
counter use establish a game role without assigning it one.

The [callable ledger](resident_callable_ledger.csv) lists ten fixup-backed
GAME.OVR far callers: unit `0x31:+0x02A9` (file `0x0AE69`),
`0x31:+0x0420` (`0x0AFE0`), `0x31:+0x05D4` (`0x0B194`),
`0x38:+0x1ADC` (`0x10737`), `0x38:+0x1BEE` (`0x10849`),
`0x5E:+0x1F9F` (`0x1479A`), `0x5E:+0x292D` (`0x15128`),
`0x5E:+0x29A4` (`0x1519F`), `0x86:+0x104E` (`0x28066`), and
`0x86:+0x2C1A` (`0x29C32`). At unit `0x31:+0x0420`, the caller passes
its character pointer from `[BP+6,+8]`, then uses the returned `AL` to
index DS `0x6814`; unit `0x31:+0x05D4` uses the same counter base. A
resident near call at expanded `0x3041` also passes a character pointer
and compares the predicate with another character's `+0x10E` byte at
`0x3064..0x306B`. These uses independently tie the predicate to game
character and party state.

## Complete instruction path

A Capstone 5.0.7 decode of the hash-checked expanded image yields **13
reachable instruction starts**, direct branch edges `0x2F66→0x2F6E`
and `0x2F6C→0x2F72`, **zero calls**, and one `RETF 4` at `0x2F78`.
The far character pointer is `[BP+6,+8]`; there are no other arguments.

| Expanded-image span | Observed effect |
| --- | --- |
| `0x2F57..0x2F60` | Establish a two-byte local, load ES:DI from the far character argument, and compare the byte at `ES:[DI+0x10E]` with zero. |
| `0x2F66..0x2F72` | Store local byte `1` when equal to zero; otherwise store `0`. |
| `0x2F72..0x2F7B` | Return the local byte in `AL`, restore the stack frame, and pop the four argument bytes with `RETF 4`. |

The C function makes one far-byte callback at a 16-bit-wrapped offset and
sets its output to the exact `AL` value, `0` or `1`. It does not infer an
`AH` value or game field name. Invalid far accesses are host callback
failures, not DOS branch behavior; on such failure the output remains
unchanged. Both successful DOS branches have complete effects and no
untranslated dependency.

## Reproduction and prior work

Synthetic tests check zero, low nonzero, and high nonzero field bytes;
the exact single access, 16-bit offset wrap, one-word-zero pointer
forms, callback failure, and invalid host arguments. The tests contain
no DOS character data.

```sh
cmake -S . -B /tmp/poolrad-issue-53-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-53-build
ctest --test-dir /tmp/poolrad-issue-53-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --near --json /tmp/poolrad-issue-53-loader.json
```

Before implementation, `deja "remaining resident game routine bounded
candidate after RES_00BA_23DB"`, `deja "resident callable coverage
unclassified short game routines"`, and `deja "RES_00BA_23B7 character
field zero predicate"` found no exact prior translation. Reused the
merged [DOS character trace](../formats/DOS_CHARACTER.md), [callable
coverage](RESIDENT_CALLABLE_COVERAGE.md), [loader map](RESIDENT_LOADER_MAP.md),
and [party-counter evidence](RESIDENT_PARTY_COUNT.md) to locate the
record and candidate; checked the target and caller paths against the
locked executables. No media, saves, strings, or executable bytes are
committed.

## Reproduce (original-byte checker)

`RES_00BA_23B7` is checked against the original bytes by the registered checker `tools/overlay/res00ba_rec_models_audit.py`,
through the reusable io adapter `tools/overlay/res00ba_io_adapter.py`. What it compares is described in
[RES_00BA_REC_MODELS_AUDIT.md](RES_00BA_REC_MODELS_AUDIT.md).

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay \
  python3 tools/overlay/res00ba_rec_models_audit.py --output resources/res00ba-rec-audit/report.json
```
