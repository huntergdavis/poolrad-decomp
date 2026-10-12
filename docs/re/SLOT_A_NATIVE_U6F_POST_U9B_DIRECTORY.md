# Saved slot-A continuation after u9B: directory service boundary

A fresh DOS v1.3 replay followed the first saved insertion after unit
`0x9B` returned: unit `0x6F:+1579 -> +157C RETF 4`, the caller's
`+19E6/+19E7` near call to `+02AE`, and the first external call
at `+02D0` to loaded SYSTEM `131C:17D6`. That call reached the
DOS make-directory wrapper and returned to `+02D5`; the following
IOResult call returned 5. The `+02AE` routine took its short exit
through `+032E RETF` to caller `+19EA`. This is a native
address/hash/numeric trace and a **bounded model branch comparison**,
not a full C CPU/RAM parity or end-to-end replay claim.

Before debugging, `deja "Poolrad saved slot A u6F 1579 157C 19E6 next
external call native after u9B return"` and the shorter
`deja "u6F 19E6"` found no exact indexed session. The merged
[u9B entry-to-return parity note](SLOT_A_NATIVE_U9B_ENTRY_RETURN_PARITY.md),
[u6F caller map](U6F_PARTY_CONTINUATIONS.md), and
[bounded post-insertion model](U6F_POSTINSERT_02AE.md) supplied the
candidate stops and model contract. This run independently checked
the actual call route, runtime target segments, frames, code hashes,
registers, and RAM images. p5's cold-handler composition stops
before this post-u9B route and has no shared source edit.

## Inputs and original/loaded code

The ignored private run is
`resources/oracle-runs/slot-a-native-u6f-post-u9b-040`, prepared
from main `30d7569b4e28f2ff9555f1333a54267940d73edb`.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| START.EXE | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A SAVGAMA.DAT | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | 47,259,176 | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

| Bounded span | Bytes / original instructions | Original SHA-256 | Loaded SHA-256 | Differing bytes |
| --- | ---: | --- | --- | ---: |
| u6F `+02AE..+032E`, GAME `0x1E294..0x1E314`, loaded `2A8E` | 129 / 51 | `ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539` | `70114e09285ff85b5c6176903f5d5640e427bed0ea52b41786aa19793c2629d7` | 16 relocated operand bytes |
| Caller `+19E6..+19E9`, loaded `2A8E` | 4 | `947fbf712ef50ab57afaade48476bb613c15d273084700784c0f2e3b0b331557` | same | 0 |
| SYSTEM directory wrapper `+17D6..+17E8`, original `0AF8`, loaded `131C` | 19 | `90731793b0ac2c364059b1e22581baf55595d3328f57b9257846664eb84b4f01` | same | 0 |

The original u6F far calls at `+02D0/+02D5` name
`0AF8:17D6` and `0AF8:04A2`; the live operands name
`131C:17D6` and `131C:04A2`. The live debugger directly
stopped at `131C:17D6`, whose wrapper calls its pathname
converter, sets DOS `AH=39h`, and invokes the DOS service. The
[existing source audit](U6F_POSTINSERT_02AE.md) bounds that
wrapper and its helpers; this run does not attribute each internal
write to a specific helper instruction.

## Direct CPU stops and frames

All values are hexadecimal. The EV command recorded every visible
16-bit general register, segment register, IP, SP, BP, and FLAGS at
each stop. FS=GS=0000 throughout. The ignored numeric transcript
`ev-numeric.tsv` has SHA-256
`a38d5dd2687e68e6415d633d46fa1735b87dbca00b83582927c4758bb29e607b`.

| Stop | CS:IP | DS,ES,SS | SP,BP | AX,BX,CX,DX | SI,DI,FLAGS |
| --- | --- | --- | --- | --- | --- |
| after u9B | `2A8E:1579` | `14A0,449A,1D16` | `379E,37AE` | `0007,0004,0005,0011` | `0001,0008,7246` |
| insertion RETF | `2A8E:157C` | `14A0,449A,1D16` | `37B0,3DCA` | `0007,0004,0005,0011` | `0001,0008,7246` |
| caller post-insertion | `2A8E:19E6` | `14A0,449A,1D16` | `37B8,3DCA` | `0007,0004,0005,0011` | `0001,0008,7246` |
| call to +02AE | `2A8E:19E7` | `14A0,449A,1D16` | `37B6,3DCA` | `0007,0004,0005,0011` | `0001,0008,7246` |
| +02AE entry | `2A8E:02AE` | `14A0,449A,1D16` | `37B4,3DCA` | `0007,0004,0005,0011` | `0001,0008,7246` |
| before mkdir | `2A8E:02D0` | `14A0,1D16,1D16` | `35AC,37B2` | `000A,35A0,0000,14A0` | `5385,36BB,7202` |
| mkdir entry | `131C:17D6` | `14A0,1D16,1D16` | `35A8,37B2` | `000A,35A0,0000,14A0` | `5385,36BB,7202` |
| after mkdir / before IOResult | `2A8E:02D5` | `14A0,1D16,1D16` | `35B0,37B2` | `0005,35A0,0000,3526` | `36BB,3531,7247` |
| after IOResult | `2A8E:02DA` | `14A0,1D16,1D16` | `35B0,37B2` | `0005,35A0,0000,3526` | `36BB,3531,7246` |
| short-exit epilogue | `2A8E:032B` | `14A0,1D16,1D16` | `35B0,37B2` | `0005,35A0,0000,3526` | `36BB,3531,7246` |
| before +02AE RETF | `2A8E:032E` | `14A0,1D16,1D16` | `37B4,3DCA` | `0005,35A0,0000,3526` | `36BB,3531,7246` |
| caller return | `2A8E:19EA` | `14A0,1D16,1D16` | `37B8,3DCA` | `0005,35A0,0000,3526` | `36BB,3531,7246` |

At `+157C`, `SS:[37B0/37B2]` is return
`2A8E:19E6`. At `+02AE`, `SS:[37B4/37B6]`
is return `2A8E:19EA`. The pre-directory argument
at `SS:[35AC/35AE]` is `1D16:36B0`; the SYSTEM
entry has saved return `2A8E:02D5` at
`SS:[35A8/35AA]`. These are live frame observations.

The live `DS:537A` Pascal length at `+02D0` is **11**,
so the already documented `Copy(...,1,length-1)` path passes
**10** characters to the directory wrapper. The pathname bytes
remain private. The directory wrapper returned AX=`0005` with
carry set and changed `DS:4426` from 0 to 5. The next
IOResult call returned AX=`0005` and cleared `DS:4426`
back to 0. The two source comparisons accept 5; no message,
resident display/input, or retry call was reached before
`+032E`.

## RAM images and bounded C model

Every image below is an ignored 655,360-byte
`MEMDUMPBIN 0000 0000 A0000` capture.

| RAM label | SHA-256 |
| --- | --- |
| `1579`, `157c`, `19e6` | `f6629766848b95bdf32db777302649d89c49c100458b81c12ab57615bdffa73a` |
| `19e7` | `05b39e635217cbac15f9ee29e40714479db678b553a2a88a38662c516e00e0dc` |
| `02ae` | `05e08b193645a5250cf60b364f46e3808a928c0379d549e137bbfa1746dcff2c` |
| `02d0` | `93ba55cde6ece70feaa9f85b158ac9ca76e4468d3b138807ee4be44d4738d52a` |
| `service-entry` | `5a30f869b3cfcbae85b0ebd06668ce72aaa386666e8f29f4bd94a65236b62fa7` |
| `02d5` | `1cf0602829d378fd4b1746f6a9a1bee0681cc50ccd9612288b62ed5c943fa875` |
| `02da` | `820c6ade81fa25e3d22c85884dafa2bdeec1188a54945cb75820a6a6e1670b14` |
| `032b`, `032e`, `19ea` | `022d473a65f4ce723f2d9f8e691bb17910f7087c1aa1cc3aafd6695382b04b97` |

From `+02AE` to `+032E`, native RAM has **28 net changed
bytes in five contiguous address runs**:
`0x0816E..0x0816F`, `0x20700..0x20703`,
`0x20709..0x2070F`, `0x20810..0x2081A`,
and `0x20910..0x20913`. The first address
`0x0816E` changes between `+02D0` and `+02D5`,
inside the reached SYSTEM call. This identifies an instruction
interval, not an exact writer. The `+032E` and `+19EA`
RAM images are identical.

The existing bounded `pr_u6f_postinsert_02ae` API accepts a
live Pascal span, host hooks, and an attempt guard. A private
strict C probe supplied the captured `DS:537A` span and
**assumed callbacks** that execute once: `make_directory`
reports callback completion while setting modeled `DS:4426`
to 5; `io_result` returns 5 and clears that word. Those
callback actions mirror native numeric observations but are host
substitutions, not a DOS filesystem implementation. The C result
was `OK`, attempts **1**, displays **0**, last IOResult **5**,
next source offset `+032E`; this matches the native branch.

The C API has no CPU state and no full-RAM input/output. A private
1 MiB host projection initialized from the `+02AE` 640 KiB
capture, with its uncaptured tail zero, changed only the error
word in the two callbacks and restored it to zero. Its final
first 640 KiB therefore retained the entry image and differed
from native `+032E` in the **28 bytes / five runs** above;
the first mismatch is `0x0816E`. The host tail remained zero,
but native RAM above `0x9FFFF` was not captured. Native ES,
AX, BX, CX, DX, SI, DI, and IP changed across this call, while
the C model exposes none of those returns. **Only the bounded
branch and callback order match; full CPU/RAM parity is open.**

## Private reproduction

Prepare a fresh ignored run with `tools/oracle/prepare_run.sh`
and the pinned inputs. Launch the pinned heavy DOSBox-X under a
PTY and private display. At the code-word prompt press Return;
load slot `a` from the main menu with `l`, `a`.
Gate on `BP 2A8E:19E3`, then arm
`2A8E:1579`, `157C`, `19E6`, `19E7`,
`02AE`, `02D0`, `02D5`, `02DA`,
`032B`, `032E`, `19EA`, and
`BP 131C:17D6`. At each reached stop record
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS`
and `MEMDUMPBIN 0000 0000 A0000`; move each
`MEMDUMP.BIN` to an ignored `ram-<label>.bin`
before resuming. The direct stop at `131C:17D6`
checks the loaded segment instead of assuming the original
`0AF8` segment survives relocation.

The private verifier below checks every capture, code span, frame,
branch value, and the bounded C projection. It emits no game
bytes or pathname.

```sh
run_dir=resources/oracle-runs/slot-a-native-u6f-post-u9b-040
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fPIC -shared -Iinclude \
  src/recovered/overlay_u6f_postinsert_02ae.c -o "$run_dir/libpostinsert.so"
PYTHONPATH=tools/overlay python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
from ctypes import (CDLL, Structure, c_uint8, c_uint16, c_size_t, c_int,
                    c_void_p, POINTER, CFUNCTYPE, byref, cast)
from resident_loader_map import unpack

r = Path('resources/oracle-runs/slot-a-native-u6f-post-u9b-040')
h = lambda b: sha256(b).hexdigest()
for name, size, digest in (
    ('START.EXE', 65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    ('GAME.OVR', 206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    ('SAVGAMA.DAT', 13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
):
    b = (r / 'POOLRAD' / name).read_bytes()
    assert (len(b), h(b)) == (size, digest)
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), h(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
digest = {
    '1579': 'f6629766848b95bdf32db777302649d89c49c100458b81c12ab57615bdffa73a',
    '157c': 'f6629766848b95bdf32db777302649d89c49c100458b81c12ab57615bdffa73a',
    '19e6': 'f6629766848b95bdf32db777302649d89c49c100458b81c12ab57615bdffa73a',
    '19e7': '05b39e635217cbac15f9ee29e40714479db678b553a2a88a38662c516e00e0dc',
    '02ae': '05e08b193645a5250cf60b364f46e3808a928c0379d549e137bbfa1746dcff2c',
    '02d0': '93ba55cde6ece70feaa9f85b158ac9ca76e4468d3b138807ee4be44d4738d52a',
    'service-entry': '5a30f869b3cfcbae85b0ebd06668ce72aaa386666e8f29f4bd94a65236b62fa7',
    '02d5': '1cf0602829d378fd4b1746f6a9a1bee0681cc50ccd9612288b62ed5c943fa875',
    '02da': '820c6ade81fa25e3d22c85884dafa2bdeec1188a54945cb75820a6a6e1670b14',
    '032b': '022d473a65f4ce723f2d9f8e691bb17910f7087c1aa1cc3aafd6695382b04b97',
    '032e': '022d473a65f4ce723f2d9f8e691bb17910f7087c1aa1cc3aafd6695382b04b97',
    '19ea': '022d473a65f4ce723f2d9f8e691bb17910f7087c1aa1cc3aafd6695382b04b97',
}
ram = {}
for name, expected in digest.items():
    b = (r / ('ram-' + name + '.bin')).read_bytes()
    assert len(b) == 655360 and h(b) == expected
    ram[name] = b
transcript = (r / 'ev-numeric.tsv').read_bytes()
assert h(transcript) == 'a38d5dd2687e68e6415d633d46fa1735b87dbca00b83582927c4758bb29e607b'
rows = [line.split() for line in transcript.decode().splitlines()]
header = rows[0]
assert [row[0] for row in rows[1:]] == list(digest)
assert all(len(row) == len(header) and int(row[5],16) == int(row[6],16) == 0 for row in rows[1:])
assert [(int(row[1],16),int(row[2],16)) for row in rows[1:]] == [
    (0x2A8E,0x1579),(0x2A8E,0x157C),(0x2A8E,0x19E6),(0x2A8E,0x19E7),
    (0x2A8E,0x02AE),(0x2A8E,0x02D0),(0x131C,0x17D6),(0x2A8E,0x02D5),
    (0x2A8E,0x02DA),(0x2A8E,0x032B),(0x2A8E,0x032E),(0x2A8E,0x19EA)]
p = lambda seg, off: seg * 16 + off
w = lambda b,seg,off: int.from_bytes(b[p(seg,off):p(seg,off)+2], 'little')
assert (w(ram['157c'],0x1D16,0x37B0),w(ram['157c'],0x1D16,0x37B2)) == (0x19E6,0x2A8E)
assert (w(ram['032e'],0x1D16,0x37B4),w(ram['032e'],0x1D16,0x37B6)) == (0x19EA,0x2A8E)
assert [w(ram['service-entry'],0x1D16,o) for o in (0x35A8,0x35AA,0x35AC,0x35AE)] == [0x02D5,0x2A8E,0x36B0,0x1D16]
assert ram['02d0'][p(0x14A0,0x537A)] == 11
assert [w(ram[n],0x14A0,0x4426) for n in ('02d0','02d5','02da')] == [0,5,0]
game = (r / 'POOLRAD/GAME.OVR').read_bytes()
original, *_ = unpack((r / 'POOLRAD/START.EXE').read_bytes())
for source, loaded, want_source, want_loaded, changed in (
    (game[0x1E294:0x1E315],ram['02d0'][p(0x2A8E,0x02AE):p(0x2A8E,0x02AE)+129],
     'ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539',
     '70114e09285ff85b5c6176903f5d5640e427bed0ea52b41786aa19793c2629d7',16),
    (original[p(0x0AF8,0x17D6):p(0x0AF8,0x17D6)+19],
     ram['service-entry'][p(0x131C,0x17D6):p(0x131C,0x17D6)+19],
     '90731793b0ac2c364059b1e22581baf55595d3328f57b9257846664eb84b4f01',
     '90731793b0ac2c364059b1e22581baf55595d3328f57b9257846664eb84b4f01',0),
):
    assert (h(source),h(loaded),sum(a!=b for a,b in zip(source,loaded))) == (want_source,want_loaded,changed)
for off, original_segment, live_segment in ((0x02D0,0x0AF8,0x131C),(0x02D5,0x0AF8,0x131C)):
    src = game[0x1DFE6+off:0x1DFE6+off+5]
    dst = ram['02d0'][p(0x2A8E,off):p(0x2A8E,off)+5]
    assert (w(src,0,3),w(dst,0,3)) == (original_segment,live_segment)
MK = CFUNCTYPE(c_int,c_void_p,POINTER(c_uint8),c_size_t)
IO = CFUNCTYPE(c_int,c_void_p,POINTER(c_uint16))
DISPLAY = CFUNCTYPE(c_int,c_void_p,POINTER(c_uint8),c_size_t,c_uint8)
class Hooks(Structure):
    _fields_ = [('context',c_void_p),('make_directory',MK),('io_result',IO),('display_and_input',DISPLAY)]
class Result(Structure):
    _fields_ = [('attempts',c_size_t),('displays',c_size_t),('last_io_result',c_uint16),
                ('next_offset',c_uint16),('has_io_result',c_uint8)]
mem = bytearray(1 << 20)
mem[:655360] = ram['02ae']
at_error = p(0x14A0,0x4426)
at_live = p(0x14A0,0x537A)
calls = [0,0]
def mkdir(_,path,characters):
    assert characters == 10 and path[characters] == 0
    calls[0] += 1
    mem[at_error:at_error+2] = (5).to_bytes(2,'little')
    return 0  # callback completed; DOS result is read through IOResult
def io_result(_,result):
    assert int.from_bytes(mem[at_error:at_error+2],'little') == 5
    calls[1] += 1
    result[0] = 5
    mem[at_error:at_error+2] = bytes(2)
    return 0
mk,io = MK(mkdir),IO(io_result)
hooks = Hooks(None,mk,io,DISPLAY())
out = Result()
lib = CDLL(str((r / 'libpostinsert.so').resolve()))
fn = lib.pr_u6f_postinsert_02ae
fn.argtypes = [POINTER(c_uint8),c_size_t,POINTER(c_uint8),c_size_t,
               POINTER(c_uint8),c_size_t,POINTER(Hooks),c_size_t,POINTER(Result)]
fn.restype = c_int
buf = (c_uint8 * len(mem)).from_buffer(mem)
status = fn(cast(byref(buf,at_live),POINTER(c_uint8)),12,None,0,None,0,byref(hooks),1,byref(out))
assert status == 0 and (out.attempts,out.displays,out.last_io_result,out.next_offset,out.has_io_result) == (1,0,5,0x032E,1)
assert calls == [1,1] and mem[:655360] == ram['02ae'] and not any(mem[655360:])
different = [i for i,(a,b) in enumerate(zip(mem[:655360],ram['032e'])) if a != b]
ranges = []
for index in different:
    if not ranges or index > ranges[-1][1]+1: ranges.append([index,index])
    else: ranges[-1][1] = index
assert ranges == [[0x0816E,0x0816F],[0x20700,0x20703],[0x20709,0x2070F],[0x20810,0x2081A],[0x20910,0x20913]]
assert ram['02d0'][0x0816E] != ram['02d5'][0x0816E]
print('run-040: 12 hash-locked native stops; mkdir error 5; IOResult 5; exit +032E')
print('bounded C branch: OK; attempts 1; displays 0; modeled/native RAM gaps',len(different),'in',len(ranges),'runs; first',hex(different[0]))
print('CPU parity unavailable in the bounded C API; no full replay claim')
PY
```

Original media, save, pathname, screenshots, emulator logs, and RAM
images remain ignored under `resources/`. The next C integration
would need a real directory/IOResult host effect and an explicit
CPU/full-RAM continuation contract; the observed value 5 is an
oracle result, not a value to hardcode into a replay.
