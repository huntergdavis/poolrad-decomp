# Saved slot-A first view: current-wall dual placement

This note covers the next reached wall/object placement after the prior
mode-nine pair in unit `0x3BD` entry 5. The nonzero-current-wall branch
at `+0x0824..+0x0865` places a previous wall in local view 9 when present,
updates the frame's previous-wall byte, then places the current wall in
local view 0. The accompanying C model covers that bounded branch and its
local `+0x0211` placement handoffs. It does not translate all of entry 5,
the zero-current-wall `+0x0865` route, or the eventual display transfer.

The required `deja "Poolrad slot A u3bd entry5 after view9 +0795 next wall
object placement"` was run before decoding; the index returned no output
for about a minute, so the attempt was stopped. The earlier broad static
view query had no indexed match. Reused and checked the merged
[first-wall handoff](SLOT_A_U3BD_FIRST_WALL_TILE.md),
[entry-2 nibble lookup](U3BD_NIBBLE_ENTRY2.md), and
[tile-plane boundary](SLOT_A_U40C_FIRST_TILE_PLANE.md). The immediate
predecessor view-nine branch merged in PR #330.

## Instruction and caller evidence

Ignored `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x3BD` begins at file `0x2E8F7`. The half-open branch
`+0x0824..+0x0865` maps to file `[0x2F11B,0x2F15C)`: 65 bytes,
SHA-256 `23a4689e3551c41ef9e029f4870be70705cc261c672d55599262ceffb15b9fab`.
Capstone 5.0.7 decodes 30 aligned 16-bit instructions ending at `+0x0865`.
Both placement calls target the local `+0x0211..+0x031F` span, 270 bytes,
SHA-256 `b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.
That local helper reaches `+0x02FB`, the far tile call into
`0x428:002A`, for each positive in-grid item.

At `+0x0824`, a zero current-wall byte `[BP-0x14]` branches to
`+0x0865`; this remains outside the C model. A positive current wall
falls through. If previous-wall byte `[BP-0x17]` is positive,
`+0x0830..+0x0844` passes view index 9, that previous wall, y from
`DS:[0x0CD0]`, and x `DS:[0x0CBC]+[BP-0x12]-1` to `+0x0211`.
After return, `+0x0847..+0x084A` replaces the previous-wall byte with
the current wall. Then `+0x084D..+0x0860` passes view index 0,
current wall, y `DS:[0x0CBE]`, and x
`DS:[0x0CAA]+[BP-0x12]` to the same helper. The final `+0x0863`
branches onward to `+0x08BA`.

The private native replay reached `+0x02FB` ten times from the saved
slot-A first view. Calls 1..4 are the preceding branches; calls 5/6
return to `+0x0863` with the familiar `(7,6,1)` and `(7,7,13)` tile-grid
tuples while the previous-wall gate is skipped. At calls 7..10, the
entry-5 outer frame has front words `(13,3)`, lateral offset 2, and
previous/current walls `(14,14)`. View 9 has table `(offset,columns,rows)
=(154,1,2)` and origin `(5+2-1,4)=(6,4)`. View 0 has `(0,1,2)` and
origin `(5+2,4)=(7,4)`. The local helper adds two to each coordinate.

| Native call | Parent return | `(x,y,item,1,1)` |
| --- | --- | --- |
| 7 | `0x3BD:+0x0847` | `(8,6,1,1,1)` |
| 8 | `0x3BD:+0x0847` | `(8,7,13,1,1)` |
| 9 | `0x3BD:+0x0863` | `(9,6,1,1,1)` |
| 10 | `0x3BD:+0x0863` | `(9,7,13,1,1)` |

## Bounded parity and limits

The ignored full-RAM captures before calls 7, 8, 9 and 10 have SHA-256
`d46e855f3fa7f43c4f3724b90c7a7e65646725f69a9796991a7b7c965938b3cd`,
`7fef790047a394e909ec004dd4222d219fc4e8bd84c66419c75efcb1e6107cd8`,
`a5638a7c2346d1ce2846d7c5ced9aed6e321ca4191857950cce900f366d98b55`,
and `0b60c2f75c39a43406982f675dad12d71d977b075d564aec152fcb8742036da1`.
The exact `+0x0863` post-call capture has SHA-256
`ca09879f4ff33bcadc26fe39ceab3e84442a544ac47d2ddb16d30242c31683be`.

A private strict C probe supplies the captured branch frame and loaded
piece table to `pr_u3bd_entry5_dual_wall`: **4/4 callback tuples match**
the original stack frames. Passing each tuple and its pre-call object
through the already merged `pr_u40c_entry2_mode1_tile_plane` reproduces
the corresponding complete 14,135-byte destination object **4/4 times**.
Calls 7 and 9 change zero destination data bytes; calls 8 and 10 change
nine each. The latter changes lie at data offsets `0x1282..0x14CC` and
`0x1286..0x14D0`, respectively. These are offscreen object offsets;
the code has not mapped them to displayed pixels.

The public C state accepts the two DS view-table slices and already
loaded piece buffer from its host. It checks the selected 156-byte
wall slice and reached item bounds, preserves the old wall if a view-nine
hook fails before `+0x084A`, and updates it before view zero. Synthetic
tests cover the four-call order, distinct current-wall update, zero
current/previous gates, clipping before piece reads, zero items,
reached short-buffer and missing-hook failures, invalid table slices,
and low-byte origin narrowing. No original media or pixels are embedded.

## Reproduce the public gates

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
p = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
data = p[0x2F11B:0x2F15C]
print(len(data), sha256(data).hexdigest())
PY
cmake -S . -B resources/build-slot-a-dual-wall-p5 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-dual-wall-p5 \
  --target poolrad_overlay_u3bd_dual_wall_tests
ctest --test-dir resources/build-slot-a-dual-wall-p5 \
  -R '^poolrad_overlay_u3bd_dual_wall_tests$' --output-on-failure
```

All original executables, RAM captures and source art remain under ignored
`resources/`. This branch claims a bounded path only, with no full-C or
game-identity count change.
