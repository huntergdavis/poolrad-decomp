# Saved slot-A post-loop second call

A fresh hash-pinned native DOS slot-A load continued from unit `0x6F`
`2A8E:1A23` to the next consequential call at `+0x1A40` and stopped at
its return, `+0x1A45`. The 15 setup instructions before that call only
changed stack argument bytes. The live call to SYSTEM `0x0B0B` copied a
Pascal object with a one-byte payload from `CS:15E7` into local
`SS:3BB7`, replacing its prior length byte `46` with `1`. The two-byte
destination object then matched the source. The next resident call at
`+0x1A45` was not executed.
This establishes one native load-path boundary, not a one-Up route, C
parity, or a new game-routine count.

Before debugging, `deja "Poolrad u6F 1A23 next consequential call write
boundary saved slot A native post loop"` found no indexed session. The
merged [first post-loop call watch](SLOT_A_NATIVE_POSTLOOP_FIRST_CALL.md)
provided the `+1A23` stop and SYSTEM-copy lead; this run independently
checked its own call frame, original/loaded span hashes, and RAM effects.

## Pinned input and code

The fresh private run is ignored under
`resources/oracle-runs/slot-a-postloop-next-boundary-live-028`.
Unit `0x6F` starts at `GAME.OVR` file `0x1DFE6`, loaded at `2A8E:0000`.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

| Inclusive unit span | Bytes / instructions | Original SHA-256 | Loaded SHA-256 | Changed bytes |
| --- | ---: | --- | --- | ---: |
| `+0x1A23..+0x1A44` | 34 / 16 | `d53269b08e11b25a13dce221d79a90c946c3675dd4cd31e94dda85868f908ce2` | `eef029065ad55046f9c40fbd7d1e7149ab2361508e6dea29808a3d5f2f591dbb` | 2 |
| Argument setup `+0x1A23..+0x1A3F` | 29 / 15 | `e0ef8f4ee846e89644aa28c085dd6014597961ab26c4c632c7dc3edf1e5c9f84` | same | 0 |
| Far call `+0x1A40..+0x1A44` | 5 / 1 | `7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae` | `1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b` | 2 |

The two changed bytes are the far-call segment relocation: original
`0AF8:0B0B`, loaded `131C:0B0B`. The setup is three local-address
calculations, one far-pointer load from the head local, one immediate
address load, and ten pointer-word pushes. No conditional branch occurs
in this 34-byte span.

## Live frame and return effect

At all three stops, `DS:[5D96]` and caller local `SS:[3BFD]` held the
head `449A:0008`. At `+1A40`, the caller stack from `SS:37A0` held these
six far pointers, top first:

| Stack words | Far pointer | Role established by setup |
| --- | --- | --- |
| `37A0..37A3` | `2A8E:15E7` | Source Pascal object, length 1 |
| `37A4..37A7` | `1D16:3BB7` | Destination local |
| `37A8..37AB` | `449A:0008` | Head record |
| `37AC..37AF` | `1D16:3AB7` | Earlier local |
| `37B0..37B3` | `1D16:39B7` | Earlier local |
| `37B4..37B7` | `1D16:38B7` | Retained destination from prior call |

From `+1A23` to `+1A40`, 19 RAM bytes changed, all in caller stack
ranges physical `0x20900..0x20908` and `0x2090A..0x20913`. From the
pre-call `+1A40` dump to the return at `+1A45`, only six bytes changed:
four call/return stack bytes at `0x208FC..0x208FF` and two destination
bytes at `0x20D17..0x20D18`. The destination length changed `46 → 1`;
its length and one data byte equal the two source bytes after return.
The source's 256-byte window did not change. The earlier local pointers
and head pointer retained their values. This is a measured effect for this
input, not a claim about every SYSTEM-copy case.

## Private snapshot hashes and reproduction

Each private RAM image is 655,360 bytes.

| Stop | SHA-256 |
| --- | --- |
| `+1A23` | `6ab931a2ed8b899596fe66645eb4509b78388d77566bdb40eb5d502e78ff2d27` |
| `+1A40` | `43ef04f5367cb354c85cd42c91516ec171f34416bf494adaf78174382b0749db` |
| `+1A45` | `2d035644f50d89c8e5bb6e397b7e465a7d1a6e9ff3ad14ef263a1cc4f4ac399f` |

Prepare a fresh ignored run with `tools/oracle/prepare_run.sh` from the
same save, and launch it with `tools/oracle/launch.sh` and the pinned heavy
DOSBox-X on `PATH`. At the load-slot prompt, open the debugger with
Alt+Pause, arm `BP 2A8E:1A23`, `BP 2A8E:1A40`, and `BP 2A8E:1A45`, then
run and select slot `a`. At each reached stop, use `EV CS IP DS ES DI SS
SP BP AX BX CX DX FLAGS` and `MEMDUMPBIN 0000 0000 A0000`; move each
dump into the ignored run directory before resuming. Stop at `+1A45`
before executing its resident call.

The following private assertions verify the hashes and numeric effects
without printing game content:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

root = Path('resources/oracle-runs/slot-a-postloop-next-boundary-live-028')
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
    '1a23': '6ab931a2ed8b899596fe66645eb4509b78388d77566bdb40eb5d502e78ff2d27',
    '1a40': '43ef04f5367cb354c85cd42c91516ec171f34416bf494adaf78174382b0749db',
    '1a45': '2d035644f50d89c8e5bb6e397b7e465a7d1a6e9ff3ad14ef263a1cc4f4ac399f',
}
m = {k: read('ram-' + k + '.bin') for k in hashes}
assert all((len(m[k]), digest(m[k])) == (655360, h) for k, h in hashes.items())
addr = lambda seg, off: seg * 16 + off
word = lambda b, p: int.from_bytes(b[p:p + 2], 'little')
far = lambda b, seg, off: (word(b, addr(seg, off + 2)), word(b, addr(seg, off)))
assert all(far(b, 0x14A0, 0x5D96) == (0x449A, 8) for b in m.values())
assert all(far(b, 0x1D16, 0x3BFD) == (0x449A, 8) for b in m.values())
frame = [(0x2A8E, 0x15E7), (0x1D16, 0x3BB7), (0x449A, 8),
         (0x1D16, 0x3AB7), (0x1D16, 0x39B7), (0x1D16, 0x38B7)]
assert [far(m['1a40'], 0x1D16, off) for off in range(0x37A0, 0x37B8, 4)] == frame
game = read('POOLRAD/GAME.OVR')
spans = (
    (0x1A23, 0x1A45, 'd53269b08e11b25a13dce221d79a90c946c3675dd4cd31e94dda85868f908ce2', 'eef029065ad55046f9c40fbd7d1e7149ab2361508e6dea29808a3d5f2f591dbb', 2),
    (0x1A23, 0x1A40, 'e0ef8f4ee846e89644aa28c085dd6014597961ab26c4c632c7dc3edf1e5c9f84', 'e0ef8f4ee846e89644aa28c085dd6014597961ab26c4c632c7dc3edf1e5c9f84', 0),
    (0x1A40, 0x1A45, '7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae', '1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b', 2),
)
for lo, hi, original_hash, loaded_hash, changed in spans:
    original = game[0x1DFE6 + lo:0x1DFE6 + hi]
    loaded = m['1a40'][addr(0x2A8E, lo):addr(0x2A8E, hi)]
    assert (digest(original), digest(loaded)) == (original_hash, loaded_hash)
    assert sum(a != b for a, b in zip(original, loaded)) == changed
assert (word(game, 0x1DFE6 + 0x1A43), word(game, 0x1DFE6 + 0x1A41)) == (0x0AF8, 0x0B0B)
assert (word(m['1a40'], addr(0x2A8E, 0x1A43)), word(m['1a40'], addr(0x2A8E, 0x1A41))) == (0x131C, 0x0B0B)
diff = lambda a, b: [i for i, (x, y) in enumerate(zip(m[a], m[b])) if x != y]
assert diff('1a23', '1a40') == list(range(0x20900, 0x20909)) + list(range(0x2090A, 0x20914))
assert diff('1a40', '1a45') == list(range(0x208FC, 0x20900)) + [0x20D17, 0x20D18]
src = addr(0x2A8E, 0x15E7)
dst = addr(0x1D16, 0x3BB7)
assert (m['1a40'][src], m['1a40'][dst], m['1a45'][dst]) == (1, 46, 1)
assert m['1a40'][src:src + 256] == m['1a45'][src:src + 256]
assert m['1a45'][dst:dst + 2] == m['1a40'][src:src + 2]
print('slot-A post-loop second-call private assertions passed')
PY
```
