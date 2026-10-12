# Saved slot-A first party-loop decision

The first native DOS visit to unit `0x6F:+0x19EA` in the hash-pinned
saved slot-A replay compared current one-based slot `1` with copied count
`6`. The equality jump was not taken: execution reached `+0x19F4`, jumped
to `+0x1937`, incremented the slot to `2`, and reached the next selector
call at `+0x19A8`. That call returned AL=`1`; its zero jump was also not
taken, and execution stopped at `+0x19B1` before the next allocation. This
is a first-decision and next-test observation, not a result for the remaining
slots, one Up, or a C routine count.

Before continuing the debugger, `deja "Poolrad saved slot A u6f 19EA 19EE
19F2 first loop decision next slot exit native"` found no indexed session.
The merged [post-insertion native watch](SLOT_A_NATIVE_19E7_LOOP_GATE.md)
supplied the already paused `+0x19EA` run and its hash-pinned RAM.
The merged [static continuation map](U6F_PARTY_CONTINUATIONS.md) supplied
the initial `+0x19EA` and `+0x1937` address leads. The original and loaded
instruction spans below were checked independently against the private
overlay and live RAM. The two first-gate numeric locals were sent to pC's
separate static-map lane; this note owns the live path only.

## Hash-locked code and inputs

| Private input or span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Saved slot-A `SAVGAMA.DAT` | 13,137 bytes | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original and loaded gate `+0x19EA..+0x19F6`, file `0x1F9D0..0x1F9DC` | 13 bytes / 4 instructions | `f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4` |
| Original and loaded next-slot prefix `+0x1937..+0x1953`, file `0x1F91D..0x1F939` | 29 bytes / 12 instructions | `4dd8a17740510fb2527c8683be4a0cf01e2a096fedb8c099cbfb315cd2577124` |
| Original selector/test `+0x19A8..+0x19B0`, file `0x1F98E..0x1F996` | 9 bytes / 3 instructions | `34225ed53f01e5483c1e290c0958129055a15cbed08fbd34cc1007f28c2a37cb` |
| Loaded selector/test `2A8E:19A8..19B0` | 9 bytes / 3 instructions | `09eb6c96435627d600410ae10b43548304ec9f27e62070481443ab6bc639c6fe` |
| Private heavy DOSBox-X executable | 47,259,176 bytes | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The unit file base is `0x1DFE6`. The gate and next-slot prefix are identical
in the original and loaded images. In the selector/test span, only the
far-call segment operand at `+0x19AB..+0x19AC` differs after loading. The
gate instructions load `[BP-0x1C9]`, compare `[BP-0x212]`, conditionally
jump to `+0x19F7`, then jump to `+0x1937`. The next-slot prefix begins by
incrementing `[BP-0x1C9]`; `+0x1947` reloads it. The selector/test span
calls the resident selector, tests AL, then conditionally jumps to
`+0x19EA`. This instruction map gives the branch meaning; the live stops
below establish which paths this replay took.

## Same-run stops and numeric state

At the visible load-slot prompt, the preceding watch armed `+0x19E6`,
`+0x19E7`, and `+0x19EA` before selecting `a`. At the first `+0x19EA`
stop, this watch added `+0x19EE`, `+0x19F2`, `+0x19F4`, `+0x1937`,
`+0x1947`, `+0x19A8`, `+0x19AD`, `+0x19AF`, and `+0x19B1` breakpoints.
The debugger then stopped in precisely that order. All stops are before
their named instructions; `CS=2A8E`, `DS=14A0`, `SS=1D16`, and `BP=3DCA`
throughout. The listed register values are hexadecimal.

| Stop | `SP`; `AX`; `FLAGS` | Observation |
| --- | --- | --- |
| `+0x19EA` | `37B8`; `0005`; `7246` | Before loading current slot. |
| `+0x19EE` | `37B8`; `0001`; `7246` | Frame byte `SS:3C01=1`; copied count `SS:3BB8=6`. |
| `+0x19F2` | `37B8`; `0001`; `7293` | ZF=0 after compare; equality exit targets `+0x19F7`. |
| `+0x19F4` | not separately sampled | Equality jump fell through to the backward jump. |
| `+0x1937` | `37B8`; `0001`; `7293` | Backward jump landed before increment. |
| `+0x1947` | `37B0`; `0001`; `7203` | Frame byte `SS:3C01=2`; copied count still `6`. Four words have been pushed for the next slot setup. |
| `+0x19A8` | `37B4`; `0004`; `7206` | Next selector call is ready; no selector result yet. |
| `+0x19AD` | `37B8`; `0001`; `7206` | Selector returned AL=1. |
| `+0x19AF` | `37B8`; `0001`; `7202` | ZF=0 after `OR AL,AL`; zero branch targets `+0x19EA`. |
| `+0x19B1` | `37B8`; `0001`; `7202` | Zero branch fell through; stop is before allocation setup. |

The debugger's `+0x19F2` and `+0x19AF` flags show why the following
equality jumps did not take their exits. The reached `+0x19F4`,
`+0x1937`, and `+0x19B1` stops independently confirm the direction.
No second-slot allocation, record load, party insertion, second-slot loop
gate, post-loop pointer phase, or Up parity is observed here.

Each private RAM snapshot is 655,360 bytes. Repeated hashes show no RAM
change between `+0x19EA`, `+0x19EE`, and `+0x1937`, or between
`+0x19AD` and `+0x19B1`:

| Stop or stops | SHA-256 |
| --- | --- |
| `+0x19EA`, `+0x19EE`, `+0x1937` | `3ed5cea594eea502fcf1bbd33bdc51c01c250201283b1d7f7eb21a47e854976d` |
| `+0x1947` | `54a8f429e9f53aaddfa5661d13bea33ba10298ddee7d25979ec4586ac0815ad9` |
| `+0x19A8` | `29207df494fe00bd19cc8ad2036618f2dc0d3e08454d0055b3852f14f59b59fb` |
| `+0x19AD`, `+0x19B1` | `e56391c6f398e29c2ca777c52f728a787799c0cc533f6a3437142117f1dd0de6` |

Across all seven snapshots, the copied count and source count remain `6`,
`DS:[49CE]` remains `0`, active-character and party-head far pointers
remain `449A:0008`, window-1 word `+0x067C` remains `1`, and the first
allocated node's `+0x00BF` byte remains `0`. These sampled fields do not
establish intermediate writes or later effects.

## Private reproduction and assertions

Use the [post-insertion watch](SLOT_A_NATIVE_19E7_LOOP_GATE.md) commands
and hash-pinned ignored input to reach the first `+0x19EA` stop with the
heavy debugger in an interactive terminal. Arm the nine additional
breakpoints listed above. At each stop, use
`EV CS IP DS SS SP BP AX FLAGS`; at `+0x19EA`, `+0x19EE`, `+0x1937`,
`+0x1947`, `+0x19A8`, `+0x19AD`, and `+0x19B1`, run
`MEMDUMPBIN 0000 0000 A0000` and move the private `MEMDUMP.BIN`
before resuming. The ignored names used below match those stops.

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

r = Path('resources/oracle-runs/slot-a-19e7-loop-gate-live-023')
load = lambda name: (r / name).read_bytes()
digest = lambda data: sha256(data).hexdigest()
expected_inputs = {
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for name, expected in expected_inputs.items():
    data = load('POOLRAD/' + name)
    assert (len(data), digest(data)) == expected
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), digest(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
names = ('gate-19ea-ram.bin', 'compare-19ee-ram.bin', 'loop-preinc-1937-ram.bin',
         'nextslot-1947-ram.bin', 'nextselector-19a8-ram.bin',
         'nextselector-return-19ad-ram.bin', 'nextslot-branch-19b1-ram.bin')
hashes = ('3ed5cea594eea502fcf1bbd33bdc51c01c250201283b1d7f7eb21a47e854976d',) * 3 + (
    '54a8f429e9f53aaddfa5661d13bea33ba10298ddee7d25979ec4586ac0815ad9',
    '29207df494fe00bd19cc8ad2036618f2dc0d3e08454d0055b3852f14f59b59fb',
    'e56391c6f398e29c2ca777c52f728a787799c0cc533f6a3437142117f1dd0de6',
    'e56391c6f398e29c2ca777c52f728a787799c0cc533f6a3437142117f1dd0de6')
ram = [load(name) for name in names]
assert all((len(data), digest(data)) == (655360, h) for data, h in zip(ram, hashes))
game = load('POOLRAD/GAME.OVR')
for start, end, snapshot, original_hash, loaded_hash in (
    (0x19EA, 0x19F7, 0, 'f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4', 'f8167ce0d816f623861fd01e72891f85d08215eece2fea53846ed0cba0c3b5a4'),
    (0x1937, 0x1954, 3, '4dd8a17740510fb2527c8683be4a0cf01e2a096fedb8c099cbfb315cd2577124', '4dd8a17740510fb2527c8683be4a0cf01e2a096fedb8c099cbfb315cd2577124'),
    (0x19A8, 0x19B1, 4, '34225ed53f01e5483c1e290c0958129055a15cbed08fbd34cc1007f28c2a37cb', '09eb6c96435627d600410ae10b43548304ec9f27e62070481443ab6bc639c6fe'),
):
    original = game[0x1DFE6 + start:0x1DFE6 + end]
    loaded = ram[snapshot][(0x2A8E << 4) + start:(0x2A8E << 4) + end]
    assert (digest(original), digest(loaded)) == (original_hash, loaded_hash)
word = lambda data, seg, off: int.from_bytes(data[(seg << 4) + off:(seg << 4) + off + 2], 'little')
far = lambda data, seg, off: (word(data, seg, off + 2), word(data, seg, off))
for i, data in enumerate(ram):
    assert data[(0x1D16 << 4) + 0x3C01] == (1 if i < 3 else 2)
    assert data[(0x1D16 << 4) + 0x3BB8] == 6
    assert data[(0x1D16 << 4) + 0x3BD1] == 6
    assert word(data, 0x14A0, 0x49CE) == 0
    assert far(data, 0x14A0, 0x5D92) == (0x449A, 8)
    assert far(data, 0x14A0, 0x5D96) == (0x449A, 8)
    assert word(data, 0x3A16, 0x067C) == 1
    assert data[(0x449A << 4) + 8 + 0xBF] == 0
print('first loop decision hashes and numeric fields passed')
PY
```

Original images, save, emulator captures, RAM dumps and extracted code stay
under ignored `resources/`. This note publishes only addresses, hashes,
instruction counts and numeric observations.
