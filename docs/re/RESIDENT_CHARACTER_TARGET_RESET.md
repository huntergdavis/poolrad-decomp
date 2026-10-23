# Full C translation: resident character target reset

`pr_rec_res_00ba_2637` translates DOS v1.3 `START.EXE`
`RES_00BA_2637` at `0x00BA:0x2637`. The hash-locked loader maps the
entry to expanded image `0x031D7`, with first packed source byte at
file `0x0329B`. All 70 instruction bytes through `RETF 4` have copy
provenance from file `0x0329B..0x032E0` and no fill provenance. The
next exact Pascal prologue is at image `0x03226`; the nine bytes after
this return and before that prologue are outside this routine. This
is a loader-proved mapping for the exact binary below.

| Ignored local input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Caller and game-role proof

The [callable ledger](resident_callable_ledger.csv) identifies 33
fixup-backed GAME.OVR far calls. I checked their target words and fixups
in the hash-matched overlay; the call sites group as follows (unit:local
offsets):

| Unit | Far-call sites | Count |
| --- | --- | ---: |
| `0x02F` | `+0370`, `+044F`, `+05EB`, `+0CA8`, `+0CD5`, `+0D00`, `+0FB1`, `+0FD5` | 8 |
| `0x031` | `+003D`, `+0123`, `+015E`, `+017D`, `+09A2`, `+0AE1`, `+0B14`, `+0E8B`, `+0F8D`, `+127C` | 10 |
| `0x038` | `+1348`, `+185F`, `+2C6B` | 3 |
| `0x05E` | `+0D12`, `+133E`, `+17E5`, `+1ABF`, `+24F6`, `+2D04`, `+3A78`, `+3B4B`, `+3C24` | 9 |
| `0x077` | `+1C08` | 1 |
| `0x086` | `+322F` | 1 |
| `0x09E` | `+0FB9` | 1 |

The separately decoded near call at expanded image `0x03233` enters
`0x031D7` from the next resident prologue at `0x03226`.

GAME.OVR unit `0x077:+0x1C00..+0x1C10` passes DS far pointer
`[0x5D92]/[0x5D94]`, the active character pointer established by the
[character trace](../formats/DOS_CHARACTER.md) and the reviewed
[unit 0x38 effect evidence](OVERLAY_UNIT38_EFFECT_ENTRIES.md). Unit
`0x038:+0x133F..+0x1352` passes its character-record argument and
tests the returned AL. The record's in-memory far slot at `+0x108`
is independently written and dereferenced by character-state paths
documented in those references. The four bytes cleared here belong
to the object reached through that slot. This is a character-state
routine used by game callers, not an identified Turbo Pascal library
primitive; **game-specific** is the recommended classification for PM
review. The linked object's type and the gameplay meaning of bytes
`+0`, `+3`, `+6`, and `+7` remain unresolved. This branch does not edit
the shared issue #24 ledgers or increase a count before review.

## Complete instruction behavior

A bounded Capstone 5.0.7 decode found **20 instruction starts / 70 bytes**,
zero calls, zero branches, and one `RETF 4` at `0x0321A`. The sole
argument is a far character-record pointer at `[BP+6,+8]`. There is no
null test or other conditional path in the DOS body.

| Expanded-image addresses | Observed effect |
| --- | --- |
| `0x031DD..0x031EA` | Follow record far words `+0x108/+0x10A`; write zero to target byte `+3`. |
| `0x031EA..0x031F6` | Follow those same record words again; write zero to target byte `+0`. |
| `0x031F6..0x03203` | Follow the record words again; write zero to target byte `+7`. |
| `0x03203..0x03210` | Follow the record words a fourth time; write zero to target byte `+6`. |
| `0x03210..0x0321D` | Store one in a stack-local byte, load it into AL, restore the frame, and far-return while popping four argument bytes. AH is not written. |

Each lookup is separate: an intervening target write can alter the
record link, and the next write then uses the new far pointer. Both
record-field and target-field additions wrap modulo 65536. The C API
keeps far-word reads and target-byte writes as ordered callbacks and
preserves the incoming AH in its AX output. Callback failures are host
outcomes outside the DOS path: the function stops without changing AX,
leaving any earlier successful writes in place. Successful callbacks
cover every instruction effect; no callee behavior is omitted.

## Verification and prior work

Synthetic tests cover write order, four independent link reloads with
retargeting after each write, preserved neighboring bytes, wrapped
record and target offsets, a partially zero far pointer, AH preservation,
all 12 callback-failure positions, and invalid host arguments. The
focused target and all 16 strict CTest targets pass on base `c52d1bd`.
The hash-locked loader audit with Capstone also completes.

```sh
cmake -S . -B /tmp/poolrad-issue66-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue66-build
ctest --test-dir /tmp/poolrad-issue66-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --near --json /tmp/poolrad-issue66-loader.json
```

Before implementation, `deja "poolrad RES_00BA_2637 resident record
0x108 four bytes return AL 1 issue 66"` found no prior routine
analysis; the broader `deja "0x2637 poolrad resident"` returned only
this assignment. The reviewed [loader map](RESIDENT_LOADER_MAP.md),
[callable coverage](RESIDENT_CALLABLE_COVERAGE.md), character trace,
and unit 0x38 effect evidence supplied address and role context; this
work independently checked the entry, provenance, 33 fixup-backed
calls, near call, full decode, and the cited overlay instructions.
No original executable, save, extracted text, media, or decoded bytes
enter Git.

## Reproduce (original-byte checker)

`RES_00BA_2637` is checked against the original bytes by the registered checker `tools/overlay/res00ba_rec_models_audit.py`,
through the reusable io adapter `tools/overlay/res00ba_io_adapter.py`. What it compares is described in
[RES_00BA_REC_MODELS_AUDIT.md](RES_00BA_REC_MODELS_AUDIT.md).

```sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay \
  python3 tools/overlay/res00ba_rec_models_audit.py --output resources/res00ba-rec-audit/report.json
```
