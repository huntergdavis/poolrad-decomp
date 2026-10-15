# Saved slot-A post-Up `2D14:02FB` first two tile calls

**Classification:** two reached, ordinal-matched calls into the existing
entry-2 tile-plane model. The C model matches the entire destination object
and dirty metadata at each direct return. This is not a complete dispatcher,
resident compositor, CPU, view, or displayed-frame translation; no full-C
count follows from it.

Before debugging, `deja "Poolrad slot A post-Up 2D14 02FB tile handoff 02FB
0300 U3BD view native offscreen resident 2632"` was run with a 25-second
bound and returned no reusable result. The checked-in
[view-four callback-four note](SLOT_A_U3BD_VIEW4_TILE04.md),
[group-three tile note](SLOT_A_U40C_GROUP3_TILE_PLANE.md), and
[post-Up row-59 note](SLOT_A_POST_UP_OFFSCREEN_01BE_GATE.md) supplied
address and ABI leads. This investigation independently captured both reached
post-Up calls; the earlier view-four call at the same address is a different
frame.

## Pinned images and bounded code

Ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

| Original span | File range | Size / aligned instructions | SHA-256 |
| --- | --- | --- | --- |
| unit `0x3BD:+02FB..+0300`, far call to unit `0x428:002A` | `GAME.OVR [0x2EBF2,0x2EBF7)` | 5 bytes / 1 | `64940f6941110590c5125977fe8e435e68b57a1fc5b998a494f76b176f0050ec` |
| unit `0x40C` entry 2 | `GAME.OVR [0x32392,0x324E2)` | 336 bytes / 127 | `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00` |
| resident compositor `+08D4..+0D71` | `START.EXE [0x4F4D,0x53EA)` | 1,181 bytes / 429 | `9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe` |
| resident changed-byte store `+0C6A..+0C6E` | `START.EXE [0x52E3,0x52E7)` | 4 bytes / 1 | `f1798d82c73cbb1d10f18e8d3d5f89265a778c5e2ea9cdfaca48e0dd4d68ec90` |

The loaded caller `2D14:02FB..0300` is one relocated far call, SHA-256
`cca89ed9a112f415a0e76a1464e43fa7a8c7d6bb20c7265c54018bd62666141f`,
and reaches loaded trampoline `0C4C:002A..002F`, a far jump to
`223B:015D`, SHA-256
`220f267469c1dcc65dbbc52e8a07f848d2fa416762b3f9cac48093ca46811bae`.
At original dispatcher `+0213`, the group-zero remap word is subtracted
from the one-based item, then `+0249` calls original resident
`0464:08D4`. The bounded C model already exists as
`pr_u40c_entry2_mode1_tile_plane`; this change adds a reached two-call
synthetic and private-pair verification, without editing that implementation.

## Same-run native call order

One hash-pinned DOSBox-X replay loaded visible `(15,4,N; 10:50)`, then sent
exactly one Up. `BPLIST` showed the sole caller breakpoint at `2D14:02FB`
for each pre-call stop and the sole direct-return breakpoint at `2D14:0300`
for its matching return. The first `02FB` stop must be paired with the
first `0300` stop, and likewise for the second. A preliminary comparison
crossed these ordinals; all results below use the corrected pairs.

At both pre-call stops, `DS=14A0`, `SS:SP=1D16:3EA2`, `BP=3EBA`.
Stack words at `SP+0,+2,+4,+6,+8` are:

| Ordinal | Stack words | Effective item index | First data offset | Immediate return |
| --- | --- | ---: | ---: | --- |
| 1 | `(1,1,1,5,6)` | 0 | 3384 | `2D14:0300`, `SS:SP=1D16:3EAC` |
| 2 | `(1,1,18,6,6)` | 17 | 4056 | `2D14:0300`, `SS:SP=1D16:3EAC` |

At both returns, `DS=14A0`, `BP=3EBA`; `ES` changes from `3D0E` before
the call to `3DE4` afterward. The measured `AX,BX,CX,DX,SI,DI` tuples
are respectively `(1,3E78,2,0,3E92,0892)` to
`(7,0,0,00FF,3E92,005E)` for call 1, and
`(1,0,3,0,3E92,0893)` to `(7,0,0,1,3E92,006E)` for call 2.
Only the second return's FLAGS were explicitly sampled (`0x7246`); these
register observations do not establish whole-CPU C parity.

The ignored 640 KiB RAM captures are pinned by SHA-256:

| Ordinal | Pre-call RAM | Direct-return RAM | RAM value changes | Object value changes |
| --- | --- | --- | ---: | ---: |
| 1 | `e0bc785c960e31dd3fd2a72a763d56848a5d3c73e0563ebe604742879558fd2a` | `89ca7e7265a87d78019814ea8785792e745aa09c63ef896e1518bcdde6b641ad` | 42 | 0 |
| 2 | `37be0af83142c96cffe255c394c41f8a8e0fb956c9494bb5ee7996bc6075e31f` | `0c01f56f3c3c60fdccc6d43551e5519a03c878004990873e8ef90e13e07a3494` | 23 | 14 |

The destination far pointer `3DE4:0008` resolves to a 14,135-byte
object. Call 1 leaves its SHA-256
`4e92a157485951c88f0ef9e151bd2083d90ff28b9f2f090180d4274334f688b0`
unchanged. Call 2 starts from that hash and returns
`bf038de10ab4a98444bde1b3234bd9b14ee6b24dff311c2280e95a3eb421bf04`.
Dirty rows remain 16..103 (88 rows). The 1,176-byte dirty-metadata span
`DS:4A28..4EBF` has no value changes in either call. These are **value
change counts, not write counts**.

An independent native write watch on the second call caught the first changed
object byte at physical `3DE4:104D` (object offset 4165, row 49, column 6,
plane 2), changing `0` to `128`. The guest stopped at `0C88:0C6E` just after
the original store at `+0C6A`; the nested stack return is `223B:024E`, and
the outer return is `2D14:0300`. Its 640 KiB RAM SHA-256 is
`484342f239c3553b9d448cd61221779394d9737b119a465f58c3a7951b1b3eb2`.
Only one object byte differed from the fresh second pre-call snapshot at
that watch. This pins the first **changed** byte, not necessarily the first
write.

## Bounded C/native result and reproduction

The new synthetic test exercises the two reached stack tuples with an
8-row one-column source and auxiliary mask. Call 1 performs 32 modeled
plane-byte writes with zero changed destination bytes. Call 2 performs 32
modeled writes and changes 14 bytes across two planes. A second, optional
private mode reads the four ignored RAM captures, derives source, auxiliary,
and destination far pointers from pre-call RAM, and checks the **entire
14,135/14,135 destination object** and **1,176/1,176 dirty-metadata bytes**
against each matching native return. Both pairs pass. It does not compare
the transient stack or other guest RAM.

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
root = Path('resources/DOS/POOLRAD')
for name, lo, hi in (('GAME.OVR', 0x2EBF2, 0x2EBF7),
                     ('GAME.OVR', 0x32392, 0x324E2),
                     ('START.EXE', 0x4F4D, 0x53EA),
                     ('START.EXE', 0x52E3, 0x52E7)):
    span = (root / name).read_bytes()[lo:hi]
    print(name, hex(lo), hex(hi), len(span), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-02fb-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-02fb-p9 --target \
  poolrad_slot_a_post_up_02fb_two_tile_calls_tests
ctest --test-dir resources/build-post-up-02fb-p9 \
  -R '^poolrad_slot_a_post_up_02fb_two_tile_calls_tests$' --output-on-failure
resources/build-post-up-02fb-p9/poolrad_slot_a_post_up_02fb_two_tile_calls_tests \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/02fb-call1-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/02fb-call1-return-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/02fb-call2-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/02fb-call2-return-ram-private.bin
```

Original media, save, RAM, emulator trace, and captures remain ignored under
`resources/`. This gate ends at the second direct `0300` return; later page
composition and displayed output are outside it.
