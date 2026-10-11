# Nested resident DAX open helper at `0x0802:0x025D`

This is the DOS v1.3 helper for [issue #89](https://github.com/huntergdavis/poolrad-decomp/issues/89),
distinct from the adjacent file-open caller in [PR #87](https://github.com/huntergdavis/poolrad-decomp/pull/87).
`pr_rec_res_0802_025d` is a **path-complete bounded model**, not a strict
full translation. The local branch and selector rules are decoded, but file
record operations, runtime I/O side effects, and the retry display/input
remain named callback boundaries. The helper's game-authored versus linked
library origin remains unresolved; this branch adds no full game-C or
verified game-identity count.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Loader provenance, callers, and ABI

The hash-locked START.EXE unpacker maps loaded image `0x0827D` to resident
`0x0802:0x025D` and packed file `0x082B4`. The body spans loaded
`0x0827D..0x0830A`, **142 contiguous copied bytes** from packed file
`0x082B4..0x08341`, with no fill provenance. Capstone 5.0.7 decodes 63
instructions, five internal conditional branches, six calls (two local,
four SYSTEM far), and one exit at `0x08308 RETF 0x0E`.

The fixup-backed [resident inventory](resident_loader_ledger.csv) reports
five overlay far callers: unit `0x036:+0x05A1` and unit `0x06F` at
`+0x0C26`, `+0x0D0B`, `+0x0E09`, and `+0x17A9`. The
[PR #87 parent](https://github.com/huntergdavis/poolrad-decomp/pull/87)
adds a decoded near call at image `0x08679`, after pushing its mode byte and
first file-record far address at `0x0866F..0x08677`. The nested body itself
establishes the remaining frame positions and return size:

| Frame location | Meaning | Instruction evidence |
| --- | --- | --- |
| `[BP+0x10]` | Far Pascal candidate-name string | Copied into local `[BP-0x2A]` with cap `0x28`. |
| `[BP+0x0C]` | Far Pascal extension/error string | Copied into local `[BP-0x53]` with cap `0x28`. |
| `[BP+0x0A]` | Word-sized mode slot | Only its low byte is compared to zero. |
| `[BP+0x06]` | Far address of first file record | Passed to runtime Assign and Reset only on selector success. |

The parent file-open helper's runtime string operations retain the two
string arguments on its stack. Its explicit pushes supply the mode and file
record; `RETF 0x0E` consumes all 14 bytes. The code uses no output-size or
archive-content argument. The overlay caller roles remain unclassified; the
fixup-backed sites prove calls, not game authorship.

## Complete outer paths

| Loaded addresses | Observed effect | C boundary |
| --- | --- | --- |
| `0x08283..0x082A4` | Two `0xAF8:0x0B25` Pascal copies, each capped at 40 bytes. | Copy both synthetic length-prefixed strings into locals. |
| `0x082A9..0x082B2` | Call local selector `0x0830B` with the name local; retain its AL result. | Run the decoded selector rule and file callbacks below. |
| `0x082B5..0x082CA` | If AL is zero and the low mode byte is zero, call local display/input helper `0x081B3` with code `0x0E` and the extension local. | `retry_display` callback. |
| `0x082CD..0x082D7` | Recheck AL and low mode byte; loop to the selector if both are zero. | Retry, bounded by host `max_attempts`. |
| `0x082D9..0x082F7` | For nonzero AL, call runtime Assign `0xAF8:0x13E2` on the first file record and copied name, then Reset `0xAF8:0x1410` with record size one. | `assign(FIRST)` and `reset(FIRST, 1)` callbacks. |
| `0x082FC..0x0830A` | Return the saved selector result in AL, popping 14 argument bytes. | Completed AL is zero or one. |

The selector at `0x0830B..0x08375` is **107 contiguous copied bytes** at
file `0x08342..0x083AC`: 43 instructions, three internal branches, five
SYSTEM far calls, and `0x08373 RETF 4`. It is also a separately called
resident target with 15 fixup-backed overlay sites; this branch models its
behavior only as reached by the nested helper. It copies the candidate name
with cap 40, then calls runtime Assign on a temporary stack file record,
Reset with record size `0x80`, Close, and IOResult in that order. It writes
the returned IOResult word to `DS:0x49CE` at `0x08354`, then returns AL=1
**only if that word is zero and the copied name has nonzero length**. It
otherwise returns AL=0. The code ignores earlier runtime return values;
an I/O error is represented by the word returned from the IOResult callback,
and the later steps still occur.

The file operation roles are backed by decoded SYSTEM entry instructions:
Assign initializes a file record and copies a capped name; Reset enters
DOS file open; Close at loaded `0x0C411` reaches DOS close `AH=0x3E` when
appropriate; IOResult at `0x0B422` exchanges a runtime error word with
zero. Their complete exceptional and external effects, the private stack
record's identity, and the display/input helper `0x081B3` are not translated.
The model exposes a zero sentinel for that temporary record and passes the
caller's far address for the first record. Callback failure is a **host**
status that stops the model with AL unchanged and any prior effects retained.
After a completed failed attempt in mode zero, the model calls the display
boundary and retries. Reaching the configured retry limit reports a host
status instead of claiming that DOS returns.

## Verification and prior work

Synthetic tests cover successful open, nonzero mode failure, empty name with
zero IOResult, low-byte-only mode behavior, failure then success retry,
retry bound, 40-byte string caps, file-operation ordering and record sizes,
the global `DS:0x49CE` write, each callback-failure position, and invalid
host arguments. No game strings or executable bytes are embedded.

```sh
cmake -S . -B /tmp/poolrad-issue-89-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-89-build
ctest --test-dir /tmp/poolrad-issue-89-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR --near \
  --json /tmp/poolrad-issue-89-loader.json
```

`deja "poolrad RES_0802_025D 0x0827D nested DAX open helper"` found no prior
exact translation; the issue's broader query surfaced the assignment only.
The [merged loader map](RESIDENT_LOADER_MAP.md) supplied the unpacker,
[resident inventory](resident_loader_ledger.csv) supplied the five far-call
leads, and PR #87 supplied the adjacent caller as provisional context.
This work independently checked both hashes, the copied spans, branch and
call census, caller frame, selector rule, and SYSTEM file-operation entries.
The p9-owned file-open model files remain untouched. No original archive,
executable, save, game string, or decoded media enters Git.
