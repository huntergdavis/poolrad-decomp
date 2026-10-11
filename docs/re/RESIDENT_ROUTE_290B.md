# Resident route helper `RES_00BA:290B`

This note supports [issue #111](https://github.com/huntergdavis/poolrad-decomp/issues/111).
The routine is called from the merged `RES_00BA:27D9` LOAD route and from
other overlay sites. Its string construction and final output effects cross
unrecovered callee boundaries, so `pr_rec_res_00ba_290b` is a **path-complete
bounded model** of this local body, not a full C translation. The call graph
supports a route/display helper role; game-authored versus linked-library
ownership remains unresolved.

## Hash-locked body and frame

| Ignored DOS v1.3 input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

The audited loader maps `00BA:290B` to image `0x034AB`. The **complete body**
is image `0x034AB..0x036FD`, 595 bytes copied contiguously from START.EXE
file `0x0356F..0x037C1`, with no fill. Its span SHA-256 is
`197d6b93ad7761754f5cd680c5c16c12dccff53bcc6137764c09f16506172c78`.
The ledger's `0x0356F` is the packed **entry file offset**, not a body end.
The next exact Pascal prologue is `RES_00BA:2B5E` at image `0x036FE`.

Capstone 5 x86-16 linear decoding covers all 595 bytes in 238 instructions:
nine internal direct branches with instruction-boundary destinations, 33
calls, and `0x036FD RETF` without an immediate operand. The prologue reserves
`0x232` stack bytes. The body reads no `[BP+6]` or higher caller argument
slot, and its plain `RETF` performs no caller-argument stack cleanup. It does not
establish a stable final AX value. Direct writes are only to local stack
bytes; observable output and string changes are callee effects.

## Callers and decoded branches

The resident candidate-seeded path walk finds four direct near-call sites:

| Caller body | Call image site | Context |
| --- | --- | --- |
| `RES_00BA:27D9` | `0x0340F` | LOAD route mode 2 |
| `RES_00BA:27D9` | `0x03431` | LOAD route mode 4 |
| `RES_00BA:27D9` | `0x03453` | LOAD route mode 3 |
| `RES_00BA:2B5E` | `0x03716` | Separate resident procedure |

Each near caller pushes CS for the Pascal far return before `CALL 0x034AB`.
The hash-locked overlay inventory separately identifies seven far-call
instructions with fixup-backed target `00BA:290B`:

| Overlay unit | Call site | GAME.OVR file offset |
| --- | --- | --- |
| `0x01C` | `+0x2D3D` | `0x03A83` |
| `0x068` | `+0x0B9B` | `0x17266` |
| `0x068` | `+0x0D0E` | `0x173D9` |
| `0x068` | `+0x103C` | `0x17707` |
| `0x068` | `+0x1069` | `0x17734` |
| `0x06A` | `+0x0819` | `0x1804A` |
| `0x06A` | `+0x1FC4` | `0x197F5` |

The local branch predicates and reached effects are:

| Image site | Predicate and effect |
| --- | --- |
| `0x034C2..0x034F3` | Read `DS:49FA`; values above 1 replace local bytes from the `DS:49D2` far record at `+0x186/+0x188`. Independent rereads equal to 3 and 4 add 13 and 26 to the first byte modulo 256. |
| `0x03518..0x03541` | After SYSTEM `0x0B25`, local `[BP-5] < 2` reaches three string calls; other values skip them. |
| `0x03574..0x0359D` | After another SYSTEM `0x0B25`, local `[BP-8] < 2` reaches the analogous three calls. |
| `0x035A2..0x035EC` | Record word `+0x1F6 == 0` reaches the six-call coordinate construction path. |
| `0x0366D..0x036D1` | `DS:49F3 == 2` selects a three-call suffix. Otherwise bit 0 of the far record at `DS:49D6`, field `+0x594`, selects the other three-call suffix or none. |

The body reads `DS:6AAD`, `6AAE`, `49FA`, `6AAF`, and `49F3` at their
decoded sites. It reloads `DS:49D2` for each use of record bytes `+0x186`
and `+0x188` and words `+0x192`, `+0x190`, `+0x18E`, and `+0x1F6`;
`DS:49D6` supplies the optional record flag. The second near `0x01C13`
call receives `(10 * word[+0x190] + word[+0x18E]) mod 65536`. A byte at
`DS:6AAF` selects the far DS source offset `0x25A8 + 3 * byte`, wrapping as
a 16-bit offset. These reads happen after earlier callbacks, so the model
does not cache record fields or selectors.

## Callee boundary and verification

The 33 call instructions target near `0x01C13` twice, near `0x01BD7` twice,
SYSTEM `0x0B25` nine times, SYSTEM `0x0B0B` six times, SYSTEM `0x0B98`
twelve times, resident `0x03F1:0x0137` once, and resident
`0x0709:0x0626` once. The C callback records every reached image call site,
target, and logical push in source order. Far pointers to local stack slots,
CS literals, and the DS table remain symbolic; game text and string bytes
are not stored in Git. The host supplies the two post-`0x0B25` length bytes
used for branches and owns all callee-managed string and output effects.
Unknown high bytes of byte-valued AX pushes are not claimed.

Synthetic tests cover the minimal, rich, and mixed branches; independent
`DS:49FA` rereads; 8-bit and 16-bit wrapping; the two optional suffixes;
callsite and selected push arguments; each reached host failure; and missing
callbacks at first use. A host boundary failure stops before later reads or
calls and preserves earlier callback effects. The model does not establish
the exact contents of constructed strings, DOS output, or game/library
identity. No game-function or full-C count changes.

Before implementation, `deja "poolrad RES_00BA_290B 0x034AB issue 111 resident
near call body callers"` found no exact session. The shorter `deja "290B"`
returned the current PM address assignment and a prior Claude aggregate
call-graph count. The merged [27D9 evidence](RESIDENT_LOAD_ROUTE_27D9.md)
provided the three route caller leads. The loader provenance, full body,
branch/call walk, four near callers, and seven fixup-backed far callers were
checked again against the ignored hash-locked inputs.

Reproduce the address-only audits and portable tests with the ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/resident_loader_map.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR --near \
  --json resources/analysis/issue111/loader.json
PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/resident_inventory.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --ovr resources/DOS/POOLRAD/GAME.OVR \
  --json resources/analysis/issue111/inventory.json
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
from resident_loader_map import unpack
exe = Path('resources/DOS/POOLRAD/START.EXE').read_bytes()
image, origin, fill, _ = unpack(exe)
a, b, source = 0x34AB, 0x36FE, 0x356F
body = bytes(image[a:b])
assert body == exe[source:source + b - a]
assert all(origin[x] + 0x200 == source + x - a for x in range(a, b))
assert not any(fill[a:b])
assert sha256(body).hexdigest() == (
    '197d6b93ad7761754f5cd680c5c16c12dccff53bcc6137764c09f16506172c78')
PY
cmake -S . -B /tmp/poolrad-issue-111-build \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /tmp/poolrad-issue-111-build
ctest --test-dir /tmp/poolrad-issue-111-build --output-on-failure
```
