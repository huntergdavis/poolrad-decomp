# Saved slot-A post-loop selector branch

In one hash-pinned native DOS slot-A load, unit `0x6F` continued from
`2A8E:1A45` through seven far calls and the first subsequent control
decision. The final call at `+1A7E` passed `SS:3AB9` to resident selector
`0x0802:02EB`. Its loaded `1026:0334` store changed `DS:[49CE]` from
`0` to `103`; it returned AL=0 at `+1A83`. The test at `+1A85` did not
take the nonzero branch, and execution reached `+1B65` through the jump
at `+1A87`. The run stopped before executing `+1B65`. This is native
load-path evidence, not a one-Up route, C parity, an explanation for
the runtime result `103`, or a new game-routine count.

Before debugging, `deja "Poolrad slot A u6f 1A45 1A7E 1A83 1A85
selector branch native post loop"` found no indexed session. The
merged [second post-loop call watch](SLOT_A_NATIVE_POSTLOOP_SECOND_CALL.md)
supplied the `+1A45` boundary. The
[resident selector model](RESIDENT_DAX_SELECTOR_02EB.md) supplied the
candidate callee bounds and AL contract. This run independently checked
the loaded caller, argument, selector writer, return, and branch.

## Pinned inputs and code spans

All captures and memory images are ignored under
`resources/oracle-runs/slot-a-postloop-selector-branch-live-029`.
Unit `0x6F` starts at `GAME.OVR` file `0x1DFE6`, loaded at `2A8E:0000`.
The resident selector's original 107 bytes are at `START.EXE` file
`0x08342..0x083AC`, loaded at `1026:02EB..0355` in this run.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

| Inclusive span | Bytes / instructions | Original SHA-256 | Loaded SHA-256 | Changed bytes |
| --- | ---: | --- | --- | ---: |
| Overlay `+1A45..+1A89` | 69 / 24 | `9c6c1fb196a4d10c85fefa092ba7877016b5c784cb868396ebdf56960037dd22` | `2a80b19e8cbcd7f810546e00dfbc6760e76b28c3eb73d89355ee349068c9ff28` | 14 |
| Overlay setup/calls `+1A45..+1A7D` | 57 / 20 | `3554f50a2021f09513e1e931d6d34a7c18380186e6f8fb64cf7230d05b95714e` | `bcbae1b4638c10111f7d65167286bbf522d043f7493d589d91e9155b7c5a0110` | 12 |
| Selector call `+1A7E..+1A82` | 5 / 1 | `84087c84e89649ccb1e5d5618377e5395beca7fc4080f5718865da6f0e0fdd6b` | `8cb85b242ae034a616972aad3545d6f5fc95ee435692370a2512ade155f942bd` | 2 |
| AL test and branches `+1A83..+1A89` | 7 / 3 | `e5b12a33ad0b1396c90bf64e7d45c8a18e200229edc2ba7473b4c19b1568afef` | same | 0 |
| Resident selector `+02EB..+0355` | 107 / 43 | `141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474` | `a1bd874454a477dc56eb74bee8be51f5e77b17c782081440306e0ffb5213d2e2` | 10 |
| Resident writer `+0334..+0336` | 3 / 1 | `390361f8eb46a3bd47a38a0a5e6fce7ee26c17602fffb09d166b8d69cd58d567` | same | 0 |

The overlay's 14 changed bytes are the segment operands of seven far
calls. The original selector call targets `0802:02EB`; it targets
`1026:02EB` after loading. The 24-instruction overlay span contains
calls at `+1A45`, `+1A4A`, `+1A4F`, `+1A5E`, `+1A6F`, `+1A79`, and
`+1A7E`, followed by `OR AL,AL`, a conditional branch, and a
zero-result jump. These hashes recheck both original images and the
runtime copies; they do not assign effects to each intervening call.

## Live argument, writer, and branch

At `+1A45`, `DS:[49CE]=0`, `DS:[5D96]=449A:0008` (list head), and
`SS:[3BFD]=449A:0008` (caller local). At `+1A7E`, `DS:[49CE]` was still
zero. The selector's far argument was `1D16:3AB9`, retained at
`SS:37B4..37B7` after the preceding SYSTEM append. Its Pascal length
byte was 23. The pointer lies two bytes into the caller's `SS:3AB7`
local; this numeric alias is observed without publishing its contents.

The aggregated `+1A45 → +1A7E` RAM comparison has 244 changed bytes
in 25 contiguous ranges. One changed byte is at physical `0x46C`;
the others are within `0x200CC..0x20D47`. This interval includes seven
far calls and stack setup, so the comparison does not identify which
instruction changed each byte. Selected local length bytes changed
`SS:38B7` `11 → 19`, `SS:39B7` `67 → 8`, and `SS:3AB7` `67 → 14`.
The `67` values are pre-setup bytes, not validated prior strings.

After arming the resident stops at `+1A7E`, execution reached loaded
`1026:0334`, before `MOV DS:[49CE],AX`, and then `1026:0337` after it.
At the writer, the resident caller frame at `SS:BP=1D16:37AE` held
return `2A8E:1A83` at `[BP+2..+5]` and the far argument
`1D16:3AB9` at `[BP+6..+9]`. The 107-byte body model and this frame
agree on one far argument and `RETF 4`. The before/after RAM dumps differ
at exactly physical `0x193CE`: `DS:[49CE]` changed `0 → 103`, with its
high byte unchanged. No meaning is assigned to `103` beyond the
observed returned word.

The selector returned to `2A8E:1A83` with AL=0. `OR AL,AL` set ZF=1;
`JNE +1A8A` at `+1A85` was not taken. The next stops were `+1A87`
and its jump target `+1B65`. The complete RAM images at `+1A83` and
`+1B65` are identical. No branch through `+1A8A` or later node-link
read is claimed for this stop.

## Private snapshot hashes and reproduction

Each private RAM image is 655,360 bytes.

| Stop | SHA-256 |
| --- | --- |
| `+1A45` | `b3ec965c190642beff3a7dd4621c91e8abbac91105af474e06fe29c20faa70e6` |
| `+1A7E` | `39d5c1667be910dfc8970a2d2c5210da184c370b8727e30883aca6676a60abfc` |
| Resident `+0334`, pre-store | `5df831d282360c1c564de80a3625a144f21bea6c5096a8e21bc0a2f57797da90` |
| Resident `+0337`, post-store | `da6725225871a164086db70a491eb01da3e3912cdb1482881c0080c382b1671d` |
| `+1A83`, `+1B65` | `138745acd8c88365e43f8cfc42e79c82b975622f9c6fee8fe19b9030d633c119` |

Prepare a new ignored run from the pinned save with
`tools/oracle/prepare_run.sh`; launch the heavy DOSBox-X through
`tools/oracle/launch.sh`. At the load-slot prompt open the debugger
with Alt+Pause, arm `BP 2A8E:1A45` and `BP 2A8E:1A7E`, run, and
select `a`. At `+1A7E`, additionally arm `BP 1026:0334`,
`BP 1026:0337`, `BP 2A8E:1A83`, `BP 2A8E:1A85`,
`BP 2A8E:1A87`, `BP 2A8E:1A8A`, and `BP 2A8E:1B65`.
Use `EV CS IP DS ES DI SS SP BP AX BX CX DX FLAGS` and
`MEMDUMPBIN 0000 0000 A0000` at the sampled stops, moving each dump
under the ignored run directory before resuming. Stop at `+1B65`.

The following private assertions check hashes, the live alias and
frame, and the exact writer and branch-state RAM effects without
printing game content:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

root = Path('resources/oracle-runs/slot-a-postloop-selector-branch-live-029')
digest = lambda b: sha256(b).hexdigest()
read = lambda name: (root / name).read_bytes()
inputs = {
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for name, expected in inputs.items():
    b = read('POOLRAD/' + name)
    assert (len(b), digest(b)) == expected
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), digest(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
hashes = {
    '1a45': 'b3ec965c190642beff3a7dd4621c91e8abbac91105af474e06fe29c20faa70e6',
    '1a7e': '39d5c1667be910dfc8970a2d2c5210da184c370b8727e30883aca6676a60abfc',
    '0334': '5df831d282360c1c564de80a3625a144f21bea6c5096a8e21bc0a2f57797da90',
    '0337': 'da6725225871a164086db70a491eb01da3e3912cdb1482881c0080c382b1671d',
    '1a83': '138745acd8c88365e43f8cfc42e79c82b975622f9c6fee8fe19b9030d633c119',
    '1b65': '138745acd8c88365e43f8cfc42e79c82b975622f9c6fee8fe19b9030d633c119',
}
m = {k: read('ram-' + k + '.bin') for k in hashes}
assert all((len(m[k]), digest(m[k])) == (655360, h) for k, h in hashes.items())
addr = lambda seg, off: seg * 16 + off
word = lambda b, p: int.from_bytes(b[p:p + 2], 'little')
far = lambda b, seg, off: (word(b, addr(seg, off + 2)), word(b, addr(seg, off)))
assert all(far(b, 0x14A0, 0x5D96) == (0x449A, 8) for b in m.values())
assert all(far(b, 0x1D16, 0x3BFD) == (0x449A, 8) for b in m.values())
assert [word(m[k], addr(0x14A0, 0x49CE)) for k in m] == [0, 0, 0, 103, 103, 103]
assert far(m['1a7e'], 0x1D16, 0x37B4) == (0x1D16, 0x3AB9)
assert m['1a7e'][addr(0x1D16, 0x3AB9)] == 23
assert (word(m['0334'], addr(0x1D16, 0x37B0)), word(m['0334'], addr(0x1D16, 0x37B2))) == (0x1A83, 0x2A8E)
assert far(m['0334'], 0x1D16, 0x37B4) == (0x1D16, 0x3AB9)
assert [(m['1a45'][addr(0x1D16, o)], m['1a7e'][addr(0x1D16, o)]) for o in (0x38B7, 0x39B7, 0x3AB7)] == [(11, 19), (67, 8), (67, 14)]
game = read('POOLRAD/GAME.OVR')
exe = read('POOLRAD/START.EXE')
overlay_spans = (
    (0x1A45, 0x1A8A, '9c6c1fb196a4d10c85fefa092ba7877016b5c784cb868396ebdf56960037dd22', '2a80b19e8cbcd7f810546e00dfbc6760e76b28c3eb73d89355ee349068c9ff28', 14),
    (0x1A45, 0x1A7E, '3554f50a2021f09513e1e931d6d34a7c18380186e6f8fb64cf7230d05b95714e', 'bcbae1b4638c10111f7d65167286bbf522d043f7493d589d91e9155b7c5a0110', 12),
    (0x1A7E, 0x1A83, '84087c84e89649ccb1e5d5618377e5395beca7fc4080f5718865da6f0e0fdd6b', '8cb85b242ae034a616972aad3545d6f5fc95ee435692370a2512ade155f942bd', 2),
    (0x1A83, 0x1A8A, 'e5b12a33ad0b1396c90bf64e7d45c8a18e200229edc2ba7473b4c19b1568afef', 'e5b12a33ad0b1396c90bf64e7d45c8a18e200229edc2ba7473b4c19b1568afef', 0),
)
for lo, hi, original_hash, loaded_hash, changed in overlay_spans:
    original = game[0x1DFE6 + lo:0x1DFE6 + hi]
    loaded = m['1a83'][addr(0x2A8E, lo):addr(0x2A8E, hi)]
    assert (digest(original), digest(loaded)) == (original_hash, loaded_hash)
    assert sum(a != b for a, b in zip(original, loaded)) == changed
resident_spans = (
    (0x02EB, 0x0356, '141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474', 'a1bd874454a477dc56eb74bee8be51f5e77b17c782081440306e0ffb5213d2e2', 10),
    (0x0334, 0x0337, '390361f8eb46a3bd47a38a0a5e6fce7ee26c17602fffb09d166b8d69cd58d567', '390361f8eb46a3bd47a38a0a5e6fce7ee26c17602fffb09d166b8d69cd58d567', 0),
)
for lo, hi, original_hash, loaded_hash, changed in resident_spans:
    original = exe[0x08342 + lo - 0x02EB:0x08342 + hi - 0x02EB]
    loaded = m['0334'][addr(0x1026, lo):addr(0x1026, hi)]
    assert (digest(original), digest(loaded)) == (original_hash, loaded_hash)
    assert sum(a != b for a, b in zip(original, loaded)) == changed
diff = lambda a, b: [i for i, (x, y) in enumerate(zip(m[a], m[b])) if x != y]
setup_diff = diff('1a45', '1a7e')
assert len(setup_diff) == 244 and 0x46C in setup_diff
assert all(i == 0x46C or 0x200CC <= i <= 0x20D47 for i in setup_diff)
assert diff('0334', '0337') == [0x193CE]
assert diff('0337', '1a83') == [0x2090D]
assert m['1a83'] == m['1b65']
print('slot-A selector-branch private assertions passed')
PY
```
