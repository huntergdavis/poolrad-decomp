# Saved slot-A directory service: first RAM writer and callback boundary

Three fresh DOS v1.3 replays isolated the first value change at linear guest
RAM `0x0816E` in the saved post-u9B unit `0x6F:+02AE` directory path.
Run 041 armed a linear memory watch at `+02D0`; runs 042 and 043 bracketed
the reached SYSTEM and IOResult instructions. All observations here are
addresses, numeric state, hashes, and counts. This note proposes a C boundary;
it does **not** implement it or claim full C/native parity, a strict full-C
routine, or a game-function count change.

Before debugging, `deja "Poolrad SYSTEM directory IOResult 0x0816E +02D0
first writer RAM DOSBox-X saved slot A"` and `deja "u6F postinsert mkdir
0816E writer callback CPU RAM"` returned no indexed match. The merged
[post-u9B continuation note](SLOT_A_NATIVE_U6F_POST_U9B_DIRECTORY.md)
provided the `+02D0`, `+02D5`, and `+02DA` candidate stops; this work
independently checked the reached loaded SYSTEM code and divided its effects
by instruction boundary. p5 owns the earlier `+0338` cold loader and confirmed
there is no source or interface overlap.

## Hash-locked code and route

Each ignored run was prepared separately with `tools/oracle/prepare_run.sh`:
`slot-a-native-directory-effect-041`, `-042`, and `-043` under
`resources/oracle-runs/`. The input hashes were identical in all three.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A SAVGAMA.DAT | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The original SYSTEM segment is `0AF8`; the reached loaded segment is
`131C`. These original spans were compared byte for byte with the loaded
RAM image; each had zero differences:

| SYSTEM span | Bytes / instructions | Original and loaded SHA-256 |
| --- | ---: | --- |
| Directory wrapper `+17D6..+17E8` | 19 / 9 | `90731793b0ac2c364059b1e22581baf55595d3328f57b9257846664eb84b4f01` |
| DOS-call helper `+181B..+1829` | 15 / 9 | `978fabd0bca91df715152943c57b87c26e835dbd9533fbb60843e32bfc86908b` |
| IOResult `+04A2..+04A8` | 7 / 3 | `d70153d7282a717eca08c439be05fb911569333751907973b73ca0b475e08d1b` |

The reached wrapper at `131C:17D6` calls the pathname converter at
`+1800`, sets AH=`39h`, and calls `+181B`. That helper saves DS,
sets DX to the stack pathname, makes DS=SS, executes `INT 21h` at
`131C:1821`, restores DS at `+1823`, branches on carry at `+1824`,
and, when carry is set, stores AX to `DS:4426` at `+1826`.
The wrapper returns to unit `0x6F:+02D5`. The reached IOResult at
`131C:04A2` zeroes AX, exchanges AX with `DS:4426` at `+04A4`,
and returns far at `+04A8` to unit `0x6F:+02DA`.

## First value-changing writer

In run 041, `BPLM 0816E` was armed while stopped at `2A8E:02D0`.
The heavy debugger initializes a new memory breakpoint's comparison value
to zero. Its `00 -> 8E` hit at `131C:17D7` therefore only synchronized the
watch with the already present byte; the pre-service RAM image also has
`0x8E` at `0x0816E`. The **first real value change** was
`0x8E -> 0x0A`, detected at `F000:D105 IRET`. The adjacent byte
`0x0816F` changed `0x3D -> 0x35`. At that stop, the interrupt frame at
`SS:351C/351E/3520` held `1823/131C/7247`; AX was 5.
The watch is polled at instruction boundaries, so `F000:D105` is the
*detection* PC, not a claimed guest store instruction.

Run 042 proves the narrower causal interval with direct stops before
`131C:1821 INT 21h` and after it at `131C:1823`, before any later
wrapper instruction. The same two bytes changed across that single
interrupt operation. Its other 21 changed bytes are in two DOS stack runs,
`0x2066B..0x2066D` and `0x20670..0x20681`; the total is
**23 bytes in three runs**. `DS:4426` stayed zero. At `+1821`, AH was
`39h`, DS=SS=`1D16`, DX=`3526`, SP=`3522`, and FLAGS=`7246`.
At `+1823`, AX was 5 and FLAGS=`7247` (carry set); the other listed
registers and SP were unchanged. `SS:3522/3524` held `14A0/17E5`,
the saved DS and near return to the wrapper. The earliest **guest
operation** responsible for the change is `131C:1821 INT 21h`.
The service executes inside DOSBox-X between those guest PCs; no internal
guest CS:IP or separate guest write instruction was observed or inferred.

The post-INT and pre-store (`131C:1826`) RAM images are identical.
Executing the guest `MOV DS:[4426],AX` at `+1826` changes exactly
one byte, linear `0x18E26`, from 0 to 5. Its high word byte stays zero.
The post-store and unit `0x6F:+02D5` RAM images are identical. This
separates the DOS service's low-memory and stack effects from the Pascal
wrapper's error-word store. Value 5 is this run's DOS result, not a
constant to use in C.

| Run / stop | CS:IP | DS,SS,SP | AX,DX,FLAGS | 640 KiB RAM SHA-256 |
| --- | --- | --- | --- | --- |
| 041 pre-service | `2A8E:02D0` | `14A0,1D16,35AC` | `000A,14A0,7202` | `988634cfcb12a872c0d8612f14a52415a428c6a080bce3ebfc1904d8ac0a4bfb` |
| 041 watch initialized | `131C:17D7` | — | — | `7b4ebc62a6aa3f82e5147cd5b51881eeb1e045b328353f5dde158b2905784e71` |
| 041 first real watch | `F000:D105` | `1D16,1D16,351C` | `0005,3526,7246` | `f2e0f5286e351791265738e48e67bb067f1e0df7bb1a9082b86c56c3b41f246f` |
| 042 pre-INT | `131C:1821` | `1D16,1D16,3522` | `3900,3526,7246` | `f87022341b6c711643f8e32e2eb68895a9656cf37063c3d4472a324402e0adcc` |
| 042 post-INT | `131C:1823` | `1D16,1D16,3522` | `0005,3526,7247` | `6ac1ef57e3baaaaa38ee202201bd0fdfeb914692b3f6c2a73974867fcc36aeb5` |
| 042 pre-store | `131C:1826` | `14A0,1D16,3524` | `0005,3526,7247` | same as post-INT |
| 042 post-store | `131C:1829` | — | — | `f5ff75ca8cfaf85b037956be2e7e34066b45053f6cf0e853061c791d3aac9c95` |
| 042 wrapper return | `2A8E:02D5` | `14A0,1D16,35B0` | `0005,3526,7247` | same as post-store |

The two dashed rows have a direct debugger PC and RAM capture but no
separately transcribed EV tuple. The other rows show selected fields from
direct EV commands, which also sampled ES, FS, GS, BP, BX, CX, SI, and DI.

## Direct IOResult entry and exit

IOResult is called elsewhere during startup. Run 043 therefore gated
first on `2A8E:02D5`, then armed `131C:04A2`, `+04A8`, and
`2A8E:02DA`. The far call wrote four return-frame bytes at
`0x2070C..0x2070F` before IOResult entry. At `+04A2`, AX=5,
FLAGS=`7247`, SP=`35AC`, and `DS:4426`=5. By `+04A8`,
the XOR/XCHG sequence had changed **only** `DS:4426` at linear
`0x18E26` from 5 to 0; AX remained 5 and FLAGS became `7246`.
`RETF` advanced SP to `35B0` and returned to `2A8E:02DA` without
further RAM change. This proves the modeled IOResult exchange as a
separate reached effect; it does not assign DOS service writes to IOResult.

| Run 043 stop | CS:IP | AX,SP,FLAGS | 640 KiB RAM SHA-256 |
| --- | --- | --- | --- |
| before far call | `2A8E:02D5` | — | `fb9a6688979542dd31a75bd38b4dd8046d65b9d2c5407e48c4b2f59bdc47dcb5` |
| IOResult entry | `131C:04A2` | `0005,35AC,7247` | `ecfacd1a14664c66139982e4fd6c739ac49a60553bd6e850defad9c0cbcd88e7` |
| after exchange | `131C:04A8` | `0005,35AC,7246` | `b38d8589331ddc347e209ae43837198099c53315426628ec4a172221c02232ec` |
| caller return | `2A8E:02DA` | `0005,35B0,7246` | same as after exchange |

## Proposed C callback contract

The existing `pr_u6f_postinsert_02ae` hooks report callback completion
and IOResult value but do not carry CPU or full guest RAM. A replay-capable
interface should add a separate guest-state path without changing that
bounded API. At the reached `INT 21h` operation, pass the complete 16-bit
CPU register/segment/FLAGS state and a bounded guest-memory accessor to a
DOS-service callback. Its input PC must be `131C:1821`, AH=`39h`,
and DS:DX must address the wrapper's stack pathname. It returns either
an explicit `BLOCKED_DOS_SERVICE` **without executing the interrupt**
or a complete event: post-interrupt CPU state at `131C:1823` and a
validated ordered RAM write journal. The event owns all service effects,
including low memory and DOS stack writes, not only AX/CF. Its AX, carry,
and writes come from the service implementation or a supplied oracle
event; they are never fixed to this run's result 5.

A concrete new interface can take a read-only pre-state, a bounded
memory view, and an output event, with no edits to the current hook type:

```c
pr_status pr_dos_int21_39_event(void *context,
                              const pr_guest_cpu16 *before,
                              const pr_guest_memory_view *memory,
                              pr_guest_cpu16 *after,
                              pr_guest_write *writes,
                              size_t write_capacity,
                              size_t *write_count);
```

`pr_guest_write` records a linear address plus old and new byte in
execution order, including repeated writes to the same address. The
reducer checks address bounds and each old value before applying a
completed event, then checks `after->cs:ip == 131C:1823`. A missing
callback returns `BLOCKED_DOS_SERVICE` with the reached pre-INT CPU/RAM
intact; wrapper effects already completed before `+1821` remain intact.

The C wrapper then executes the known guest instructions: restore DS;
branch on the returned carry; store returned AX into `DS:4426` only
on the carry path; unwind the wrapper frame. The C IOResult path executes
`AX=0; XCHG AX,DS:4426; RETF`, including FLAGS and stack changes.
Each transition accepts a full CPU/RAM input snapshot and produces a full
CPU/RAM output snapshot, so a replay harness can compare every modeled
register and all captured RAM after `+1823`, `+1829`, `+02D5`,
`+04A8`, and `+02DA`. The guest-memory accessor must define segment
translation, capacity, and write bounds; the current native captures
cover only `0x00000..0x9FFFF`, so full 1 MiB RAM parity still needs
a larger capture. Synthetic tests should supply success, error 5,
and a different error to verify the carry branch and IOResult exchange
without baking in a single oracle outcome.

Private reproduction: prepare each ignored run with
`tools/oracle/prepare_run.sh`, launch the pinned heavy DOSBox-X through
`tools/oracle/launch.sh` under a private Xvfb display, select the saved
slot A, and use `EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS`
plus `MEMDUMPBIN 0000 0000 A0000` at each listed stop. For run 041,
arm `BP 2A8E:02D0`, then `BPLM 0816E` and
`BP 131C:17D6` at that stop; discard only the watch's
`00 -> 8E` initialization hit. For run 042, arm
`BP 131C:1821`, `BP 131C:1823`, `BP 131C:1826`, and
`BP 131C:1829`. For run 043, gate on `BP 2A8E:02D5`
*before* arming `BP 131C:04A2`, `BP 131C:04A8`, and
`BP 2A8E:02DA`, because IOResult has earlier callers.
The save, pathname, RAM dumps, screenshots, emulator output, and
extracted code stay ignored under `resources/`.

The private verifier below checks the pinned inputs, exact original/loaded
code, RAM image hashes, stack frames, and numeric deltas. It prints no
game bytes or pathname.

```sh
PYTHONPATH=tools/overlay python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
from resident_loader_map import unpack

root = Path('resources/oracle-runs')
run = {n: root / ('slot-a-native-directory-effect-' + n)
       for n in ('041', '042', '043')}
h = lambda b: sha256(b).hexdigest()
inputs = {
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for directory in run.values():
    for name, expected in inputs.items():
        b = (directory / 'POOLRAD' / name).read_bytes()
        assert (len(b), h(b)) == expected
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), h(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
digests = {
    '041': {
        '02d0': '988634cfcb12a872c0d8612f14a52415a428c6a080bce3ebfc1904d8ac0a4bfb',
        'watch-initial': '7b4ebc62a6aa3f82e5147cd5b51881eeb1e045b328353f5dde158b2905784e71',
        'first-watch': 'f2e0f5286e351791265738e48e67bb067f1e0df7bb1a9082b86c56c3b41f246f',
        '02d5': '196b852feaf3758818542e8b36c1be465656e60e4358578a93aa6db33e0d0503',
        '02da': '72f5698be083f17010953e45f42e2593d526e2f58980d93a1b1ffddad809ada0',
    },
    '042': {
        'pre-int': 'f87022341b6c711643f8e32e2eb68895a9656cf37063c3d4472a324402e0adcc',
        'post-int': '6ac1ef57e3baaaaa38ee202201bd0fdfeb914692b3f6c2a73974867fcc36aeb5',
        'pre-store': '6ac1ef57e3baaaaa38ee202201bd0fdfeb914692b3f6c2a73974867fcc36aeb5',
        'post-store': 'f5ff75ca8cfaf85b037956be2e7e34066b45053f6cf0e853061c791d3aac9c95',
        '02d5': 'f5ff75ca8cfaf85b037956be2e7e34066b45053f6cf0e853061c791d3aac9c95',
        '02da': '9526402aaa4fac11510115f364fe054c015f18b9a265b665ce6b171a1e4f115e',
    },
    '043': {
        '02d5': 'fb9a6688979542dd31a75bd38b4dd8046d65b9d2c5407e48c4b2f59bdc47dcb5',
        'io-entry': 'ecfacd1a14664c66139982e4fd6c739ac49a60553bd6e850defad9c0cbcd88e7',
        'io-after-xchg': 'b38d8589331ddc347e209ae43837198099c53315426628ec4a172221c02232ec',
        '02da': 'b38d8589331ddc347e209ae43837198099c53315426628ec4a172221c02232ec',
    },
}
ram = {}
for n, images in digests.items():
    ram[n] = {}
    for stop, expected in images.items():
        b = (run[n] / ('ram-' + stop + '.bin')).read_bytes()
        assert len(b) == 0xA0000 and h(b) == expected
        ram[n][stop] = b
original, *_ = unpack((run['042'] / 'POOLRAD/START.EXE').read_bytes())
for off, size, digest, loaded in (
    (0x17D6, 19, '90731793b0ac2c364059b1e22581baf55595d3328f57b9257846664eb84b4f01', ram['042']['pre-int']),
    (0x181B, 15, '978fabd0bca91df715152943c57b87c26e835dbd9533fbb60843e32bfc86908b', ram['042']['pre-int']),
    (0x04A2, 7, 'd70153d7282a717eca08c439be05fb911569333751907973b73ca0b475e08d1b', ram['043']['io-entry']),
):
    source = original[0x0AF80 + off:0x0AF80 + off + size]
    live = loaded[0x131C0 + off:0x131C0 + off + size]
    assert h(source) == h(live) == digest
def changes(a, b):
    return [i for i, (x, y) in enumerate(zip(a, b)) if x != y]
def runs(addresses):
    out = []
    for i in addresses:
        if not out or i > out[-1][1] + 1: out.append([i, i])
        else: out[-1][1] = i
    return out
w = lambda b, seg, off: int.from_bytes(b[seg * 16 + off:seg * 16 + off + 2], 'little')
a = ram['041']['watch-initial']; b = ram['041']['first-watch']
assert (a[0x0816E], b[0x0816E], a[0x0816F], b[0x0816F]) == (0x8E, 0x0A, 0x3D, 0x35)
assert [w(b, 0x1D16, o) for o in (0x351C, 0x351E, 0x3520)] == [0x1823, 0x131C, 0x7247]
a = ram['042']['pre-int']; b = ram['042']['post-int']
d = changes(a, b)
assert len(d) == 23 and runs(d) == [[0x0816E,0x0816F],[0x2066B,0x2066D],[0x20670,0x20681]]
assert [w(a, 0x1D16, o) for o in (0x3522, 0x3524)] == [0x14A0, 0x17E5]
assert w(a, 0x14A0, 0x4426) == w(b, 0x14A0, 0x4426) == 0
assert b == ram['042']['pre-store']
assert changes(b, ram['042']['post-store']) == [0x18E26]
assert w(ram['042']['post-store'], 0x14A0, 0x4426) == 5
assert ram['042']['post-store'] == ram['042']['02d5']
assert changes(ram['043']['02d5'], ram['043']['io-entry']) == list(range(0x2070C, 0x20710))
assert changes(ram['043']['io-entry'], ram['043']['io-after-xchg']) == [0x18E26]
assert w(ram['043']['io-entry'], 0x14A0, 0x4426) == 5
assert w(ram['043']['io-after-xchg'], 0x14A0, 0x4426) == 0
assert ram['043']['io-after-xchg'] == ram['043']['02da']
print('directory INT21: 23 changed bytes; first 0x0816E; wrapper store: one byte')
print('IOResult: four far-call frame bytes, one error-word byte, zero RETF RAM bytes')
PY
```
