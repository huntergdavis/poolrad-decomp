# Saved slot-A u9B entry-to-return native/C parity

A fresh native DOS v1.3 slot-A restore stopped at the first insertion's
unit `0x9B:+0000`, the reached `+0191 -> +0255` near call, the
`+01AC RETF 4`, and caller `0x6F:+1579`. With this run's exact
entry CPU tuple and 640 KiB RAM supplied to the already merged
`pr_u9b_entry_zero_0000`, its returned 14 modeled register/FLAGS
values and every one of the native return's 655,360 captured RAM bytes
match. The host model's `0xA0000..0xFFFFF` tail began at zero and
remained unchanged. Native FS and GS were zero at every stop; the C API
does not expose them. This is **bounded entry-to-return parity for this
saved invocation**, not the full save replay or a new C/count claim.

Before this replay, `deja "Poolrad saved slot A u9B entry 0000 RETF 4
u6F 1579 native C parity first insertion"` found no exact session;
`deja "u9B entry"` recovered today's cold-dispatch investigation and
the corrected `+0000` destination. The merged [cold first-insertion
CPU note](SLOT_A_NATIVE_FIRST_INSERT_COLD_CPU.md), [native dispatch
trace](U9B_RUNTIME_DISPATCH_0000.md), and [enclosing C composition
audit](U9B_ENTRY_ZERO_COMPOSITION.md) supplied candidate stops and
the C API. This run independently checked loaded code, all live CPU
stops, far/near frames, branch inputs, and RAM deltas.

## Pinned inputs and bounded code

The private run is ignored at
`resources/oracle-runs/slot-a-native-u9b-entry-return-039`. It
was prepared from the same pinned slot-A inputs on main
`24a4ad2fa8bbebf10798a7025e108ff3159f0e32`.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A SAVGAMA.DAT | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The existing original-byte audit was rerun. The GAME.OVR
`0x2AA87..0x2AC35` span is u9B `+0000..+01AE`, 431 bytes and
159 instructions, SHA-256
`260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71`.
The reached `+0255..+0328` leaf is 212 bytes and 86 instructions,
SHA-256
`f19017361e6ac6197e38eeee7b70f438d29962a34675ff43903ac712444dd326`.
Both loaded spans at `2CCD` match their original hashes byte for
byte. No original bytes are tracked here.

## Direct native stops and reached branch

All register values are hexadecimal; FS=GS=0000 at every row.
The columns cover every 16-bit register visible in the heavy
debugger. The RAM captures were made **before** executing each
listed instruction. The ignored numeric transcript `ev-numeric.tsv`
is SHA-256
`7d330811dd38f2bc4f3a9cf741acdd55fe1a7dcb40108a8b17c0d5366f1f4ec6`.

| Stop | CS:IP | DS,ES,SS | SP,BP | AX,BX,CX,DX | SI,DI,FLAGS |
| --- | --- | --- | --- | --- | --- |
| u9B entry | `2CCD:0000` | `14A0,3A16,1D16` | `3796,37AE` | `0000,001F,0007,449A` | `44BA,0000,7202` |
| index-zero class gate | `2CCD:00A7` | `14A0,449A,1D16` | `378C,3794` | `0000,001F,0000,0000` | `44BA,0008,7246` |
| near call to +0255 | `2CCD:0191` | `14A0,449A,1D16` | `3786,3794` | `0007,001F,0000,0000` | `44BA,0008,7246` |
| +0255 leaf entry | `2CCD:0255` | `14A0,449A,1D16` | `3784,3794` | `0007,001F,0000,0000` | `44BA,0008,7246` |
| leaf return site | `2CCD:0194` | `14A0,449A,1D16` | `378C,3794` | `0007,0004,0005,0011` | `0001,000F,7246` |
| record +9C gate | `2CCD:0197` | `14A0,449A,1D16` | `378C,3794` | `0007,0004,0005,0011` | `0001,0008,7246` |
| before RETF 4 | `2CCD:01AC` | `14A0,449A,1D16` | `3796,37AE` | `0007,0004,0005,0011` | `0001,0008,7246` |
| caller return | `2A8E:1579` | `14A0,449A,1D16` | `379E,37AE` | `0007,0004,0005,0011` | `0001,0008,7246` |

The entry far argument is `449A:0008` at
`SS:[379A/379C]`; return `2A8E:1579` is at
`SS:[3796/3798]`. At `+0255`, the reached near frame has
return `2CCD:0194` and the same far argument at
`SS:[3784..378A]`.

The saved record's signed `+0x96..+0x9D` class vector was
`[0,0,1,0,0,0,0,0]`; index zero was zero, so the
`+00A7` gate skipped `+00EC -> +01AF`. The enclosing loop
called `+0255` once at `+0191`. In that leaf only inner index
two had a positive class. After the leaf, record `+0x9C` was
zero, so `+0197` skipped `+01A6 -> +0329`. Breakpoints at
`+00EC` and `+01A6` did not fire before `+01AC`.

## RAM and C parity boundary

Each row is a separate 640 KiB private `MEMDUMPBIN` capture.

| Stop label / ignored RAM name | SHA-256 |
| --- | --- |
| `u9b-entry` | `22a19f5dddb23ca3f4d73cd55d6809ef231dcb7c5f058f159a114946eec04ab9` |
| `class-gate` | `f8473186d632d96e6240a912c2929b2c1f57819b081893695e9cebc9d772f379` |
| `call-0255` | `5ffc4900458a448bccf13c8c1d7e6aa286f1ff80359e99afb15468349bce7a6a` |
| `leaf-0255-entry` | `5c2e5cefb34c8978cc19f1b31b4bb190843919ca64e8ec5a4b7a86b11fc2c9c3` |
| `post-leaf-0194` | `7c91caa84e512a8238e33dbbdfa0fa378b386aa34a7f0db021f95ad2f3662a0a` |
| `field-gate-0197` | same as post-leaf |
| `u9b-preret` | same as post-leaf |
| `caller-return` | same as post-leaf |

Entry to caller return has 14 net changed RAM bytes in three
contiguous physical address runs:
`0x208E0..0x208E8`, `0x208EA..0x208EB`,
and `0x208F3..0x208F5`. These are stack addresses;
the record's first `0xC0` bytes have no net change. The
`+01AC` and caller-return RAM images are identical. Counts
describe snapshot differences, not unique writes.

The merged `pr_u9b_entry_zero_0000` returned
`PR_U9B_0000_OK`. From the captured entry image and exact CPU
tuple, its 640 KiB output SHA-256 was
`7c91caa84e512a8238e33dbbdfa0fa378b386aa34a7f0db021f95ad2f3662a0a`,
identical to the native caller return: **0 differing bytes**.
Its CS, IP, DS, ES, SS, SP, BP, AX, BX, CX, DX, SI, DI, and FLAGS
all equal the caller-return row. The host model's uncaptured
384 KiB tail was initialized to zero and had **0 changed bytes**.
Native RAM above `0x9FFFF` was not captured, so no parity claim
is made for that region.

## Reproduction with ignored inputs

Use `tools/oracle/prepare_run.sh` with the pinned ignored DOS tree.
Launch the pinned heavy DOSBox-X in a PTY and private display;
its debugger requires a PTY. At the code-word prompt press Return;
at the menu load `l`, slot `a`. Gate on
`BP 2A8E:19E3`, then arm
`BP 2CCD:0000`, `00A7`, `00EC`, `0191`,
`0255`, `0194`, `0197`, `01A6`, `01AC`,
and `BP 2A8E:1579`. At each reached stop record
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS`
and `MEMDUMPBIN 0000 0000 A0000`; rename each ignored
`MEMDUMP.BIN` to `ram-<label>.bin` before resuming.
The saved numeric `ev-numeric.tsv` uses the table's stop order.

The following private verifier uses only published C sources and
the ignored snapshots. It rechecks hashes, bounded loaded code,
branch values, and all modeled return registers. It emits only
numeric results and hashes:

```sh
run_dir=resources/oracle-runs/slot-a-native-u9b-entry-return-039
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fPIC -shared -Iinclude \
  src/recovered/overlay_u9b_entry_zero_0000.c \
  src/recovered/overlay_u9b_record_table_0255.c \
  src/recovered/overlay_u9b_record_fields_0329.c \
  -o "$run_dir/libu9b-parity.so"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
from ctypes import CDLL, Structure, c_uint8, c_uint16, c_size_t, c_int, POINTER, byref

r = Path('resources/oracle-runs/slot-a-native-u9b-entry-return-039')
h = lambda b: sha256(b).hexdigest()
inputs = {
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for name, want in inputs.items():
    data = (r / 'POOLRAD' / name).read_bytes()
    assert (len(data), h(data)) == want
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), h(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
digests = {
    'u9b-entry': '22a19f5dddb23ca3f4d73cd55d6809ef231dcb7c5f058f159a114946eec04ab9',
    'class-gate': 'f8473186d632d96e6240a912c2929b2c1f57819b081893695e9cebc9d772f379',
    'call-0255': '5ffc4900458a448bccf13c8c1d7e6aa286f1ff80359e99afb15468349bce7a6a',
    'leaf-0255-entry': '5c2e5cefb34c8978cc19f1b31b4bb190843919ca64e8ec5a4b7a86b11fc2c9c3',
    'post-leaf-0194': '7c91caa84e512a8238e33dbbdfa0fa378b386aa34a7f0db021f95ad2f3662a0a',
    'field-gate-0197': '7c91caa84e512a8238e33dbbdfa0fa378b386aa34a7f0db021f95ad2f3662a0a',
    'u9b-preret': '7c91caa84e512a8238e33dbbdfa0fa378b386aa34a7f0db021f95ad2f3662a0a',
    'caller-return': '7c91caa84e512a8238e33dbbdfa0fa378b386aa34a7f0db021f95ad2f3662a0a',
}
ram = {}
for name, digest in digests.items():
    data = (r / ('ram-' + name + '.bin')).read_bytes()
    assert len(data) == 655360 and h(data) == digest
    ram[name] = data
game = (r / 'POOLRAD/GAME.OVR').read_bytes()
for file_lo, file_hi, live_off, digest in (
    (0x2AA87, 0x2AC36, 0x0000, '260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71'),
    (0x2ACDC, 0x2ADB0, 0x0255, 'f19017361e6ac6197e38eeee7b70f438d29962a34675ff43903ac712444dd326'),
):
    source = game[file_lo:file_hi]
    live = ram['u9b-entry'][0x2CCD0 + live_off:0x2CCD0 + live_off + len(source)]
    assert h(source) == h(live) == digest
rec = 0x449A0 + 8
assert list(ram['u9b-entry'][rec + 0x96:rec + 0x9E]) == [0,0,1,0,0,0,0,0]
assert ram['post-leaf-0194'][rec + 0x9C] == 0
assert ram['u9b-entry'][rec:rec + 0xC0] == ram['caller-return'][rec:rec + 0xC0]
transcript = (r / 'ev-numeric.tsv').read_bytes()
assert h(transcript) == '7d330811dd38f2bc4f3a9cf741acdd55fe1a7dcb40108a8b17c0d5366f1f4ec6'
rows = [line.split() for line in transcript.decode().splitlines()]
header = rows[0]
assert [row[0] for row in rows[1:]] == list(digests)
assert [(int(row[header.index('CS')], 16), int(row[header.index('IP')], 16)) for row in rows[1:]] == [
    (0x2CCD,0),(0x2CCD,0xA7),(0x2CCD,0x191),(0x2CCD,0x255),
    (0x2CCD,0x194),(0x2CCD,0x197),(0x2CCD,0x1AC),(0x2A8E,0x1579)]
assert all(int(row[header.index('FS')],16) == int(row[header.index('GS')],16) == 0 for row in rows[1:])
fields = 'ax bx cx dx si di bp sp cs ip ds es ss flags'.split()
class CPU(Structure):
    _fields_ = [(name, c_uint16) for name in fields]
def cpu_at(row):
    return CPU(*(int(row[header.index(name.upper())],16) for name in fields))
cpu = cpu_at(rows[1])
expected_cpu = cpu_at(rows[-1])
mem = bytearray(1 << 20)
mem[:655360] = ram['u9b-entry']
lib = CDLL(str((r / 'libu9b-parity.so').resolve()))
fn = lib.pr_u9b_entry_zero_0000
fn.argtypes = [POINTER(c_uint8), c_size_t, POINTER(CPU)]
fn.restype = c_int
status = fn((c_uint8 * len(mem)).from_buffer(mem), len(mem), byref(cpu))
actual = tuple(getattr(cpu, name) for name in fields)
want = tuple(getattr(expected_cpu, name) for name in fields)
different = [i for i, (x,y) in enumerate(zip(mem[:655360], ram['caller-return'])) if x != y]
tail_changed = sum(bool(x) for x in mem[655360:])
assert status == 0 and actual == want and not different and tail_changed == 0
assert h(mem[:655360]) == digests['caller-return']
changed = [i for i, (x,y) in enumerate(zip(ram['u9b-entry'], ram['caller-return'])) if x != y]
ranges = []
for index in changed:
    if not ranges or index > ranges[-1][1] + 1:
        ranges.append([index,index])
    else:
        ranges[-1][1] = index
assert ranges == [[0x208E0,0x208E8],[0x208EA,0x208EB],[0x208F3,0x208F5]]
print('u9B saved parity: status 0; 640KiB differences 0; CPU/FLAGS equal; host tail changes 0')
print('entry-to-return net changed RAM bytes', len(changed), 'address runs', len(ranges))
PY
```

All original media, save data, emulator output, RAM images, and
private captures remain ignored under `resources/`. The first
insertion's preceding cold loader and subsequent caller effects are
separate boundaries. This note does not establish C-native
end-to-end one-Up parity.
