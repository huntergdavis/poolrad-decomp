# Saved slot-A piece replay gate at unit `0x6F:+1C08`

The native DOS v1.3 saved slot-A load reached `2A8E:1C08`, set its
local index to 1, copied two words from the actual window-0 pointer
into the first piece-table slot, and fell through the signed branch at
`+1C70` to the external call at `+1C94`. Its loaded stub resolved to
unit `0x3BD` entry 6 at `2D14:0E08`. This replay stopped at the
stub, before its body executed. No effect inside that callee, one-Up
route, C parity, or new routine count is claimed.

Before analysis, `deja "Poolrad saved slot A unit 6F 1C08 next call
return branch gate native"` found no indexed session. The merged
[preceding post-null trace](SLOT_A_NATIVE_POST_NULL_PHASE.md) supplied
the `+1C08` stop and window pointers. The existing
[unit `0x3BD` entry-6 note](U3BD_PIECES_ENTRY6.md) supplied the
callee identity; this run independently checked the live operand,
stub jump, and caller frame.

## Provenance and bounded code

The private full-RAM snapshots, numeric debugger transcript, game
media, and captures are ignored under
`resources/oracle-runs/slot-a-native-1c08-live-036`. An initial
private launch without a PTY could not open the heavy DOSBox-X
debugger; run 036 was a fresh prepared copy launched with a PTY.
The heavy emulator is 47,259,176 bytes, SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |

Unit `0x6F` starts at `GAME.OVR` file offset `0x1DFE6` and was
loaded at `2A8E:0000`. All bounds below are inclusive and aligned
to i8086 instructions. The counts are from a linear decode of each
exact span, checked against the live debugger stops. Only the segment
word in the far-call operand relocated.

| Unit span | File offset | Bytes / instructions | Original SHA-256 | Loaded SHA-256 | Changed bytes |
| --- | --- | ---: | --- | --- | ---: |
| `+1C08..+1C34` | `0x1FBEE..0x1FC1A` | 45 / 15 | `3f3d2c27c51b14716b35017f9af98cefa3d4cf92989ff870c5029cb791ad954a` | same | 0 |
| `+1C35..+1C38` | `0x1FC1B..0x1FC1E` | 4 / 1 | `f6342a950229859e57be5e94c55ce75b9755410b53bd8b9077ba6936a17eff67` | same | 0 |
| `+1C39..+1C5A` | `0x1FC1F..0x1FC40` | 34 / 12 | `81394a23fc6ad00f286dbd6c5b526af687442d2fd38efa17a9d0b3a3e8ebd46d` | same | 0 |
| `+1C5B..+1C5E` | `0x1FC41..0x1FC44` | 4 / 1 | `27090da1dbda72edf7f4401e63f310c258336251f0715faf1f99932988791dd5` | same | 0 |
| `+1C5F..+1C71` | `0x1FC45..0x1FC57` | 19 / 7 | `928d608b48ae825a8891a465fdb843b2f431e9507aef8661c908d77ca3960bfb` | same | 0 |
| `+1C72..+1C98` | `0x1FC58..0x1FC7E` | 39 / 15 | `ebe76ca48685cfafece2a4530168a7ff53dc88cc3173b15464ad05f92044c467` | `a2648ebc38177926726a44516b00301dd24ecd47f8deb223a9884058c0f05b9a` | 2 |
| Whole `+1C08..+1C98` | `0x1FBEE..0x1FC7E` | 145 / 51 | `08b7e4c11777e4b286049d404f733cfa45d6c97dc8acd2dac182c655222eb974` | `bd2f155fe96ceea56b777e67c77996daf887cc088a37c31f65cd023d1a3cd391` | 2 |

## Live stop sequence and numeric effects

The actual window-0 pointer `DS:[49D2]` was `3996:0000`,
`DS=14A0`, `SS=1D16`, and `BP=3DCA`. At entry the local byte
`SS:[BP-01C9]` was 6. `+1C08` wrote 1 there; the `+1C0D`
jump skipped the increment at `+1C0F`. The entry-to-`+1C35`
RAM difference is exactly physical byte `0x20D61`, that local.

For index 1, the wrapped effective source addresses in window 0
were `+0x03F4` and `+0x03FA`; their words were 2 and 1. The
destination words were `DS:[6AB2]` and `DS:[6AB4]`.
At `+1C35` the first destination was 0. Its store changed only
physical byte `0x1B4B2`, yielding word 2 at `+1C39`.
At `+1C5B`, the second destination was already 1; its store
left the entire RAM image unchanged at `+1C5F`. The RAM images
at `+1C39`, `+1C5B`, `+1C5F`, and `+1C70` are identical.

At `+1C70`, the preceding comparison tested signed word 2
against zero. `FLAGS=7202` (`SF=OF=0`), so `JL +1C99` was
untaken. The direct next stop was `+1C94`, after two argument
pushes. The `+1C70` to `+1C94` RAM difference is exactly three
stack bytes at physical `0x20914..0x20916`. At `+1C94`,
`SS:SP=1D16:37B4` with words `0001,0002` at `SS:[37B4..37B7]`.
The far-call operand is `03D5:003E` in the original overlay and
`0BF9:003E` in loaded RAM. The stub at `0BF9:003E` jumped to
`2D14:0E08`, the documented unit `0x3BD` entry 6. At stub entry
`SS:SP=1D16:37B0`; the four frame words were return IP `1C99`,
return CS `2A8E`, second-pushed slot 1, first-pushed id 2. The
call instruction changed only four stack bytes between the
`+1C94` and stub stops. The guest was stopped before executing
the stub jump. Actual window-0 time words `+0x18E/+0x190/+0x192`
remained `(0,5,10)` at every recorded stop.

| Direct stop | `CS:IP` | `SS:SP` | `AX,DX,FLAGS` | Full-RAM SHA-256 |
| --- | --- | --- | --- | --- |
| Entry | `2A8E:1C08` | `1D16:37B8` | `0014,0000,7246` | `031bd7ec3240ab7fc8c9866c258cb99ebc023d05e25b3f7c2813dc8ccfe6ebdf` |
| First store before | `2A8E:1C35` | `1D16:37B8` | `0001,0002,7212` | `a3bc6e8eae477adae496aa63b081da543d683ee036ddb21f99fe41ad52e9099f` |
| First store after | `2A8E:1C39` | `1D16:37B8` | `0001,0002,7212` | `c40221bcfb1cbd27c80d41abbe8bd5fde266a6564d3db56a25380dedbc5fa8b1` |
| Second store before | `2A8E:1C5B` | `1D16:37B8` | `0001,0001,7212` | same as `+1C39` |
| Second store after | `2A8E:1C5F` | `1D16:37B8` | `0001,0001,7212` | same as `+1C39` |
| Signed branch | `2A8E:1C70` | `1D16:37B8` | `0001,0001,7202` | same as `+1C39` |
| Far call before | `2A8E:1C94` | `1D16:37B4` | `0001,0001,7212` | `45df76c428f9bb3316a4d7174309e19a1cdf63721cfe21f52e16f43c4c134a88` |
| Far stub entry | `0BF9:003E` | `1D16:37B0` | `0001,0001,7212` | `e7219ca9ece5541165f96e0e1b5fe946e0570546c2d20c72f97e7fff694ae10e` |

## Private reproduction and assertions

Prepare a fresh ignored run with `tools/oracle/prepare_run.sh` and
launch the pinned heavy DOSBox-X through `tools/oracle/launch.sh`
inside `xvfb-run`, with a **PTY** attached to its process. At the
title/load menu, enter the debugger with Alt+Pause, arm
`BP 2A8E:1C08`, resume with F5, and select slot A using Enter,
`l`, `a`. At `+1C08`, capture `EV CS IP DS ES DI SS SP BP AX BX CX DX FLAGS`
and `MEMDUMPBIN 0000 0000 A0000`; move the generated
`MEMDUMP.BIN` to a distinct ignored filename before resuming.
Then arm `+1C35`, `+1C39`, `+1C5B`, `+1C5F`, `+1C70`,
`+1C94`, `+1C99`, and the loaded stub `0BF9:003E`.
Resume with F5 and capture each reached stop; stop at the stub.
The private `ev-numeric.tsv` records the eight reached stop tuples.

This local check reads the ignored files but publishes only its pass
message:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

r = Path('resources/oracle-runs/slot-a-native-1c08-live-036')
read = lambda p: (r / p).read_bytes()
h = lambda b: sha256(b).hexdigest()
a = lambda s, o: s * 16 + o
w = lambda b, p: int.from_bytes(b[p:p + 2], 'little')
inputs = {
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for name, expected in inputs.items():
    b = read('POOLRAD/' + name)
    assert (len(b), h(b)) == expected
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), h(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
hashes = {
    '1c08': '031bd7ec3240ab7fc8c9866c258cb99ebc023d05e25b3f7c2813dc8ccfe6ebdf',
    '1c35': 'a3bc6e8eae477adae496aa63b081da543d683ee036ddb21f99fe41ad52e9099f',
    '1c39': 'c40221bcfb1cbd27c80d41abbe8bd5fde266a6564d3db56a25380dedbc5fa8b1',
    '1c5b': 'c40221bcfb1cbd27c80d41abbe8bd5fde266a6564d3db56a25380dedbc5fa8b1',
    '1c5f': 'c40221bcfb1cbd27c80d41abbe8bd5fde266a6564d3db56a25380dedbc5fa8b1',
    '1c70': 'c40221bcfb1cbd27c80d41abbe8bd5fde266a6564d3db56a25380dedbc5fa8b1',
    '1c94': '45df76c428f9bb3316a4d7174309e19a1cdf63721cfe21f52e16f43c4c134a88',
    'far-stub': 'e7219ca9ece5541165f96e0e1b5fe946e0570546c2d20c72f97e7fff694ae10e',
}
ram = {}
for name, digest in hashes.items():
    b = read('ram-' + name + '.bin')
    assert len(b) == 655360 and h(b) == digest, name
    assert tuple(w(b, a(0x3996, o)) for o in (0x18E, 0x190, 0x192)) == (0, 5, 10)
    ram[name] = b
ev = [line.split() for line in read('ev-numeric.tsv').decode().splitlines()]
assert ev[0] == 'label CS IP DS ES DI SS SP BP AX BX CX DX FLAGS'.split()
assert len(ev) == 9 and all(len(row) == 14 for row in ev)
assert [(int(row[1], 16), int(row[2], 16)) for row in ev[1:]] == [
    (0x2A8E, n) for n in (0x1C08, 0x1C35, 0x1C39, 0x1C5B, 0x1C5F, 0x1C70, 0x1C94)
] + [(0x0BF9, 0x003E)]
assert [int(row[7], 16) for row in ev[1:]] == [0x37B8] * 6 + [0x37B4, 0x37B0]
assert int(ev[6][13], 16) == 0x7202
game = read('POOLRAD/GAME.OVR')
loaded = ram['1c08']
spans = [
    (0x1C08, 0x1C35, '3f3d2c27c51b14716b35017f9af98cefa3d4cf92989ff870c5029cb791ad954a', None, 0),
    (0x1C35, 0x1C39, 'f6342a950229859e57be5e94c55ce75b9755410b53bd8b9077ba6936a17eff67', None, 0),
    (0x1C39, 0x1C5B, '81394a23fc6ad00f286dbd6c5b526af687442d2fd38efa17a9d0b3a3e8ebd46d', None, 0),
    (0x1C5B, 0x1C5F, '27090da1dbda72edf7f4401e63f310c258336251f0715faf1f99932988791dd5', None, 0),
    (0x1C5F, 0x1C72, '928d608b48ae825a8891a465fdb843b2f431e9507aef8661c908d77ca3960bfb', None, 0),
    (0x1C72, 0x1C99, 'ebe76ca48685cfafece2a4530168a7ff53dc88cc3173b15464ad05f92044c467', 'a2648ebc38177926726a44516b00301dd24ecd47f8deb223a9884058c0f05b9a', 2),
    (0x1C08, 0x1C99, '08b7e4c11777e4b286049d404f733cfa45d6c97dc8acd2dac182c655222eb974', 'bd2f155fe96ceea56b777e67c77996daf887cc088a37c31f65cd023d1a3cd391', 2),
]
for lo, hi, original, runtime, changed in spans:
    src = game[0x1DFE6 + lo:0x1DFE6 + hi]
    dst = loaded[a(0x2A8E, lo):a(0x2A8E, hi)]
    assert (h(src), h(dst), sum(x != y for x, y in zip(src, dst))) == (original, runtime or original, changed)
changes = lambda x, y: [i for i, (v, z) in enumerate(zip(ram[x], ram[y])) if v != z]
assert changes('1c08', '1c35') == [0x20D61]
assert changes('1c35', '1c39') == [0x1B4B2]
assert all(not changes(x, y) for x, y in [('1c39','1c5b'),('1c5b','1c5f'),('1c5f','1c70')])
assert changes('1c70', '1c94') == [0x20914, 0x20915, 0x20916]
assert changes('1c94', 'far-stub') == [0x20910, 0x20911, 0x20912, 0x20913]
assert w(ram['1c08'], a(0x14A0, 0x49D4)) == 0x3996
assert w(ram['1c08'], a(0x14A0, 0x49D2)) == 0
assert (w(ram['1c08'], a(0x3996, 0x03F4)), w(ram['1c08'], a(0x3996, 0x03FA))) == (2, 1)
assert (w(ram['1c08'], a(0x14A0, 0x6AB2)), w(ram['1c08'], a(0x14A0, 0x6AB4))) == (0, 1)
assert (w(ram['1c39'], a(0x14A0, 0x6AB2)), w(ram['1c5f'], a(0x14A0, 0x6AB4))) == (2, 1)
assert [w(ram['1c94'], a(0x1D16, o)) for o in (0x37B4, 0x37B6)] == [1, 2]
assert [w(ram['far-stub'], a(0x1D16, o)) for o in (0x37B0, 0x37B2, 0x37B4, 0x37B6)] == [0x1C99, 0x2A8E, 1, 2]
print('slot-A +1C08 private assertions passed')
PY
```
