# Saved slot-A U3BD placement to tile-data effect

The saved first-view route reaches U3BD `+0x0860 CALL +0x0211`. A fresh
private replay captured that call, both reached `+0x02FB` tile callbacks and
their `+0x0300` returns, and the helper's `+0x0863` return in **one run**.
`pr_u3bd_0211_art_join` composes the reached mode-zero placement walk with
the merged group-zero, video-state-one tile-data model. It models neither
the tile callback's CPU/stack effects nor the later screen transfer. This
adds no full-C or identity credit.

Before debugging, bounded `deja "Poolrad U3BD entry5 saved slot A first view
art effect join native input ownership"` timed out. Reused and rechecked:
the [saved caller branch](SLOT_A_U3BD_JNE_080A.md) establishes the
`+0x0860` call handoff, the [first-wall placement audit](SLOT_A_U3BD_FIRST_WALL_TILE.md)
establishes local `+0x0211`'s mode-zero table and five-word tile ABI, and
the [tile-plane model](SLOT_A_U40C_FIRST_TILE_PLANE.md) establishes the
group-zero data effect. The six frames and byte comparisons below are new
same-run evidence; no older wall-callback RAM was paired with this run.

## Original span and reached path

Ignored DOS `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
U3BD starts at file `0x2E8F7`; local `+0x0211..+0x031F` is file
`[0x2EB08,0x2EC16)`, 270 bytes / 108 aligned 16-bit instructions,
SHA-256 `b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03`.
The loaded `2D14` span hashes
`3514808da1ae78c2476bc9eabafdef19a30cb66ee9f6cc31ee8ae05aea9e1f6a`;
only the far-call operand bytes at `+0x02FE/+0x02FF` differ after loader
relocation. The previous caller audit independently pinned original
`+0x080A..+0x0863` and its saved branches.

Private run `slot-a-u3bd-art-effect-p5-032` used the hash-verified copied
DOS files under ignored `resources/` and the network-isolated heavy
debugger. Its closed EV/dump transcript SHA-256 is
`5ee5e4eea81e528867a5539e8c2edef2476efe0b6807e22a33f479292b8a6d44`.
Each stop has all 14 recorded CPU fields and a 640-KiB RAM image:

| Stop | RAM SHA-256 | Changed bytes since previous stop |
| --- | --- | ---: |
| `2D14:+0860`, before local call | `6d724167613df028dc0a1d70368f18d356ea33e59efd8cd64ded9a6ffacf6639` | — |
| First `2D14:+02FB`, before far tile call | `bb427b275705cf61112d6c3265f7ec89d5804557cd701fc8ee3a41d39c88fe11` | 16 |
| First `2D14:+0300`, after tile return | `86f0057f914682f0f3e0fa73f402ef045b28531bf3bfbb4448b301dae426a438` | 13 |
| Second `2D14:+02FB`, before far tile call | `14e0c78b39eebcaf5ed41d6a5193530ef7b842ba20a9a77aeee4475154490649` | 4 |
| Second `2D14:+0300`, after tile return | `dbbed3185775c591e3d67c60319846c74396cc545bdf8a2c4707676f7cd38c10` | 9 |
| `2D14:+0863`, placement return | `d2601750ebcb98252bc38f31fc5a0e1fffd4c551151fd74de7f0de0fad2236c3` | 1 |

All inter-stop RAM differences are within the same stack page. Both tile
callbacks leave the destination object unchanged; this is a measured
zero-delta data effect for these two calls, not evidence that the tile
routine is generally a no-op. The `+0860` frame supplies wall 14,
origin `(5,4)`, and mode zero. The two `+02FB` stack frames give
`(x,y,item,1,1)=(7,6,1,1,1)` and `(7,7,13,1,1)`. The two callbacks return
to `+0300`, then local placement returns to `+0863`.

## C boundary and verification

The new C entry accepts caller-owned piece, source, mask, and destination
buffers. It applies the reached `+0x0211` mode-zero slice and clipping
rules, then invokes the merged `pr_u40c_entry2_mode1_tile_plane` for each
positive visible item. On the captured inputs, it reports two calls,
64 modeled plane-byte writes, zero changed plane bytes, and exact equality
with the native destination object after `+0863`. The replay audit also
hash-locks all six RAM images and the original/loaded code, parses the
recorded CPU stop sequence, checks the two call frames, and proves that no
native non-stack byte changed across the measured chain.

The C function does not model the original helper's stack/register state,
the far tile callback's metadata and CPU effects, alternate view/group/video
paths, or visible EGA output. Its host bounds reject unsupported spans
before a read; earlier completed tiles remain when a later tile fails.
Synthetic tests cover two changed tile bytes, a zero item, clipping before
piece access, a reached bounds failure, and a later unsupported item after
the first effect.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 tools/replay/u3bd_0211_art_join_audit.py
cmake -S . -B resources/build-u3bd-0211-art-join \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-u3bd-0211-art-join \
  --target poolrad_overlay_u3bd_0211_art_join_tests
ctest --test-dir resources/build-u3bd-0211-art-join \
  -R '^poolrad_overlay_u3bd_0211_art_join_tests$' --output-on-failure
```

Original art, saved data, RAM images, transcript, and screenshots remain
ignored under `resources/`.
