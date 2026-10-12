# Native slot-A first allocation and record-loader return

One native DOS v1.3 replay of the hash-pinned saved slot A reached the first
nonzero-selector continuation in unit `0x6F`. It stopped in order at loaded
`2A8E:19BA`, `:19C2`, `:19D7`, and `:19DA`. The SYSTEM GetMem call at
`+0x19B5` returned far pointer `449A:0008`, the caller stored those words,
and its near call to record loader `+0x0AEC` returned to `+0x19DA`. This is a
reachability and caller-frame observation. It does not identify record
contents, completion of individual file effects, party insertion, directory
creation, or end-to-end Up parity. No strict-C routine count is claimed.

Before debugging, `deja "Poolrad slot A unit 6F 19B5 GetMem return 19BA
19C2 record loader 19D7 19DA native"` and the shorter `deja "19D7 record
loader"` found no indexed match. The merged
[continuation map](U6F_PARTY_CONTINUATIONS.md) supplied the call and local
slot leads; they were rechecked against the ignored original image and the
four live breakpoints. The [preceding selector return](SLOT_A_NATIVE_19AD_SELECTOR_RETURN.md)
established that this saved first iteration selects `+0x19B1`.

## Hash-locked inputs and code

| Private input or span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Saved slot-A `SAVGAMA.DAT` | 13,137 bytes | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original caller `+0x19B1..+0x19E9`, GAME file `0x1F997..0x1F9CF` | 57 bytes, 22 instructions | `76347194a33838c597b3fba609cb594dcd17a78bf4d74883330276032344a9c5` |
| Loaded caller `2A8E:19B1..19E9` in private `+19D7` RAM | 57 bytes, 22 instructions | `7d004b43fc63f8a7b130244c05a3f204b131bfdaa1cb0dea4567e56c21c6cfef` |
| Record loader `+0x0AEC..+0x0EC3`, GAME file `0x1EAD2..0x1EEA9` | 984 bytes | `715424407210de3824c3a0218cc29033f04bb357ec718aa24a7995d9452d1e36` |
| Private heavy DOSBox-X executable | 47,259,176 bytes | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The original caller decodes `+0x19B5` as a far call to SYSTEM
`0x0AF8:0x023F`, with first post-return instruction `+0x19BA`. The loaded
call targets `0x131C:0x023F`; only relative bytes `+7,+8` of the 57-byte
caller differ through segment relocation. `+0x19BA/+0x19BE` store AX/DX at
`SS:[BP-0x1CD]`/`SS:[BP-0x1CB]`; `+0x19C2` follows both stores. Source
`+0x19D6` pushes CS before the near call at `+0x19D7` to `+0x0AEC`, whose
first post-return instruction is `+0x19DA`.

## First-iteration native stops

At the visible load-slot prompt, breakpoints were armed at the four loaded
addresses before choosing `a`. All four stops below occurred in this order
in one run. Register values and pointer words are hexadecimal. `CS=2A8E`,
`DS=14A0`, `SS=1D16`, and `BP=3DCA` at all four stops.

| Stop before instruction | `SP` | `AX`; `DX`; `FLAGS` | Reached observation |
| --- | --- | --- | --- |
| `+0x19BA` | `37B8` | `0008`; `449A`; `7202` | GetMem returned `DX:AX=449A:0008`. |
| `+0x19C2` | `37B8` | `0008`; `449A`; `7202` | Caller locals `[BP-0x1CD]=0008`, `[BP-0x1CB]=449A` after both stores. |
| `+0x19D7` | `37AA` | `0001`; `449A`; `7202` | `DI=3BD4`; top seven stack words are recorded below before the loader call. |
| `+0x19DA` | `37B8` | `3E01`; `0000`; `7206` | Loader returned; `DI=3626`; caller locals still hold `449A:0008`. |

At `+0x19D7`, the seven observed 16-bit words at `SS:37AA` in increasing
address order were `2A8E, 0001, 0001, 0008, 449A, 3BD4, 1D16`. The last
four include the allocated far pointer and basename-local pointer as the
source pushes specify; no basename or record bytes are published. The
return to `+0x19DA` means no blocking/failure boundary prevented this
particular call from returning. The next caller instruction pushes the
allocated pointer for a separate insertion call at `+0x19E3`, outside this
watch. `AX=3E01` is recorded as a register value, not classified as a
record-loader success code.

The four 655,360-byte private RAM dumps have SHA-256, in stop order:

| Stop | Private dump SHA-256 |
| --- | --- |
| `+0x19BA` | `59576ad37f2291a9f2b84d7a5a9c0231d15ce7ca382ed461bf163e98ccca1636` |
| `+0x19C2` | `bc7d9e1e3d761aa4f4451487c86a1815129f1e42afd71320f7ae01bd3db636bf` |
| `+0x19D7` | `a5cfe325747c5f9f8fb79510dcd00380ca5fc521a1ba172395f5b71949de5023` |
| `+0x19DA` | `8355da7c4be299e3f2bea1d023258e75b3a240d18bb066a521e923ee35979aa0` |

## Private reproduction and numeric assertions

Run in an isolated checkout with the ignored pinned DOS inputs and private
heavy debugger:

```sh
tools/oracle/prepare_run.sh slot-a-19ba-record-live-021
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s "-screen 0 1280x1024x24" \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-19ba-record-live-021 \
  -time-limit 1200
```

Wait for the visible code-word prompt, enter Return, choose `l` at the main
menu, then pause at the visible load-slot prompt with Alt+Pause. In the
debugger arm `BP 2A8E:19BA`, `BP 2A8E:19C2`, `BP 2A8E:19D7`, and
`BP 2A8E:19DA`; enter `RUN` and choose `a`. At each stop record
`EV CS IP DS SS SP BP AX DX DI FLAGS` and run
`MEMDUMPBIN 0000 0000 A0000`; move the private `MEMDUMP.BIN` to an
individual file under the ignored run directory before resuming. The four
names used below are `getmem-return-19ba-ram.bin`,
`pointer-stored-19c2-ram.bin`, `record-call-19d7-ram.bin`, and
`record-return-19da-ram.bin`.

With those ignored files, this emits numeric values and hashes only:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

run = Path('resources/oracle-runs/slot-a-19ba-record-live-021')
load = lambda name: (run / name).read_bytes()
digest = lambda data: sha256(data).hexdigest()
game = load('POOLRAD/GAME.OVR')
start = load('POOLRAD/START.EXE')
save = load('POOLRAD/SAVGAMA.DAT')
assert (len(game), digest(game)) == (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614')
assert (len(start), digest(start)) == (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd')
assert (len(save), digest(save)) == (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1')
names = ['getmem-return-19ba', 'pointer-stored-19c2', 'record-call-19d7', 'record-return-19da']
expected = [
    '59576ad37f2291a9f2b84d7a5a9c0231d15ce7ca382ed461bf163e98ccca1636',
    'bc7d9e1e3d761aa4f4451487c86a1815129f1e42afd71320f7ae01bd3db636bf',
    'a5cfe325747c5f9f8fb79510dcd00380ca5fc521a1ba172395f5b71949de5023',
    '8355da7c4be299e3f2bea1d023258e75b3a240d18bb066a521e923ee35979aa0',
]
rams = [load(name + '-ram.bin') for name in names]
assert all(len(ram) == 655360 for ram in rams)
assert [digest(ram) for ram in rams] == expected
original = game[0x1F997:0x1F9D0]
loaded = rams[2][(0x2A8E << 4) + 0x19B1:(0x2A8E << 4) + 0x19EA]
assert digest(original) == '76347194a33838c597b3fba609cb594dcd17a78bf4d74883330276032344a9c5'
assert digest(loaded) == '7d004b43fc63f8a7b130244c05a3f204b131bfdaa1cb0dea4567e56c21c6cfef'
assert [i for i, (a, b) in enumerate(zip(original, loaded)) if a != b] == [7, 8]
assert digest(game[0x1EAD2:0x1EEAA]) == '715424407210de3824c3a0218cc29033f04bb357ec718aa24a7995d9452d1e36'
word = lambda ram, off: int.from_bytes(ram[(0x1D16 << 4) + off:(0x1D16 << 4) + off + 2], 'little')
assert [word(ram, 0x3BFD) for ram in rams[1:]] == [8, 8, 8]
assert [word(ram, 0x3BFF) for ram in rams[1:]] == [0x449A] * 3
stack = [word(rams[2], 0x37AA + 2 * i) for i in range(7)]
assert stack == [0x2A8E, 1, 1, 8, 0x449A, 0x3BD4, 0x1D16]
print('caller far pointer:', f'{word(rams[3], 0x3BFF):04X}:{word(rams[3], 0x3BFD):04X}')
print('record-call stack words:', [f'{value:04X}' for value in stack])
PY
```

Original images, save, RAM dumps, screenshot and debugger stream remain
under ignored `resources/`; no game bytes or captures enter Git.
