# Resident retry display/input helper at `0x0802:0x0193`

Issue [#93](https://github.com/huntergdavis/poolrad-decomp/issues/93) covers
the resident helper at loaded image `0x081B3`. `pr_rec_res_0802_0193` is a
**bounded path-complete model**, not a strict full translation. It reproduces
the observed Pascal copy limit, display-command sequence, key-read position,
and return-byte source. The lower renderer and key routine retain global,
device, and runtime effects outside this model. No game-specific identity or
full game-C count is added: the helper's game versus linked-library authorship
is unresolved.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Mapping, calls, and frame

The hash-locked resident unpacker maps the `0x0802:0x0193` procedure to
loaded `0x081B3..0x0820C`, **90 contiguous copied bytes** from packed file
`0x081EA..0x08243`, with zero fill bytes. The SHA-256 of that private loaded
span is `03865df7d3a85b7cb3f9d71f8c0a7fb6a82fb8db3a0beea9568dd3cf92de11fc`.
Capstone 5.0.7 decodes all 90 bytes as 48 instructions: no branch, five
calls (one SYSTEM far copy, two local clear calls, one local text call, one
resident far key call), and one exit at `0x0820A RETF 6`.

The two raw near-call encodings to `0x081B3` are at loaded `0x082CA` and
`0x0886A`. Both are instruction-aligned in decoded parent paths:

| Caller | Argument evidence | Following effect |
| --- | --- | --- |
| Nested DAX open `0x082C1..0x082CA` | Push word code `0x000E`, then far address of its copied extension. | Rechecks open result and mode, and can retry. |
| ECL DAX block reader `0x0884B..0x0886A` | Push word code `0x000E`, build a far Pascal message through runtime calls, then call. | Calls resident fatal handler `0x0899:0x0000`. |

The helper's `[BP+6]` is the far Pascal message, `[BP+0x0A]` is a word code
slot read only as a byte, and `RETF 6` removes all six argument bytes. A
41-byte stack slot holds the capped string (`0x081B6..0x081C7`); one other
local byte holds the key AL but is never read (`0x081F4`). The caller does not
use the helper's AL on either decoded path. Nevertheless, its own final AL
comes from its **last display call**, not from the key read: the key AL is
stored at `0x081F4`, later argument setup writes AL, and `0x08195` returns
the AL left by `0x08060`.

## Decoded sequence and explicit boundaries

| Loaded addresses | Proven effect | Model operation |
| --- | --- | --- |
| `0x081B9..0x081C7` | Copy a Pascal string through `0x0AF8:0x0B25` with maximum length `0x28`. | Snapshot at most 40 bytes. |
| `0x081CC..0x081D9` | Call local `0x08195` with `(0, 0x18, 0, 0x28)`. It forwards those bytes and glyph `0xDB` to `0x08060`. | First `display_command(0,24,0,40,0xDB)`. |
| `0x081DC..0x081EC` | Call local `0x08125` with `(0, 0x18, code_low, &message)`. It copies at most `0xFF` bytes, then calls `0x08060` once per byte with increasing eight-bit first coordinate and count one. | One display command per copied byte, in order. |
| `0x081EF..0x081F4` | Far-call `0x0899:0x0059`, store returned AL in an unread local. | `read_key` callback; discard its byte. |
| `0x081F7..0x0820A` | Repeat the initial `0x08195` call; return through `RETF 6`. | Final display command supplies AL. |

`0x08195..0x081B2` is 30 contiguous copied bytes and 16 instructions, ending
in `RETF 8`. `0x08125..0x08194` is 112 contiguous copied bytes and 44
instructions, with a zero-length exit and a per-character loop, ending in
`RETF 0x0A`. The second copy's `0xFF` cap cannot further shorten this
helper's already capped 40-byte string. Its loop increments the first
coordinate after each command and does not append a terminator command.
The SYSTEM copy entry `0x0AF8:0x0B25` maps to image `0x0BAA5`:
`0x0BAB6..0x0BAC2` clamps the Pascal length to the low byte of the cap,
stores the resulting length, then copies that many payload bytes.

`0x08060` writes display globals and calls `0x0A32:0x000B` two or four
times depending on a global flag. Its complete device effects remain the
`display_command` callback. `0x0899:0x0059` starts at loaded `0x089E9` and
reads a key through `0x0A40:0x030F`, with additional state-dependent calls
and global writes; its effects remain `read_key`. Callback failure is a host
boundary status, not a DOS branch: prior callbacks remain observable and the
caller-supplied AL is unchanged. Only completed calls report the final
display callback's AL.

The merged [nested-open model](RESIDENT_DAX_NESTED_OPEN.md) already has a
`retry_display(ctx, code, extension)` callback. An adapter can call
`pr_rec_res_0802_0193` with exactly that copied extension and code, discarding
the returned AL as the DOS caller does. This change does not alter that
existing API or p9's file-open files. The ECL block-reader model's oversized
block path is separate and still terminates through its fatal boundary.

## Verification and provenance

Synthetic tests cover an empty string, a 43-byte string capped to 40,
per-byte order and coordinates, high-byte independence of the code word,
the fill arguments, discarded key byte versus returned AL, all five host
failure positions on a two-byte string, and invalid arguments. They contain
no DOS strings, executable bytes, media, or device output.

```sh
cmake -S . -B /tmp/poolrad-issue-93-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-93-build
ctest --test-dir /tmp/poolrad-issue-93-build --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR --near \
  --json /tmp/poolrad-issue-93-loader.json
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 - <<'PY'
import hashlib
from pathlib import Path
from resident_loader_map import unpack
image, origin, fill, _ = unpack(Path('resources/DOS/POOLRAD/START.EXE').read_bytes())
span = bytes(image[0x081B3:0x0820D])
assert len(span) == 90 and not any(fill[0x081B3:0x0820D])
assert origin[0x081B3] + 0x200 == 0x081EA
assert origin[0x0820C] + 0x200 == 0x08243
print(hashlib.sha256(span).hexdigest())
PY
```

Before implementation, `deja "poolrad RES_0802 0x081B3 retry display input
nested DAX open"` found no prior matching session, and the broader
`deja "DAX retry display"` found only the current assignment. Reused work:
the merged [nested-open investigation](RESIDENT_DAX_NESTED_OPEN.md) supplied
the first caller lead, the [resident loader map](RESIDENT_LOADER_MAP.md)
supplied the hash-locked unpacker, and the [callable ledger](resident_callable_ledger.csv)
supplied a cross-check for resident call counts. The copied span, both caller
sites, exact frame, local loop, key boundary, and return flow were checked
independently here. No private input or derived game content enters Git.
