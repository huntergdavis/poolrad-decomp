# Saved slot-A post-loop first call

In one hash-pinned native DOS slot-A load, unit `0x6F` left its six-record
party loop, replaced the caller's local record pointer with the list head,
took the nonzero-head branch at `+0x1A0E`, and reached its first subsequent
far call at `+0x1A1E`. That SYSTEM `0x0B0B` call copied an 11-byte
Pascal object from `DS:537A` into the caller's empty `SS:38B7` local. The
return stop was `+0x1A23`. This is a bounded load-path observation. It does
not establish a one-Up route, later calls, a C replay, or a new game-routine
count.

Before this watch, `deja "Poolrad slot A u6f post loop 19FE 1A0E 1A13
1A1E native first call pointer"` found no indexed session. The merged
[six-slot watch](SLOT_A_NATIVE_PARTY_LOOP_EXIT.md) established the equality
exit and `+0x19FE` entry state. The existing
[filename gate audit](SLOT_A_PARTY_FILENAME_GATE_AUDIT.md) identifies SYSTEM
`0x0AF8:0x0B0B` as an uncapped Pascal copy at an earlier call site. This
watch independently checked the present call target, stack arguments, and
before/after RAM.

## Pinned private inputs and instruction span

All private images and dumps are ignored under
`resources/oracle-runs/slot-a-postloop-first-call-live-027`. Unit `0x6F`
starts at `GAME.OVR` file `0x1DFE6` and was loaded at `2A8E:0000`.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

| Inclusive unit span | Bytes / instructions | Original SHA-256 | Loaded SHA-256 | Changed bytes |
| --- | ---: | --- | --- | ---: |
| `+0x19F7..+0x1A22` | 44 / 15 | `75b52fb90b97e81dce6a5cb7e51fc6968b1bba399a370bd9061f27d4f7156431` | `d9204537420919c92058ca3cde9cbee3d001dad22d9b8462a30c79c83cf156b5` | 2 |
| Local stores and head gate `+0x19FE..+0x1A12` | 21 / 6 | `4903211a89691a5cbe72340e5728448e6a2345ed01501415daf157b1cde8c18d` | same | 0 |
| First far call `+0x1A1E..+0x1A22` | 5 / 1 | `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae` | `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b` | 2 |

The two changed bytes are the far call's loaded segment: original
`0AF8:0B0B`, loaded `131C:0B0B`. The `44`-byte span decodes as 15
instructions: two head-pointer reads, two local stores, one local-word read,
OR with the other local word, conditional branch, unconditional zero-head
jump, one destination address calculation, five argument-setup instructions,
and the far call.
The zero-head jump at `+0x1A10` was not executed in this run.

## Stops and observed effects

At `+0x19F7`, the sixth slot remained current (`SS:[3C01]=6`), with
`DS:[5D96]=449A:0008` as the head and `DS:[5D92]=4529:0008` as the
last active record. `SS:[3BFD]` initially held `4529:0008`.

| Stop before instruction | Registers and local state | Consequence at next stop |
| --- | --- | --- |
| `2A8E:19FE` | `AX=0008`, `DX=449A`, `SS:BP=1D16:3DCA`, `SP=37B8`; local `SS:[3BFD]=4529:0008` | The low-word store re-wrote `0008`; RAM unchanged at `+1A02`. |
| `2A8E:1A02` | local still `4529:0008` | The high-word store changed local to `449A:0008`; exactly two RAM bytes changed by `+1A06`. |
| `2A8E:1A0E` | `AX=449A`, `FLAGS=7206` (`ZF=0`) | Conditional branch reached `+1A13`; `+1A10` was skipped. |
| `2A8E:1A1E` | `SP=37B0`, four argument words at `SS:37B0..37B7`: `537A,14A0,38B7,1D16` | Far runtime call returned at `+1A23`, `SP=37B4`. |
| `2A8E:1A23` | `AX=440B`, `DX=14A0`, `FLAGS=7246` | Destination local now has Pascal length 11. |

The call frame supplies source `DS:537A` and destination `SS:38B7`. In
the private snapshots, source length was 11 and destination length was 0
before the call. At `+1A23`, the destination's length and 11 following
bytes equal the source's corresponding 12 bytes; the source's 256-byte
window is unchanged. Across the call, 16 total RAM bytes changed: four at
physical `0x2090C..0x2090F` (call/return stack) and 12 at
`0x20A17..0x20A22` (destination). This observed equality supports the
copy effect for this input; it does not prove every runtime case.

## Private snapshots and reproduction

Each dump is 655,360 bytes. The hash at `+19F7`, `+19FE`, and `+1A02`
is identical, corroborating that the first store wrote an unchanged word.

| Stop | SHA-256 |
| --- | --- |
| `+19F7`, `+19FE`, `+1A02` | `83b960bfa4e456a5a60fb4bf294cb9fb79dc353b3cf501606fdf1e86380b9caa` |
| `+1A06` | `13da6bee258efa74a65cdb615673a41e72923b37dce4a03cd81b6d9c0c314e72` |
| `+1A1E` | `199f4da69a5449ef659bc43ebb139f3aad192208b8a02215b86f2397c2958a00` |
| `+1A23` | `a2c0939f9a6dc263be941b45164b90dcc269cd5fe0d60b59df10511611c7227d` |

Prepare a fresh ignored run from the same save using
`tools/oracle/prepare_run.sh`; start it with `tools/oracle/launch.sh` and
the pinned heavy DOSBox-X on `PATH`. At the load-slot prompt, open the
debugger with Alt+Pause and arm `BP 2A8E:19F7`, `BP 2A8E:19FE`,
`BP 2A8E:1A02`, `BP 2A8E:1A06`, `BP 2A8E:1A0E`, `BP 2A8E:1A10`,
`BP 2A8E:1A13`, `BP 2A8E:1A1E`, and `BP 2A8E:1A23`. Run and select slot
`a`. At each reached stop, use `EV CS IP DS ES DI SS SP BP AX BX CX DX
FLAGS` and `MEMDUMPBIN 0000 0000 A0000`, moving each dump into the
ignored run directory before resuming. The observed stop order after
`+19F7` was `+19FE`, `+1A02`, `+1A06`, `+1A0E`, `+1A13`, `+1A1E`,
`+1A23`.

The following assertions verify the private hashes, target, frame, branch
input, local change, and copy effect without printing game content:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

root = Path('resources/oracle-runs/slot-a-postloop-first-call-live-027')
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
    '19f7': '83b960bfa4e456a5a60fb4bf294cb9fb79dc353b3cf501606fdf1e86380b9caa',
    '19fe': '83b960bfa4e456a5a60fb4bf294cb9fb79dc353b3cf501606fdf1e86380b9caa',
    '1a02': '83b960bfa4e456a5a60fb4bf294cb9fb79dc353b3cf501606fdf1e86380b9caa',
    '1a06': '13da6bee258efa74a65cdb615673a41e72923b37dce4a03cd81b6d9c0c314e72',
    '1a1e': '199f4da69a5449ef659bc43ebb139f3aad192208b8a02215b86f2397c2958a00',
    '1a23': 'a2c0939f9a6dc263be941b45164b90dcc269cd5fe0d60b59df10511611c7227d',
}
m = {k: read('ram-' + k + '.bin') for k in hashes}
assert all((len(m[k]), digest(m[k])) == (655360, v) for k, v in hashes.items())
word = lambda b, p: int.from_bytes(b[p:p + 2], 'little')
addr = lambda seg, off: seg * 16 + off
far = lambda b, seg, off: (word(b, addr(seg, off + 2)), word(b, addr(seg, off)))
assert all(word(m[k], addr(0x1D16, 0x3C01)) & 0xff == 6 for k in m)
assert all(far(m[k], 0x14A0, 0x5D96) == (0x449A, 8) for k in m)
assert all(far(m[k], 0x14A0, 0x5D92) == (0x4529, 8) for k in m)
assert all(far(m[k], 0x1D16, 0x3BFD) == (0x4529, 8) for k in ('19f7', '19fe', '1a02'))
assert all(far(m[k], 0x1D16, 0x3BFD) == (0x449A, 8) for k in ('1a06', '1a1e', '1a23'))
assert [word(m['1a1e'], addr(0x1D16, off)) for off in range(0x37B0, 0x37B8, 2)] == [0x537A, 0x14A0, 0x38B7, 0x1D16]
game = read('POOLRAD/GAME.OVR')
spans = (
    (0x19F7, 0x1A23, '75b52fb90b97e81dce6a5cb7e51fc6968b1bba399a370bd9061f27d4f7156431', 'd9204537420919c92058ca3cde9cbee3d001dad22d9b8462a30c79c83cf156b5', 2),
    (0x19FE, 0x1A13, '4903211a89691a5cbe72340e5728448e6a2345ed01501415daf157b1cde8c18d', '4903211a89691a5cbe72340e5728448e6a2345ed01501415daf157b1cde8c18d', 0),
    (0x1A1E, 0x1A23, '7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae', '1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b', 2),
)
for lo, hi, original_hash, loaded_hash, changed in spans:
    original = game[0x1DFE6 + lo:0x1DFE6 + hi]
    loaded = m['1a1e'][addr(0x2A8E, lo):addr(0x2A8E, hi)]
    assert (digest(original), digest(loaded)) == (original_hash, loaded_hash)
    assert sum(a != b for a, b in zip(original, loaded)) == changed
assert (word(game, 0x1DFE6 + 0x1A21), word(game, 0x1DFE6 + 0x1A1F)) == (0x0AF8, 0x0B0B)
assert (word(m['1a1e'], addr(0x2A8E, 0x1A21)), word(m['1a1e'], addr(0x2A8E, 0x1A1F))) == (0x131C, 0x0B0B)
assert m['19fe'] == m['1a02']
diff = lambda a, b: [i for i, (x, y) in enumerate(zip(m[a], m[b])) if x != y]
assert diff('1a02', '1a06') == [0x20D5F, 0x20D60]
assert diff('1a06', '1a1e') == list(range(0x20910, 0x20918))
assert diff('1a1e', '1a23') == list(range(0x2090C, 0x20910)) + list(range(0x20A17, 0x20A23))
src = addr(0x14A0, 0x537A)
dst = addr(0x1D16, 0x38B7)
assert (m['1a1e'][src], m['1a1e'][dst], m['1a23'][dst]) == (11, 0, 11)
assert m['1a1e'][src:src + 256] == m['1a23'][src:src + 256]
assert m['1a23'][dst:dst + 12] == m['1a1e'][src:src + 12]
print('slot-A post-loop first-call private assertions passed')
PY
```
