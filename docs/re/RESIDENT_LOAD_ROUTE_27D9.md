# Resident LOAD FILES/PIECES route callee `0x00BA:0x27D9`

This note supports [issue #103](https://github.com/huntergdavis/poolrad-decomp/issues/103).
`pr_rec_res_00ba_27d9` models the independently decoded local dispatch and
outgoing calls. Called resident and overlay routines remain explicit callback
boundaries. This is a **path-complete bounded model**, not a full translation
of those dependencies. The game's use of the routine is established by the
LOAD FILES/PIECES route, but game-authored versus linked-library ownership
is unresolved; no game-identity or full-C count changes.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Loader, boundary, and frame

The hash-locked loader places `RES_00BA:27D9` at loaded image `0x03379`.
Its entire body is `0x03379..0x03491`, **281 contiguous copied bytes** from
START.EXE file `0x0343D..0x03555`, with no fill provenance. It begins with a
Pascal frame and ends at `0x03491 RETF` without an immediate operand. The
body span SHA-256 is
`49a9f1f5878be7a80a6840d98a589e500580cdde6e1a7f2795f96fe465d6d241`.
The next Pascal prologue is at `0x034AB`; the intervening `0x03492..0x034AA`
is outside this body. Independent Capstone 5 x86-16 linear decoding covers
all 130 instructions and 281 bytes. The body has six conditional and five
unconditional internal branches, 22 call instructions, and one return.

It reads no `[BP+6]` or higher argument slot and does not pop caller
arguments. The 28 fixup-backed overlay callers pass no argument consumed by
this body. It has no final AX assignment; external register results are not
claimed as a return contract. Its direct DS reads are a selector byte at
`0x49F3`, bytes at `0x49FD` and `0x49FE` on one path, and words at `0x5D94`
then `0x5D92` on four paths. It directly writes no DS global. The pointer
`DS:0x6ABE` is passed on two paths. The Pascal literals at `CS:0x27D4`
and `CS:0x27D5` have measured lengths zero and three; their contents remain
outside Git.

## Decoded local paths and callee graph

The initial byte is loaded from `DS:0x49F3` at `0x0337F`. The comparisons
occur in order for values `0`, `1`, `2`, `4`, `3`, and `6`; all other values
fall through to the epilogue without a call. The table records push order;
for byte-valued pushes, only the low byte of AX is established by the body.

| Mode | Loaded span | Ordered calls and arguments |
| ---: | --- | --- |
| `0` | `0x03386..0x033AC` | SYSTEM `0xAF8:0x0B0B` copies the empty `CS:0x27D4` string; resident `0x03F1:0x02E8` receives that retained far destination and pushed low bytes `1, 1, 0x26, 0x16, 0, 0x0F`. |
| `1` | `0x033B3..0x033D5` | Near `0x032CA` gets byte `1`; after its return, read `DS:0x49FD` and `0x49FE`, pass them to overlay `0x03D0:0x003E`; then overlay `0x03D0:0x0039` gets `3, 3, 1`. |
| `2` | `0x033DC..0x03412` | Near `0x032CA` gets `1`; SYSTEM `0x0B0B` copies the three-byte `CS:0x27D5` string; overlay `0x03D0:0x002A` gets that string, `0x1D`, `0`, and `DS:0x6ABE`; after it returns, read and push words `DS:0x5D94`, `DS:0x5D92`, call near `0x01307`, then near `0x034AB`. |
| `4` | `0x03418..0x03434` | Near `0x032CA` gets `1`; overlay `0x03CA:0x0025` gets no arguments; read and push `DS:0x5D94`, `DS:0x5D92`, call near `0x01307`, then near `0x034AB`. |
| `3` | `0x0343A..0x03456` | As mode `4`, except near `0x032CA` gets `0`. |
| `6` | `0x0345C..0x0348B` | As mode `2`, except overlay `0x03D0:0x002A` gets byte `1` instead of `0x1D`, and near `0x034AB` is not called. |

The distinct outgoing entries are resident `0x03F1:0x02E8`, near
`0x032CA`, near `0x01307` (`RES_00BA:0767`), near `0x034AB`
(`RES_00BA:290B`), overlay `0x03D0:0x003E/0x0039/0x002A` (unit
`0x3B9`), overlay `0x03CA:0x0025` (unit `0x3B4`), and SYSTEM
`0xAF8:0x0B0B`. Near `0x032CA` is a separate procedure with a low-byte
`[BP+6]` test and `RETF 2`; it is not inlined. The destination-retaining
SYSTEM copy and all other called effects are separate from this body's
branch logic. The model's callback arguments expose low bytes of pushed AX
words and full words loaded from DS; it does not invent the former's high
bytes or downstream DOS behavior.

## Fixup-backed overlay callers

The hash-locked overlay inventory finds **28 far calls** to this resident
entry across eight units. Every listed call has a segment-word fixup and
targets `0x00BA:0x27D9`; the listed function is the containing Pascal
prologue. The two reached LOAD FILES/PIECES route sites are unit `0x01C`
`+0x0DD4` and the gameplay loop `+0x3378`. Their flag and mode conditions
are independently described in [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) and
[ECL_NEW_GAME_ROUTE.md](ECL_NEW_GAME_ROUTE.md).

| Unit | Containing prologue | Call site | GAME.OVR file |
| --- | --- | --- | --- |
| `0x01C` | `+0x0C74` | `+0x0DD4` | `0x01B1A` |
| `0x01C` | `+0x179B` | `+0x18EF` | `0x02635` |
| `0x01C` | `+0x2DD3` | `+0x2DF5` | `0x03B3B` |
| `0x01C` | `+0x2E10` | `+0x2E4F` | `0x03B95` |
| `0x01C` | `+0x32F8` | `+0x3378` | `0x040BE` |
| `0x01E` | `+0x09D7` | `+0x0C29` | `0x051A0` |
| `0x01E` | `+0x0D09` | `+0x0D14` | `0x0528B` |
| `0x01E` | `+0x0D09` | `+0x0F02` | `0x05479` |
| `0x01E` | `+0x0D09` | `+0x0F13` | `0x0548A` |
| `0x020` | `+0x0C06` | `+0x0D17` | `0x06295` |
| `0x020` | `+0x0D38` | `+0x0DA0` | `0x0631E` |
| `0x020` | `+0x0D38` | `+0x0E1D` | `0x0639B` |
| `0x020` | `+0x0ECD` | `+0x0EDE` | `0x0645C` |
| `0x023` | `+0x053E` | `+0x054A` | `0x0721E` |
| `0x023` | `+0x053E` | `+0x0768` | `0x0743C` |
| `0x023` | `+0x053E` | `+0x0779` | `0x0744D` |
| `0x06A` | `+0x04A1` | `+0x0565` | `0x17D96` |
| `0x06A` | `+0x090A` | `+0x0AB0` | `0x182E1` |
| `0x06A` | `+0x0B5B` | `+0x0E2E` | `0x1865F` |
| `0x06A` | `+0x0FBC` | `+0x13C8` | `0x18BF9` |
| `0x06A` | `+0x1C59` | `+0x1CF3` | `0x19524` |
| `0x06A` | `+0x1E6F` | `+0x1E96` | `0x196C7` |
| `0x075` | `+0x02A4` | `+0x0340` | `0x209DD` |
| `0x075` | `+0x02A4` | `+0x06AA` | `0x20D47` |
| `0x077` | `+0x0AB3` | `+0x0D15` | `0x21D4F` |
| `0x077` | `+0x1734` | `+0x1747` | `0x22781` |
| `0x077` | `+0x2156` | `+0x216A` | `0x231A4` |
| `0x086` | `+0x0A91` | `+0x0AE2` | `0x27AFA` |

## Verification and limits

Synthetic tests exercise modes `0, 1, 2, 3, 4, 6`, unhandled modes, the
post-callee timing of DS reads, exact callback order and low-byte/full-word
arguments, and host failure at every callback event. The model reads the
three private CS literal bytes through a callback; no game text, bytes, or
media enter source control. Callback failure is a host boundary status with
prior effects retained, not an extra DOS branch. Missing callbacks are checked
only when reached; unselected paths do not require their callbacks. CMake only registers this
new source and test; p5's `0x0802:0x094A` files are untouched.

The required preimplementation `deja "poolrad RES_00BA_27D9 0x03379 issue
103 callers loader body"` found no exact session. A short `deja "27D9"`
found earlier Claude route callsite leads; the two route sites were checked
again in the hash-locked overlay inventory. The loader mapping and prior
route notes were reused as leads, while hashes, contiguous body, instruction
walk, branch paths, call graph, frame, literals' lengths, and all 28 fixups
were checked directly against ignored DOS v1.3 inputs.

The address-only audits and portable tests can be reproduced with ignored
local inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR --near \
  --json /tmp/poolrad-issue-103-loader.json
PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/resident_inventory.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --json /tmp/poolrad-issue-103-inventory.json
cmake -S . -B /tmp/poolrad-issue-103-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-103-build
ctest --test-dir /tmp/poolrad-issue-103-build --output-on-failure
```
