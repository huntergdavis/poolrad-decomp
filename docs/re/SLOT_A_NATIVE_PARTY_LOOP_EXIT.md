# Saved slot-A native party loop exit

One hash-pinned native DOS replay reached all six party-loop gates in unit
`0x6F`, took the equality exit after slot 6, and stopped at `+0x19FE`
**before** the first post-loop local write. Slots 3–6 each reached the
allocation, record-loader, insertion, and post-insertion return boundaries.
The six sampled records form one linked list in slot order; the window-1
count progresses from 1 to 6. A second targeted replay resolved a transient
`DS:[49CE]` value: the record loader's nested resident selector writes
`0x0067` during slot 4, and the next slot's direct selector clears it.
These are native load-path observations, not one-Up or C-native parity and
not a new game-routine count.

Before debugging, `deja "Poolrad saved slot A u6f remaining party slots
19EA 19F7 post loop head active allocation native"` found no indexed
session. The merged [first-loop decision](SLOT_A_NATIVE_19EA_FIRST_LOOP.md)
and [second insertion](SLOT_A_NATIVE_SECOND_SLOT_INSERTION.md) notes supplied
the prior gate and pointer leads. The [unit continuation map](U6F_PARTY_CONTINUATIONS.md)
supplied code bounds and instruction counts. The [resident selector map](RESIDENT_DAX_SELECTOR_02EB.md)
supplied the START.EXE copy range and `DS:[49CE]` writer lead. This watch
rechecked original and loaded span hashes and measured the live branch,
pointer, and writer states independently.

## Pinned private inputs and code

Both ignored runs used the same copied DOS inputs and private heavy
DOSBox-X executable:

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

Unit `0x6F` begins at `GAME.OVR` file `0x1DFE6`, loaded at `2A8E:0000`.
The resident selector is copied from `START.EXE` file
`0x08342..0x083AC` into this run's `1026:02EB..0355`. Counts for the
complete bounded bodies are from the cited static maps. The hashes and
changed-byte counts below were recomputed from these private images and
RAM. An inclusive address range appears in each row.

| Span | Bytes / instructions | Original SHA-256 | Loaded SHA-256 | Changed bytes |
| --- | ---: | --- | --- | ---: |
| Next-slot prefix `+1937..1953` | 29 / 12 | `4dd8a17740510fb2527c8683be4a0cf01e2a096fedb8c099cbfb315cd2577124` | `4dd8a17740510fb2527c8683be4a0cf01e2a096fedb8c099cbfb315cd2577124` | 0 |
| Direct selector/test `+19A8..19B0` | 9 / 3 | `34225ed53f01e5483c1e290c0958129055a15cbed08fbd34cc1007f28c2a37cb` | `09eb6c96435627d600410ae10b43548304ec9f27e62070481443ab6bc639c6fe` | 2 |
| Allocation/load/insertion caller `+19B1..19E9` | 57 / 22 | `76347194a33838c597b3fba609cb594dcd17a78bf4d74883330276032344a9c5` | `7d004b43fc63f8a7b130244c05a3f204b131bfdaa1cb0dea4567e56c21c6cfef` | 2 |
| Loop gate `+19EA..19F6` | 13 / 4 | `f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4` | `f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4` | 0 |
| Post-loop pointer gate `+19F7..1A12` | 28 / 8 | `bfac9c4b98fb5b3f87a5c17a87f7f14be9895def4a1333f1ced5a24d3234ae40` | `bfac9c4b98fb5b3f87a5c17a87f7f14be9895def4a1333f1ced5a24d3234ae40` | 0 |
| Nested selector call `+0DAA..0DAE` | 5 / 1 | `84087c84e89649ccb1e5d5618377e5395beca7fc4080f5718865da6f0e0fdd6b` | `8cb85b242ae034a616972aad3545d6f5fc95ee435692370a2512ade155f942bd` | 2 |
| Resident selector `+02EB..0355` | 107 / 43 | `141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474` | `a1bd874454a477dc56eb74bee8be51f5e77b17c782081440306e0ffb5213d2e2` | 10 |
| Resident write `+0334..0336`, START file `0x0838B..0x0838D` | 3 / 1 | `390361f8eb46a3bd47a38a0a5e6fce7ee26c17602fffb09d166b8d69cd58d567` | `390361f8eb46a3bd47a38a0a5e6fce7ee26c17602fffb09d166b8d69cd58d567` | 0 |

The selector and caller differences are in loaded far-call operands. The
three-byte writer itself is identical in original and loaded images.

## Six gates and equality exit

The main run was `resources/oracle-runs/slot-a-party-loop-live-025`.
`BP 2A8E:19EA` was armed at the load-slot prompt before selecting `a`.
The first two gate visits agree with the merged prior watches. From the
second gate, breakpoints covered `+19B1`, `+19BA`, `+19D7`, `+19DA`,
`+19E3`, `+19E6`, `+19F2`, `+19F7`, and `+19FE`. At those caller
boundaries, slots 3–6 each passed the selector's nonzero-AL path and
returned from allocation, record load, insertion, and post-insertion
before the next gate. Every sampled gate had copied and source count `6`.
The current one-based slot byte at `SS:[3C01]` was respectively `1` through
`6`.

The four newly watched allocator returns at `+19BA` had `AX=0008` and
`DX=44D6,44F5,4513,4529` for slots 3–6. Their subsequent `+19DA` caller
locals `SS:[3BFD]` held those same far pointers. The first two pointers
`449A:0008` and `44BB:0008` are rechecked at their gate RAM snapshots
and were allocated in the cited prior watches.

| Gate | Slot / count | `DS:[5D92]` active | `DS:[5D96]` head | Window-1 `+067C` | `DS:[49CE]` |
| --- | --- | --- | --- | ---: | ---: |
| First `+19EA` | 1 / 6 | `449A:0008` | `449A:0008` | 1 | 0 |
| Second `+19EA` | 2 / 6 | `44BB:0008` | `449A:0008` | 2 | 0 |
| Third `+19EA` | 3 / 6 | `44D6:0008` | `449A:0008` | 3 | 0 |
| Fourth `+19EA` | 4 / 6 | `44F5:0008` | `449A:0008` | 4 | 103 |
| Fifth `+19EA` | 5 / 6 | `4513:0008` | `449A:0008` | 5 | 103 |
| Sixth `+19EA` | 6 / 6 | `4529:0008` | `449A:0008` | 6 | 0 |

At the sixth gate the node slot bytes `+0xBF` are `0,1,2,3,4,5` in
that order. Each node's far link at `+0x104` points to the next listed
node; the sixth link is null. All six sampled `+0x108` links are null.
At each gate the head remains the first record, the active pointer is the
newest, and the window-1 word equals the number of inserted records. These
snapshots establish the listed states, not an absence of intermediate
writes.

At `+19F2` after the sixth compare, `AX=0006`, `FLAGS=7246` (ZF=1),
and execution next stopped at `+19F7`. The post-loop code read the head
far pointer into `DX:AX=449A:0008` and stopped at `+19FE` **before**
`MOV [BP-01CD],AX`, its first local write. The caller local still held
the sixth record pointer `4529:0008`; the full-RAM hashes at gate 6,
`+19F7`, and `+19FE` are identical. The branch at `+1A0E` and later
post-loop calls were not reached by this watch.

## Transient `DS:[49CE]` writer and clearer

The main run sampled `DS:[49CE]=0` at slot 4's `+19BA`, then `103`
at `+19DA`; it remained `103` through that slot's gate. It was `0`
again by slot 5's `+19BA`, then `103` at slot 5's `+19DA` and gate.
It was `0` by slot 6's `+19BA` and gate. Thus `103` is an observed
transient, not a final party-loop result. The main run alone bracketed
the first change inside slot 4's record loader.

The separate ignored `slot-a-party-49ce-watch-026` run used the same
hashed inputs. At slot 3's gate `DS:[49CE]=0`. After the slot-4 direct
selector returned AL=1, the `+19D7` loader-call snapshot still had
`DS:[49CE]=0`. A `BPM 14A0:49CE` memory-change watch then reported
`00 -> 67` and stopped at `1026:0337`, immediately after resident
`1026:0334 MOV DS:[49CE],AX`. `AX=0067`. The resident frame at
`SS:BP=1D16:33E2` contains return `2A8E:0DAF`, tying the write to
the record loader's nested selector call at `+0DAA`. The preceding
resident instruction calls SYSTEM IOResult `0x04A2`; this shows the
returned word stored here, without assigning a DOS-error meaning to
`0x0067`.

At the next slot's direct selector-call site `2A8E:19A8`, the word was
still `0x0067`. The same watch then reported `67 -> 00` and stopped at
`1026:0337` after the same `+0334` write, now with `AX=0000`. The
resident frame at `SS:BP=1D16:37AE` contains return `2A8E:19AD`,
proving the clearing call is the direct next-slot selector. It returned
AL=1 at `+19AD`. Both value changes have last-before and first-after
RAM hashes below. No contents of the probed files or reason for the
runtime result are inferred.

## Private RAM hashes and reproducibility

Each dump is 655,360 bytes. All images, save files, screenshots,
extracted code, and memory dumps stay under ignored `resources/`.

| Main-run stop | SHA-256 |
| --- | --- |
| Gate 1 | `aeed12af9a04549904660df999bd79e784c3497df9582f32f0b0899e684a528f` |
| Gate 2 | `7beee86f0f37728ebae93a5f43e85680a99e038b3bc6236c46668289ac2eaeae` |
| Gate 3 | `ac9802f1f32270177646d0c0508133c8639132229f7af3dafc7bf6efaf63f89f` |
| Gate 4 | `b90c8754cc442d9947013567e8f56e620a50c0bfa0d6462f198cf3d851d403d7` |
| Gate 5 | `a57fae02b943e3faece64e57abb79138126120c2aeab5b3a41e8b27fe9f1736f` |
| Gate 6, `+19F7`, `+19FE` | `8999ee558ae6f758ee3dd4ea4376abcfeba53be2ca59c2a1add70d2604aa12e2` |
| Slot 3 `+19BA`, `+19DA`, `+19E6` | `36ba7f102cfea91770eca0d478eea15b706bb32bc881f0cd9c7f2853ddedd656`, `de7b3352203fdfe9be72ae37af7647300cb1aec5072723c109a47f57e8e8a402`, `d4734b24c0e6bddc3b0c5239f3201b41edf7e6bfb243a13d8d2e1e733829537d` |
| Slot 4 `+19BA`, `+19DA`, `+19E6` | `bed3e4e466633269fa34723d77997ba4331eb9511231faca74ecacb4ba7c7d38`, `4a78f16c10d1ba88a89ff46b5299f01b9c39cff4b46d2dee4f27229372179261`, `e1ed91739b1214c0be2adc6ff78f100c64d2012fb369bed3b6f87ca1a36a8616` |
| Slot 5 `+19BA`, `+19DA`, `+19E6` | `dc90da7f1e7b63c0af78bdd50dcf8eb63acf1a39f7db225e24a36882b0021c49`, `029fafa320051685771c11baa0d9f275b35375451d90ae291712873cf35909ac`, `9dcb2e7cbcc43ea3924e327e002c0e6b3111a249f6e3d903a34d6641c423d82a` |
| Slot 6 `+19BA`, `+19DA`, `+19E6` | `4460bf006eafef36341b6cb684c52dd8b9c86d66280a5f2f1ae6add104614d2c`, `bc0a6e67706c1bd67d843513de4afaf9599eb7b1e2e8b814a8a5be9726afb6fc`, `976f45e26019e9cf15ea77fcad2798d14a784197ae782cef1f67c7aafcbf60e0` |

| Write-watch stop | SHA-256 |
| --- | --- |
| Slot-3 gate, pre-watch zero | `92d446a0d24330977e3c0134cb241810533688ffbb9a4da78347221fa7bcc24e` |
| Slot-4 `+19D7`, last zero | `3a12d9aa0ef77a750ebd92c3f53ab5ed76b8e34835d353bb804c7962ed06e1f2` |
| Resident `1026:0337`, first 103 | `5371b070e2501979a4284aa206d9860e6904a0c7d2d4aa8e4183aabae2056a1b` |
| Slot-5 `+19A8`, last 103 | `6377beef0132d3641064bd2973fd7c9386aaa8876b9c1e2df9b1eb1a581f14f0` |
| Resident `1026:0337`, first zero after clear | `9da0420f7c756d39c77551eb1259d0183fd52e87553f43b6c8b2682baa217e97` |

Prepare two fresh ignored runs with `tools/oracle/prepare_run.sh` from the
same save. Launch each through `tools/oracle/launch.sh` with the pinned heavy
DOSBox-X on `PATH` in an interactive terminal. At the load-slot prompt,
open the debugger with Alt+Pause, arm `BP 2A8E:19EA`, `RUN`, and select
`a`. In the main run, use the boundary breakpoints listed above; at each
sample, `EV CS IP DS ES DI SS SP BP AX BX CX DX FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`, moving the private dump before resuming.
In the targeted run, continue to the third gate, arm
`BPM 14A0:49CE` plus `BP 2A8E:19A8`, `BP 2A8E:19AD`,
`BP 2A8E:19D7`, and `BP 2A8E:19DA`. Capture the two memory-change
stops and the stated last-value boundaries. The memory watch reports
the changed byte and pauses at the instruction after the write.

The following private assertions check the hashes and numeric claims
without printing game bytes:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

a = Path('resources/oracle-runs/slot-a-party-loop-live-025')
b = Path('resources/oracle-runs/slot-a-party-49ce-watch-026')
digest = lambda data: sha256(data).hexdigest()
read = lambda root, name: (root / name).read_bytes()
inputs = {
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for root in (a, b):
    for name, expected in inputs.items():
        data = read(root, 'POOLRAD/' + name)
        assert (len(data), digest(data)) == expected
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), digest(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
gate_hashes = (
    'aeed12af9a04549904660df999bd79e784c3497df9582f32f0b0899e684a528f',
    '7beee86f0f37728ebae93a5f43e85680a99e038b3bc6236c46668289ac2eaeae',
    'ac9802f1f32270177646d0c0508133c8639132229f7af3dafc7bf6efaf63f89f',
    'b90c8754cc442d9947013567e8f56e620a50c0bfa0d6462f198cf3d851d403d7',
    'a57fae02b943e3faece64e57abb79138126120c2aeab5b3a41e8b27fe9f1736f',
    '8999ee558ae6f758ee3dd4ea4376abcfeba53be2ca59c2a1add70d2604aa12e2',
)
gate = [read(a, f'ram-gate-{i}.bin') for i in range(1, 7)]
assert all((len(m), digest(m)) == (655360, h) for m, h in zip(gate, gate_hashes))
extra = {
    'ram-loop-exit-19f7.bin': gate_hashes[5],
    'ram-postloop-first-write-19fe.bin': gate_hashes[5],
    'ram-s3-19ba.bin': '36ba7f102cfea91770eca0d478eea15b706bb32bc881f0cd9c7f2853ddedd656',
    'ram-s3-19da.bin': 'de7b3352203fdfe9be72ae37af7647300cb1aec5072723c109a47f57e8e8a402',
    'ram-s3-19e6.bin': 'd4734b24c0e6bddc3b0c5239f3201b41edf7e6bfb243a13d8d2e1e733829537d',
    'ram-s4-19ba.bin': 'bed3e4e466633269fa34723d77997ba4331eb9511231faca74ecacb4ba7c7d38',
    'ram-s4-19da.bin': '4a78f16c10d1ba88a89ff46b5299f01b9c39cff4b46d2dee4f27229372179261',
    'ram-s4-19e6.bin': 'e1ed91739b1214c0be2adc6ff78f100c64d2012fb369bed3b6f87ca1a36a8616',
    'ram-s5-19ba.bin': 'dc90da7f1e7b63c0af78bdd50dcf8eb63acf1a39f7db225e24a36882b0021c49',
    'ram-s5-19da.bin': '029fafa320051685771c11baa0d9f275b35375451d90ae291712873cf35909ac',
    'ram-s5-19e6.bin': '9dcb2e7cbcc43ea3924e327e002c0e6b3111a249f6e3d903a34d6641c423d82a',
    'ram-s6-19ba.bin': '4460bf006eafef36341b6cb684c52dd8b9c86d66280a5f2f1ae6add104614d2c',
    'ram-s6-19da.bin': 'bc0a6e67706c1bd67d843513de4afaf9599eb7b1e2e8b814a8a5be9726afb6fc',
    'ram-s6-19e6.bin': '976f45e26019e9cf15ea77fcad2798d14a784197ae782cef1f67c7aafcbf60e0',
}
for name, expected in extra.items():
    data = read(a, name)
    assert (len(data), digest(data)) == (655360, expected)
watch = {
    'ram-gate-3-prewatch.bin': '92d446a0d24330977e3c0134cb241810533688ffbb9a4da78347221fa7bcc24e',
    'ram-s4-19d7-last-zero.bin': '3a12d9aa0ef77a750ebd92c3f53ab5ed76b8e34835d353bb804c7962ed06e1f2',
    'ram-s4-first-103.bin': '5371b070e2501979a4284aa206d9860e6904a0c7d2d4aa8e4183aabae2056a1b',
    'ram-s5-19a8-last-103.bin': '6377beef0132d3641064bd2973fd7c9386aaa8876b9c1e2df9b1eb1a581f14f0',
    'ram-s5-selector-clear.bin': '9da0420f7c756d39c77551eb1259d0183fd52e87553f43b6c8b2682baa217e97',
}
for name, expected in watch.items():
    data = read(b, name)
    assert (len(data), digest(data)) == (655360, expected)
game = read(a, 'POOLRAD/GAME.OVR')
loaded = gate[5]
overlay_spans = (
    (0x1937, 0x1954, '4dd8a17740510fb2527c8683be4a0cf01e2a096fedb8c099cbfb315cd2577124', '4dd8a17740510fb2527c8683be4a0cf01e2a096fedb8c099cbfb315cd2577124', 0),
    (0x19A8, 0x19B1, '34225ed53f01e5483c1e290c0958129055a15cbed08fbd34cc1007f28c2a37cb', '09eb6c96435627d600410ae10b43548304ec9f27e62070481443ab6bc639c6fe', 2),
    (0x19B1, 0x19EA, '76347194a33838c597b3fba609cb594dcd17a78bf4d74883330276032344a9c5', '7d004b43fc63f8a7b130244c05a3f204b131bfdaa1cb0dea4567e56c21c6cfef', 2),
    (0x19EA, 0x19F7, 'f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4', 'f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4', 0),
    (0x19F7, 0x1A13, 'bfac9c4b98fb5b3f87a5c17a87f7f14be9895def4a1333f1ced5a24d3234ae40', 'bfac9c4b98fb5b3f87a5c17a87f7f14be9895def4a1333f1ced5a24d3234ae40', 0),
    (0x0DAA, 0x0DAF, '84087c84e89649ccb1e5d5618377e5395beca7fc4080f5718865da6f0e0fdd6b', '8cb85b242ae034a616972aad3545d6f5fc95ee435692370a2512ade155f942bd', 2),
)
for start, end, original_hash, loaded_hash, changed in overlay_spans:
    original = game[0x1DFE6+start:0x1DFE6+end]
    live = loaded[(0x2A8E<<4)+start:(0x2A8E<<4)+end]
    assert (digest(original), digest(live)) == (original_hash, loaded_hash)
    assert sum(x != y for x, y in zip(original, live)) == changed
exe = read(b, 'POOLRAD/START.EXE')
resident = read(b, 'ram-s4-first-103.bin')
for start, end, original_hash, loaded_hash, changed in (
    (0x02EB, 0x0356, '141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474', 'a1bd874454a477dc56eb74bee8be51f5e77b17c782081440306e0ffb5213d2e2', 10),
    (0x0334, 0x0337, '390361f8eb46a3bd47a38a0a5e6fce7ee26c17602fffb09d166b8d69cd58d567', '390361f8eb46a3bd47a38a0a5e6fce7ee26c17602fffb09d166b8d69cd58d567', 0),
):
    original = exe[0x08342+start-0x02EB:0x08342+end-0x02EB]
    live = resident[(0x1026<<4)+start:(0x1026<<4)+end]
    assert (digest(original), digest(live)) == (original_hash, loaded_hash)
    assert sum(x != y for x, y in zip(original, live)) == changed
word = lambda m, s, o: int.from_bytes(m[(s<<4)+o:(s<<4)+o+2], 'little')
byte = lambda m, s, o: m[(s<<4)+o]
far = lambda m, s, o: (word(m, s, o+2), word(m, s, o))
nodes = ((0x449A,8),(0x44BB,8),(0x44D6,8),(0x44F5,8),(0x4513,8),(0x4529,8))
for i, m in enumerate(gate):
    assert byte(m, 0x1D16, 0x3C01) == i+1
    assert byte(m, 0x1D16, 0x3BB8) == byte(m, 0x1D16, 0x3BD1) == 6
    assert far(m, 0x14A0, 0x5D92) == far(m, 0x1D16, 0x3BFD) == nodes[i]
    assert far(m, 0x14A0, 0x5D96) == nodes[0]
    assert far(m, 0x14A0, 0x49D6) == (0x3A16,0)
    assert word(m, 0x3A16, 0x067C) == i+1
    assert word(m, 0x14A0, 0x49CE) == (103 if i in (3,4) else 0)
    for j in range(i+1):
        s, off = nodes[j]
        assert byte(m, s, off+0xBF) == j
        assert far(m, s, off+0x104) == (nodes[j+1] if j < i else (0,0))
        assert far(m, s, off+0x108) == (0,0)
for slot in range(3,7):
    pre = read(a, f'ram-s{slot}-19ba.bin')
    postload = read(a, f'ram-s{slot}-19da.bin')
    postinsert = read(a, f'ram-s{slot}-19e6.bin')
    assert far(postload, 0x1D16, 0x3BFD) == nodes[slot-1]
    assert far(postinsert, 0x14A0, 0x5D92) == nodes[slot-1]
    assert word(postinsert, 0x3A16, 0x067C) == slot
    assert word(pre, 0x14A0, 0x49CE) == 0
    assert word(postload, 0x14A0, 0x49CE) == (103 if slot in (4,5) else 0)
assert far(read(a, 'ram-postloop-first-write-19fe.bin'), 0x1D16, 0x3BFD) == nodes[5]
assert [word(read(b, name), 0x14A0, 0x49CE) for name in watch] == [0,0,103,103,0]
first = read(b, 'ram-s4-first-103.bin')
clear = read(b, 'ram-s5-selector-clear.bin')
assert [word(first, 0x1D16, 0x33E2+o) for o in (0,2,4)] == [0x37A6,0x0DAF,0x2A8E]
assert [word(clear, 0x1D16, 0x37AE+o) for o in (0,2,4)] == [0x3DCA,0x19AD,0x2A8E]
print('six-slot loop, writer, clearer, and code hashes passed')
PY
```

The first post-loop local store and all later post-loop effects remain
outside this watch. The source of the runtime `0x0067` result is not
classified here. Issue #3 C-native one-Up parity remains open.
