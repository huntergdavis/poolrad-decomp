# Saved slot-A second party insertion

In one native DOS replay of the hash-pinned slot-A save, the second party
slot reached unit `0x6F:+0x19B1`, allocated a record, returned from the
record loader and insertion calls, returned from the post-insertion call,
and reached the second `+0x19EA` loop gate. At that final stop the current
slot byte was `2` and the copied party count was `6`. The replay stopped
before this gate's comparison and before a third slot. It establishes no
one-Up or C-native parity result.

Before debugging, `deja "Poolrad saved slot A second slot 19B1 19DA 19E3
19E6 19EA allocation record insertion native"` found no indexed session.
The merged [first-loop native watch](SLOT_A_NATIVE_19EA_FIRST_LOOP.md)
provided the prior reached `+0x19B1` address and comparison run. The
merged [continuation map](U6F_PARTY_CONTINUATIONS.md) provided code-span
and callee leads; this replay checked the original and loaded span hashes
and observed each named return. The [first insertion watch](SLOT_A_NATIVE_19E3_INSERTION_RETURN.md)
provided the `+0x104/+0x106` list-link and window-1 `+0x067C` address
leads, checked here against the live RAM snapshots.

## Inputs and instruction boundaries

The private save, game images, emulator executable, and RAM dumps remain
under ignored `resources/`. No contents or extracted game bytes are in
this repository. The private prepared run was
`resources/oracle-runs/slot-a-second-slot-live-024`; its input hashes were:

| Input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Saved slot-A `SAVGAMA.DAT` | 13,137 bytes | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Private heavy DOSBox-X executable | 47,259,176 bytes | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

Unit `0x6F` begins at `GAME.OVR` file offset `0x1DFE6` and was loaded at
`2A8E:0000`. Instruction counts and code bounds are from the cited static
continuation map; the hashes below were recomputed from this run's private
original image and live RAM. The spans include embedded data where that
map identifies it, so their byte totals are not routine instruction counts.

| Span | Bytes / executable instructions | Original SHA-256 | Loaded SHA-256 |
| --- | ---: | --- | --- |
| Caller `+0x19B1..+0x19E9`, file `0x1F997..0x1F9CF` | 57 / 22 | `76347194a33838c597b3fba609cb594dcd17a78bf4d74883330276032344a9c5` | `7d004b43fc63f8a7b130244c05a3f204b131bfdaa1cb0dea4567e56c21c6cfef` |
| Record loader `+0x0AEC..+0x0EC3`, file `0x1EAD2..0x1EEA9` | 984 / 370 | `715424407210de3824c3a0218cc29033f04bb357ec718aa24a7995d9452d1e36` | `4da70731d0fe4afe0387c8df693aea3420b8c9c1916be920e79f16e04ad41ccc` |
| Insertion `+0x1446..+0x157E`, file `0x1F42C..0x1F564` | 313 / 99 | `370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245` | `cd7d4b58fff401796a53d4779c4c55e5a731215f5eecf16f304a25b4b48a9dad` |
| Post-insertion `+0x02AE..+0x032E`, file `0x1E294..0x1E314` | 129 / 51 | `ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539` | `70114e09285ff85b5c6176903f5d5640e427bed0ea52b41786aa19793c2629d7` |
| Gate `+0x19EA..+0x19F6`, file `0x1F9D0..0x1F9DC` | 13 / 4 | `f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4` | `f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4` |

Relative to the original, the loaded caller, record-loader, insertion and
post-insertion spans have respectively 2, 94, 6 and 16 changed bytes. The
gate has none. Loaded far-call operands are relocated; the private hash
checks bind the actual loaded code without publishing those bytes.

## One-run debugger stops

At the visible load-slot prompt, `BP 2A8E:19EA` was armed. After selecting
`a`, the debugger stopped at the first `+0x19EA`, where slot index was
`1` and copied count was `6`. `BP 2A8E:19B1` then reached the second
slot's pre-allocation boundary. Breakpoints at `+0x19BA`, `+0x19D7`,
`+0x19DA`, `+0x19E3`, and `+0x19E6` stopped at each call boundary; the
initial `+0x19EA` breakpoint stopped at the second gate. Every row below
is before its named instruction. `CS=2A8E`, `DS=14A0`, `SS=1D16`,
`BP=3DCA`, and `SP=37B8` except where noted. Register values are hex.

| Stop | `SP`; `AX`; `DX` | Observed boundary |
| --- | --- | --- |
| First `+0x19EA` | `37B8`; `0005`; `3526` | Slot `1`, count `6`; baseline after the first insertion. |
| Second slot `+0x19B1` | `37B8`; `0001`; `3734` | Before `PUSH 0x011D` and allocator call. |
| `+0x19BA` | `37B8`; `0008`; `44BB` | SYSTEM allocation returned `DX:AX=44BB:0008`. |
| `+0x19D7` | `37AA`; `0001`; `44BB` | Before record-loader call `+0x0AEC`; stack words at `SS:37AA..37B7` were `2A8E,0001,0001,0008,44BB,3BD4,1D16`. |
| `+0x19DA` | `37B8`; `3E01`; `0000` | Record loader returned; caller local pointer `SS:[3BFD]=44BB:0008`. |
| `+0x19E3` | `37B2`; `3E01`; `0000` | Before insertion call `+0x1446`; stack words `SS:37B2..37B7` were `2A8E,0008,44BB`. |
| `+0x19E6` | `37B8`; `0007`; `000C` | Insertion returned. |
| Second `+0x19EA` | `37B8`; `0005`; `3526` | Post-insertion `+0x02AE` returned; slot `2`, count `6`. |

The `+0x19D7` and `+0x19E3` stack words show the caller's actual pushed
arguments. Return stops establish that these callees completed on this
run; they do not by themselves establish each internal file or DOS effect.

## Numeric RAM effects

The snapshots have identical `SS:[3BB8]=6` copied count,
`SS:[3BD1]=6` source count, `DS:[49CE]=0`, and window-1 pointer
`DS:[49D6]=3A16:0000`. The first record is `449A:0008`; the second
allocation returned `44BB:0008`. The table samples the far pointers at
`DS:[5D92]` (active), `DS:[5D96]` (head), first record `+0x104`
(next link), and caller local `SS:[3BFD]`. The window word is at
`3A16:+0x067C`. All pointer values are segment:offset.

| Stop | Active | Head | First `+0x104` | Local | Window word | Second `+0xBF` |
| --- | --- | --- | --- | --- | ---: | ---: |
| First `+0x19EA` | `449A:0008` | `449A:0008` | `0000:0000` | `449A:0008` | 1 | outside allocation |
| Second `+0x19B1` | `449A:0008` | `449A:0008` | `0000:0000` | `449A:0008` | 1 | outside allocation |
| `+0x19BA` | `449A:0008` | `449A:0008` | `0000:0000` | `449A:0008` | 1 | uninitialized |
| `+0x19D7` | `449A:0008` | `449A:0008` | `0000:0000` | `44BB:0008` | 1 | uninitialized |
| `+0x19DA` | `449A:0008` | `449A:0008` | `0000:0000` | `44BB:0008` | 1 | 1 |
| `+0x19E3` | `449A:0008` | `449A:0008` | `0000:0000` | `44BB:0008` | 1 | 1 |
| `+0x19E6` | `44BB:0008` | `449A:0008` | `44BB:0008` | `44BB:0008` | 2 | 1 |
| Second `+0x19EA` | `44BB:0008` | `449A:0008` | `44BB:0008` | `44BB:0008` | 2 | 1 |

At and after `+0x19DA`, the second record's `+0x104` and `+0x108` far
pointer slots are zero. At `+0x19E6`, the first record's next link points
to the second record, active points to the second record, head remains the
first, and the window word is 2. These are two-stop effects; they do not
prove the absence of intermediate writes. The post-insertion call returned
without changing those sampled fields before the second gate.

The full-RAM SHA-256 at the first `+0x19EA` is different from the
[earlier first-loop run](SLOT_A_NATIVE_19EA_FIRST_LOOP.md):
`7958dba64fe9a86196f2fb5256352a3f7105e0faefd800c00f7644df5f9a70d3`
here versus `3ed5cea594eea502fcf1bbd33bdc51c01c250201283b1d7f7eb21a47e854976d`
there. Direct comparison found 141 differing bytes: 11 in physical
`0x0041A..0x0046E`, four in `0x18E28..0x18E2B`, and 126 in
`0x20722..0x21050`. At the matched first `+0x19EA` and second
`+0x19B1` stops, the slot/count bytes, head and active pointers,
`DS:[49CE]`, window pointer and word, first-record `+0xBF`, and caller
local pointer all match. The first group contains BIOS keyboard/timer
state; the last group falls within this process's stack/work area. The
meaning of the four bytes at `0x18E28` is unassigned here. Full-RAM
hash inequality is therefore reported explicitly, not used to infer a
different party state.

## Private reproduction and assertions

In an interactive terminal with the ignored heavy DOSBox-X build on
`PATH`, prepare the same hash-pinned save with
`tools/oracle/prepare_run.sh`, then launch it with
`tools/oracle/launch.sh <private-run-directory> -time-limit 1200` under
`xvfb-run`. At the load-slot prompt use Alt+Pause, `BP 2A8E:19EA`,
`RUN`, and select `a`. At the first gate, arm `BP 2A8E:19B1` and
continue. At each subsequent stop arm the next breakpoint listed above,
use `EV CS IP DS ES DI SS SP BP AX BX CX DX FLAGS`, and use
`MEMDUMPBIN 0000 0000 A0000`; move the private `MEMDUMP.BIN` before
resuming. Keep all dumps ignored. The following assertions verify the
numeric note and the exact original and loaded code hashes without
printing game bytes:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

r = Path('resources/oracle-runs/slot-a-second-slot-live-024')
read = lambda name: (r / name).read_bytes()
digest = lambda data: sha256(data).hexdigest()
inputs = {
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for name, expected in inputs.items():
    data = read('POOLRAD/' + name)
    assert (len(data), digest(data)) == expected
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), digest(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
names = ('ram-first-19ea.bin', 'ram-second-19b1.bin',
         'ram-second-19ba.bin', 'ram-second-19d7.bin',
         'ram-second-19da.bin', 'ram-second-19e3.bin',
         'ram-second-19e6.bin', 'ram-second-19ea.bin')
hashes = (
    '7958dba64fe9a86196f2fb5256352a3f7105e0faefd800c00f7644df5f9a70d3',
    '9bb68a5522357a844395b28a3584e551bc937ec45f79a78e0692f85948062f24',
    '0d78c487fe00d9100a8a52431827c2f7e44a305811b331046a78924731ec9a69',
    'b1fccc5aa3369353cf31a7d27124cf0a93a53fbdeb585c85ba848a158402e155',
    '1517505e7d8cff0a0b42d0bc7db8f7466aae5cc14a167ed4c3d72f64d4cbc3ec',
    '243585b1f15620ddc8962ccdbffbbdade2cc85539ba2d903952b6f794f99c19a',
    '6a34b4eed8d7be5c44a369851a5aa7fb43322100726e842645a71c8ef532a10c',
    'fe05818a07ef8f445640d71bf2019058d44b5e70b8375dfb572ca504796785da',
)
ram = [read(name) for name in names]
assert all((len(m), digest(m)) == (655360, h) for m, h in zip(ram, hashes))
game = read('POOLRAD/GAME.OVR')
spans = (
    (0x19B1, 0x19EA, 1, '76347194a33838c597b3fba609cb594dcd17a78bf4d74883330276032344a9c5', '7d004b43fc63f8a7b130244c05a3f204b131bfdaa1cb0dea4567e56c21c6cfef', 2),
    (0x0AEC, 0x0EC4, 3, '715424407210de3824c3a0218cc29033f04bb357ec718aa24a7995d9452d1e36', '4da70731d0fe4afe0387c8df693aea3420b8c9c1916be920e79f16e04ad41ccc', 94),
    (0x1446, 0x157F, 5, '370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245', 'cd7d4b58fff401796a53d4779c4c55e5a731215f5eecf16f304a25b4b48a9dad', 6),
    (0x02AE, 0x032F, 6, 'ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539', '70114e09285ff85b5c6176903f5d5640e427bed0ea52b41786aa19793c2629d7', 16),
    (0x19EA, 0x19F7, 7, 'f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4', 'f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4', 0),
)
for start, end, idx, original_hash, loaded_hash, changed in spans:
    original = game[0x1DFE6 + start:0x1DFE6 + end]
    loaded = ram[idx][(0x2A8E << 4) + start:(0x2A8E << 4) + end]
    assert (digest(original), digest(loaded)) == (original_hash, loaded_hash)
    assert sum(a != b for a, b in zip(original, loaded)) == changed
word = lambda m, s, o: int.from_bytes(m[(s << 4) + o:(s << 4) + o + 2], 'little')
far = lambda m, s, o: (word(m, s, o + 2), word(m, s, o))
byte = lambda m, s, o: m[(s << 4) + o]
first = (0x449A, 8)
second = (0x44BB, 8)
for i, m in enumerate(ram):
    assert byte(m, 0x1D16, 0x3C01) == (1 if i == 0 else 2)
    assert byte(m, 0x1D16, 0x3BB8) == byte(m, 0x1D16, 0x3BD1) == 6
    assert word(m, 0x14A0, 0x49CE) == 0
    assert far(m, 0x14A0, 0x49D6) == (0x3A16, 0)
    assert far(m, 0x14A0, 0x5D96) == first
    assert far(m, 0x14A0, 0x5D92) == (second if i >= 6 else first)
    assert far(m, 0x449A, 8 + 0x104) == (second if i >= 6 else (0, 0))
    assert far(m, 0x1D16, 0x3BFD) == (second if i >= 3 else first)
    assert word(m, 0x3A16, 0x067C) == (2 if i >= 6 else 1)
    if i >= 4:
        assert byte(m, 0x44BB, 8 + 0xBF) == 1
        assert far(m, 0x44BB, 8 + 0x104) == far(m, 0x44BB, 8 + 0x108) == (0, 0)
assert [word(ram[3], 0x1D16, off) for off in range(0x37AA, 0x37B8, 2)] == [0x2A8E, 1, 1, 8, 0x44BB, 0x3BD4, 0x1D16]
assert [word(ram[5], 0x1D16, off) for off in range(0x37B2, 0x37B8, 2)] == [0x2A8E, 8, 0x44BB]
old = Path('resources/oracle-runs/slot-a-19e7-loop-gate-live-023')
for old_name, new_idx in (('gate-19ea-ram.bin', 0), ('nextslot-branch-19b1-ram.bin', 1)):
    previous = (old / old_name).read_bytes()
    current = ram[new_idx]
    assert len(previous) == len(current) == 655360
    assert sum(a != b for a, b in zip(previous, current)) == (141 if new_idx == 0 else 147)
    for s, o, n in ((0x1D16, 0x3BB8, 1), (0x1D16, 0x3BD1, 1),
                    (0x1D16, 0x3C01, 1), (0x14A0, 0x49CE, 2),
                    (0x14A0, 0x49D6, 4), (0x14A0, 0x5D92, 8),
                    (0x1D16, 0x3BFD, 4), (0x3A16, 0x067C, 2)):
        p = (s << 4) + o
        assert previous[p:p+n] == current[p:p+n]
print('second-slot native hashes and numeric fields passed')
PY
```

This watch stops at the second `+0x19EA` boundary. The later party slots,
post-loop pointer phase, gameplay key path, and issue #3 C-native parity
remain separate work.
