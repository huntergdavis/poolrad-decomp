# Full C translation: resident character link flags predicate

`pr_rec_res_00ba_2e46` translates DOS v1.3 `START.EXE` routine
`RES_00BA_2E46`. Its loaded entry is `0x00BA:0x2E46`, expanded-image
offset `0x39E6`, and first packed source file offset `0x3A9C`. The
[hash-locked loader map](RESIDENT_LOADER_MAP.md) confirms 69 contiguous
copied bytes through the next prologue at `0x3A2B`.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Character role and callers

The reviewed [RES_00BA_2E03 translation](RESIDENT_CHARACTER_LINK_CHECK.md)
establishes the character `+0xCC/+0xCE` far link, linked byte `+0x2E`,
and its indexed DS table access from exact GAME.OVR character-state
callers. This routine near-calls that predicate with the same character
argument, then re-reads the same character link and linked byte to test
another byte in the same 16-byte DS table entry. GAME.OVR unit
`0x2F:+0x0D51..+0x0D67` and unit `0x31:+0x0DC1..+0x0DDD` call the
two resident predicates consecutively with the same character pointer.
That character-specific caller and callee chain establishes game role;
the linked object's type and the table bits' gameplay meanings remain
raw.

Eight fixup-backed far callers are unit `0x2F:+0x0D60` (GAME.OVR file
`0x0A567`), `0x2F:+0x0E6C` (`0x0A673`), `0x31:+0x0DD6`
(`0x0B996`), `0x31:+0x0ED7` (`0x0BA97`), `0x5E:+0x1A0B`
(`0x14206`), `0x5E:+0x2B9A` (`0x15395`), `0x5E:+0x2D4A`
(`0x15545`), and `0x5E:+0x30F1` (`0x158EC`). The [callable
ledger](resident_callable_ledger.csv) records these exact targets.

## Complete instruction path and read order

A Capstone 5.0.7 walk of the hash-checked expanded image finds **27
instruction starts**, direct branch edges `0x39F8→0x3A1E`,
`0x3A16→0x3A1E`, and `0x3A1C→0x3A22`, one direct near call at
`0x39F3→0x39A3` to the fully translated `RES_00BA_2E03`, no other
calls, and one `RETF 4` at `0x3A28`. The only argument is a character
far pointer at `[BP+6,+8]`; the result is `AL=0` or `1`. DOS writes no
memory on this path.

| Expanded-image span | Observed effect |
| --- | --- |
| `0x39EC..0x39F8` | Push the character pointer and call `RES_00BA_2E03`. Return zero immediately when its `AL` is zero. |
| `0x39FA..0x3A16` | If the callee returned one, read character link words `+0xCC/+0xCE` again through `LES`, read linked byte `+0x2E` again, and read DS byte `0x5590 + 16 × index`. |
| `0x3A12..0x3A2B` | Mask that DS byte with `0x14`; return one exactly when both masked bits are set, otherwise zero. |

The C routine calls the reviewed `2E03` C function, so its first path
retains the callee's separate link reads and table byte at `0x558E`.
If that returns one, the caller makes two more far-word reads, one
far-byte read, and one DS-byte read in DOS order. Field additions wrap
at 16 bits. A host callback failure leaves the output unchanged and is
not claimed as a DOS branch. Valid memory covers all decoded branches
and external effects; no unresolved callee remains.

## Reproduction and prior work

Synthetic tests cover callee null and table rejection, final mask
acceptance and both single-bit failures, changing link and index values
between the callee and caller reads, wrapped field offsets, maximum
byte index, callback failures at each read, and host argument failures.
No original character data is present.

```sh
cmake -S . -B /tmp/poolrad-issue-61-build -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-61-build
ctest --test-dir /tmp/poolrad-issue-61-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/resident_loader_map.py --exe resources/DOS/POOLRAD/START.EXE --ovr resources/DOS/POOLRAD/GAME.OVR --near --json /tmp/poolrad-issue-61-loader.json
```

Before implementation, `deja "RES_00BA_2E46 character link predicate
table 0x5590"` found no exact prior translation. Reused the merged
[character-link evidence](RESIDENT_CHARACTER_LINK_CHECK.md),
[callable coverage](RESIDENT_CALLABLE_COVERAGE.md), and [loader
map](RESIDENT_LOADER_MAP.md); independently checked this routine's
packed source, call, branch, and overlay caller addresses against the
hash-locked executables. No original media, save bytes, decoded text,
or executable bytes enter Git.

## Reproduce (original-byte checker)

`RES_00BA_2E46` is checked against the original bytes by the registered checker `tools/overlay/res00ba_rec_models_audit.py`,
through the reusable io adapter `tools/overlay/res00ba_io_adapter.py`. What it compares is described in
[RES_00BA_REC_MODELS_AUDIT.md](RES_00BA_REC_MODELS_AUDIT.md).

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay \
  python3 tools/overlay/res00ba_rec_models_audit.py --output resources/res00ba-rec-audit/report.json
```
