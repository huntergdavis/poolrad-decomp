# Separately callable resident selector `0x0802:0x02EB`

This is the bounded model for [issue #95](https://github.com/huntergdavis/poolrad-decomp/issues/95).
The routine probes a temporary file record and returns a Boolean in AL. It is
also called from the nested helper modeled in [PR #91](https://github.com/huntergdavis/poolrad-decomp/pull/91),
but the independent overlay calls below establish a separate public entry.
SYSTEM file operations remain callbacks, so `pr_rec_res_0802_02eb` is a
**path-complete bounded model**, not a complete DOS I/O translation. Game
versus linked-library ownership is unresolved; no game-function identity or
full-C counter changes.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Loader and bounded body

The hash-locked `resident_loader_map.unpack` places resident
`0x0802:0x02EB` at loaded image `0x0830B`. Its body is exactly
`0x0830B..0x08375`: 107 contiguous copied bytes from START.EXE file
`0x08342..0x083AC`, with zero fill bytes. The next Pascal prologue starts at
`0x08376`. Independent Capstone 5 x86-16 decoding covers all 43 instructions
and 107 bytes: two conditional branches at `0x0835C` and `0x08362`, one
unconditional branch at `0x08366`, five SYSTEM far calls, and one exit at
`0x08373 RETF 4`. Both conditional targets and the unconditional target lie
inside the body. There is no call to issue #93's `0x081B3` retry helper.

| Loaded addresses | Effect |
| --- | --- |
| `0x08312..0x08320` | Read the one far Pascal-string pointer at `[BP+6]`, copy at most `0x28` bytes into local `[BP-0x2A]` through SYSTEM `0x0B25`. |
| `0x08325..0x08330` | Pass a private stack record and copied name to SYSTEM Assign `0x13E2`. |
| `0x08335..0x0833F` | Reset that record through SYSTEM `0x1410` with record size `0x80`. |
| `0x08344..0x0834A` | Close it through SYSTEM `0x1491`. |
| `0x0834F..0x08354` | Call SYSTEM IOResult `0x04A2`; store its full AX word at `DS:0x49CE`. |
| `0x08357..0x08373` | Return AL=1 only if that word is zero and the copied name has nonzero length; otherwise AL=0. `RETF 4` consumes the one far argument. |

Only AL is changed after IOResult: the completed return AH is the high byte
of IOResult's word. The code always executes Assign, Reset, Close, IOResult,
and the global write before choosing a return branch. It does not branch on
the earlier runtime calls' register results.

Packed SYSTEM entry checks are separate from the selector decode. At packed
image `0x0B60E`, `0xAF8:0x0B98` reads destination/source far pointers and
returns with `RETF 4`, retaining the destination on the caller's stack.
At packed image `0x0BE58`, Assign initializes the record and copies the name;
at `0x0BE86`, Reset enters the DOS open path; at `0x0BF07`, Close reaches DOS
close `AH=0x3E` for an open handle. IOResult at packed `0x0AF18` exchanges
the runtime error word with zero and returns the old word in AX. Their
exceptional and external effects are not modeled beyond named callbacks.

## Independent overlay callers and frame

All 15 calls are far calls to `0x0802:0x02EB` with a fixup on the segment word
in GAME.OVR unit `0x06F`; the table gives instruction addresses only. The
14 `append` rows immediately follow SYSTEM `0x0B98` and pass its retained
destination far pointer. The direct row pushes `SS:DI`. Every site tests
AL with `OR AL,AL` immediately after return; none uses AH at that point.
These call shapes agree with `[BP+6]` and `RETF 4` without relying on the
nested helper's frame.

| Unit `0x06F` function | Far call | GAME.OVR file offset | Argument setup |
| --- | --- | --- | --- |
| `+0x0010` | `+0x0041` | `0x1E027` | append retained destination |
| `+0x069E` | `+0x08F1` | `0x1E8D7` | append retained destination |
| `+0x069E` | `+0x09E6` | `0x1E9CC` | append retained destination |
| `+0x0AEC` | `+0x0CAA` | `0x1EC90` | append retained destination |
| `+0x0AEC` | `+0x0DAA` | `0x1ED90` | append retained destination |
| `+0x1602` | `+0x1656` | `0x1F63C` | append retained destination |
| `+0x1602` | `+0x1730` | `0x1F716` | append retained destination |
| `+0x1602` | `+0x19A8` | `0x1F98E` | append retained destination |
| `+0x1602` | `+0x1A7E` | `0x1FA64` | append retained destination |
| `+0x1602` | `+0x1AD6` | `0x1FABC` | append retained destination |
| `+0x1602` | `+0x1B2B` | `0x1FB11` | append retained destination |
| `+0x1D58` | `+0x21AC` | `0x20192` | append retained destination |
| `+0x1D58` | `+0x2201` | `0x201E7` | append retained destination |
| `+0x1D58` | `+0x2256` | `0x2023C` | append retained destination |
| `+0x22D5` | `+0x22F4` | `0x202DA` | direct local `SS:DI` push |

The nested helper independently pushes its local name at loaded
`0x082AC..0x082AD`, calls `0x0830B` at `0x082AE`, and saves AL at
`0x082B2`. The overlay sites span six public unit functions and use the
selector as a temporary file probe; their call/fixup evidence does not
establish who authored the resident routine.

## Model boundary and verification

The portable API accepts one synthetic Pascal string and callbacks for the
private record's Assign, Reset, Close, IOResult, and `DS:0x49CE` write. It
returns the complete AX word on completed paths. A host callback failure
stops the model, leaves output AX untouched, and retains prior callback
effects. That failure status is not a DOS branch. Far-memory aliasing,
runtime file exceptions, and actual DOS handles remain outside this model.

Synthetic tests cover nonempty success, empty-name failure, nonzero
IOResult including a high byte, the 40-byte copy cap, source mutation after
copy, callback order and record size, all five host callback failures, and
invalid arguments. The new API is standalone: the merged PR #91 model and
issue #93 display/input helper are unchanged; CMake only adds this source
and test target.

Reproduction uses ignored local inputs and writes no decoded media to Git:

```sh
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR --near \
  --json /tmp/poolrad-issue-95-loader.json
python3 tools/overlay/resident_inventory.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --json /tmp/poolrad-issue-95-inventory.json
cmake -S . -B /tmp/poolrad-issue-95-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-95-build
ctest --test-dir /tmp/poolrad-issue-95-build --output-on-failure
```

Before implementation, `deja "poolrad-decomp resident 0802:02EB 0830B
selector overlay callers issue 95 issue 91"` found no earlier match; the
short `deja "02EB"` query returned only this session's mention. Reused
leads from [PR #91](https://github.com/huntergdavis/poolrad-decomp/pull/91)
and its nested-open note, plus [PR #87](https://github.com/huntergdavis/poolrad-decomp/pull/87)
for the prior SYSTEM append interpretation. Hashes, loader span, body,
runtime entry behavior, 15 fixups, external stack frames, and AL uses were
rechecked directly against the ignored DOS files.
