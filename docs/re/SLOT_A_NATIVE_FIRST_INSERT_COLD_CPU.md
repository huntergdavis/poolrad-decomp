# Saved slot-A first insertion CPU and cold overlay dispatch

Two fresh native DOS v1.3 replays of the same hash-pinned slot-A save
measured the first party-insertion entry at unit `0x6F:+1446`, its
`+1574` far-call boundary, and the cold `INT 3Fh` path to unit
`0x9B:+0000`. Run 037 captured the saved caller and all three CPU
endpoints. Run 038 stopped inside the actual loaded handler and
loader. The two runs agree on their `+1574`, stub, and u9B entry
register tuples and on the net 1,214 changed RAM bytes / 20 address
runs from pre-call to u9B entry. These counts are differences between
640 KiB snapshots, not unique writes or a translation claim. The
guest stopped before the u9B body executed; cold-loader parity and
C-native composition remain open. No routine count is added.

Before this investigation, `deja "Poolrad saved slot A unit 6F 1446
1574 cold 00AC 0025 unit 9B entry CPU native"` found no indexed
session. The merged [first-insertion caller watch](SLOT_A_NATIVE_19E3_INSERTION_RETURN.md),
[corrected u6F-to-u9B transfer map](U6F_U9B_TRANSFER_1574.md), and
[cold-boundary audit](U6F_INSERTION_COLD_BOUNDARY.md) supplied the
candidate stops and original-byte leads. This work rechecked the
original and loaded spans, live vector, caller frame, all register
tuples, and same-run RAM deltas.

## Pinned inputs and original/loaded boundaries

Both private runs, full RAM, numeric transcripts, and captures are
ignored at `resources/oracle-runs/slot-a-native-first-insert-cold-037`
and `resources/oracle-runs/slot-a-native-first-insert-handler-038`.
The heavy DOSBox-X binary is 47,259,176 bytes, SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Input in each run | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |

The original resident hashes below come from expanding the pinned
START.EXE with `tools/overlay/resident_loader_map.py:unpack`; the
loaded hashes come from the private RAM snapshots. All bounds are
inclusive. The original unit `0x6F` begins at GAME file `0x1DFE6`
and was loaded at `2A8E:0000`.

| Bounded span | Bytes / original instructions | Original SHA-256 | Loaded SHA-256 | Differing bytes |
| --- | ---: | --- | --- | ---: |
| u6F `+1446..+157E`, GAME `0x1F42C..0x1F564` | 313 / 99 | `370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245` | `cd7d4b58fff401796a53d4779c4c55e5a731215f5eecf16f304a25b4b48a9dad` | 6 |
| u6F far call `+1574..+1578` | 5 / 1 | `abb0f3304fa196212ca007e1e16d7aa1983aabec90922c21d6ea93d50db07755` | `ebfa5b38184f9ef3235637ee56b73993ac80e11249023ba228d4fb790e7e8ecb` | 2 |
| Stub `+0025..+0029`, original `00AC`, loaded `08D0` before INT | 5 / descriptor | `d67da1e87a1a5cac157296d1e710a6673b63b369f977f13bc9426973402e8103` | same | 0 |
| Stub `08D0:+0025..+0029` after handler patch | 5 / 1 jump | original as above | `f52886ae72859b89ac8532f455e353655450490b518e17fc607ab12698fdea30` | 4 |
| INT 3Fh handler `+02E6..+0337`, original `0AA2`, loaded `12C6` | 82 | `6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c` | `5c69a0e6a0a9141fae2d0381a4ebe4252981d54cbad01d335a3f5f73380fd6ef` | 2, at `+02F2/+02F3` |
| Handler loader `+0338..+03E8`, original `0AA2`, loaded `12C6` | 177 | `58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991` | same | 0 |
| u9B `+0000..+01AE`, GAME `0x2AA87..0x2AC35`, loaded `2CCD` | 431 / 159 | `260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71` | same | 0 |

The original call operand is `00AC:0025`; its runtime segment was
`08D0`, so `2A8E:+1574` called `08D0:0025`. The live interrupt
vector at physical `0x00FC..0x00FF` was `12C6:02E6`, rather than
the handler's original image segment `0AA2`. An initial attempt to
break at `0AA2:02E6` missed the handler; the fresh run 038 armed
`12C6:02E6` and stopped there directly. The patched stub jumped to
`2CCD:0000`, agreeing with the original `+0000` descriptor. These
loaded addresses are measured for this pinned replay.

## Saved first-insertion CPU and frame

All values below are hexadecimal. Tuple columns cover every 16-bit
general register, all visible segment registers, IP, SP, BP, and FLAGS.
`FS=GS=0000` at all stops. Run 037 directly stopped at the saved
caller `+19E3`, then `+1446`, `+1574`, the cold stub, its patched
return, and u9B entry, in that order.

| Direct stop | `CS:IP` | `DS,ES,SS` | `SP,BP` | `AX,BX,CX,DX` | `SI,DI,FLAGS` |
| --- | --- | --- | --- | --- | --- |
| Caller before `CALL +1446` | `2A8E:19E3` | `14A0,1D16,1D16` | `37B2,3DCA` | `3E01,0006,0024,0000` | `44BA,3626,7206` |
| First `+1446` entry | `2A8E:1446` | `14A0,1D16,1D16` | `37B0,3DCA` | `3E01,0006,0024,0000` | `44BA,3626,7206` |
| Pre-far-call `+1574` | `2A8E:1574` | `14A0,3A16,1D16` | `379A,37AE` | `0000,001F,0007,449A` | `44BA,0000,7202` |
| Cold stub before `INT 3Fh` | `08D0:0025` | `14A0,3A16,1D16` | `3796,37AE` | `0000,001F,0007,449A` | `44BA,0000,7202` |
| Patched stub after handler | `08D0:0025` | `14A0,3A16,1D16` | `3796,37AE` | `0000,001F,0007,449A` | `44BA,0000,7202` |
| u9B body entry | `2CCD:0000` | `14A0,3A16,1D16` | `3796,37AE` | `0000,001F,0007,449A` | `44BA,0000,7202` |

At `+1574`, argument words at `SS:[379A/379C]` were offset
`0008` and segment `449A`. At the stub, the far call had pushed
return IP `1579` at `SS:[3796]` and CS `2A8E` at `[3798]`.
At loaded handler entry `12C6:02E6`, `SS:SP=1D16:3790` and
`FLAGS=7002`; the INT frame held saved IP `0027`, CS `08D0`,
and flags `7202` at `SS:[3790/3792/3794]`. At u9B entry,
`SS:[3790]` retained the rewound value `0025`; `SS:[3796..379C]`
still held return `2A8E:1579` and argument `449A:0008`.
Those are memory observations at stops; the exact transient writes
inside the loader were not attributed instruction by instruction.

## Direct cold-handler stops and RAM differences

Run 038 directly stopped at every row below. The full 16-bit CPU
tuples are in its ignored `ev-numeric.tsv`; the key changing values
are listed to expose the handler's frame and loader return. At all
rows, `DS=14A0`, `SS=1D16`, and `FS=GS=0000`.

| Stop | `CS:IP` | `ES,SP,BP` | `AX,BX,CX,DX,SI,DI,FLAGS` | Full-RAM SHA-256 |
| --- | --- | --- | --- | --- |
| Pre-call | `2A8E:1574` | `3A16,379A,37AE` | `0000,001F,0007,449A,44BA,0000,7202` | `d162e45045a2a1b8c54c58ac69321a0bd0a250a15116ed1fe31e425d2f1df758` |
| Cold stub | `08D0:0025` | `3A16,3796,37AE` | `0000,001F,0007,449A,44BA,0000,7202` | `da26d1e2ba18ad9efc138cd8163df2394ade35a674364b45522eaff0529bf416` |
| Handler entry | `12C6:02E6` | `3A16,3790,37AE` | `0000,001F,0007,449A,44BA,0000,7002` | `9168e596446be367a2190b0956fb6fc157fd7691331790a8708ebbaba7fd3cf5` |
| Pre-loader call | `12C6:0314` | `08D0,377C,3794` | `37AE,0027,0007,449A,44BA,0000,7212` | `de9fe269a5d64ed092982ca95e99ed70257506dd9dedb73be24e763ab46072cc` |
| Loader entry | `12C6:0338` | `08D0,377A,3794` | `37AE,0027,0007,449A,44BA,0000,7212` | `25ab3944a13b04dcfb8b43378fdab160f5f841aa2da812d9f5036aad00462b4f` |
| Post-loader return | `12C6:0317` | `08D0,377C,3794` | `00BB,2CCD,0859,0329,44BA,0039,7206` | `84b3901c8173ef048ea3051ba4345be909266ad31f9dba0518aff2d8d27ef226` |
| Pre-indirect call | `12C6:032A` | `08D0,377E,3794` | `FFFF,0000,0859,0329,44BA,0039,7297` | `290daa4a00aa7e8942092d05e87022126992cd8db607b1fc2fc427f1bf9d51c4` |
| Patched stub | `08D0:0025` | `3A16,3796,37AE` | `0000,001F,0007,449A,44BA,0000,7202` | `6e64a1a92f70b562bde20beadbabd40c3256d2b2b2829b5b9d9ed9f1a21c1552` |
| u9B entry | `2CCD:0000` | `3A16,3796,37AE` | `0000,001F,0007,449A,44BA,0000,7202` | same as patched stub |

| Consecutive RAM images in run 038 | Changed bytes | Contiguous address runs |
| --- | ---: | ---: |
| `+1574` → cold stub | 4 | 1 |
| Cold stub → handler entry | 6 | 1 |
| Handler entry → `+0314` | 23 | 2 |
| `+0314` → `+0338` | 2 | 1 |
| `+0338` → `+0317` | 1,182 | 20 |
| `+0317` → `+032A` | 4 | 2 |
| `+032A` → patched stub | 3 | 2 |
| Patched stub → u9B entry | 0 | 0 |
| **Net `+1574` → u9B entry** | **1,214** | **20** |

Run 037 independently measured the same net 1,214 / 20 result.
The stage counts cannot be summed to obtain net changed bytes because
later writes can restore or replace earlier values. The 1,182-byte
loader interval includes overlay load and patch effects in aggregate;
it is not a claim about individual writes or a C model.

## Private reproduction and check

Run `tools/oracle/prepare_run.sh` twice from the pinned ignored DOS
tree. Launch each through `tools/oracle/launch.sh` and the pinned heavy
DOSBox-X inside `xvfb-run` **with a PTY attached**, since this build's
debugger will not open without one. At the visible title/code-word
prompt, press Return; from the main menu select `l`, then slot `a`.
Use Alt+Pause to enter the debugger and F5 to resume. In run 037,
first arm `BP 2A8E:19E3`; once it fires, arm `+1446`, `+1574`,
loaded stub `08D0:0025`, and `2CCD:0000`. In run 038, gate again
on `+19E3`, then arm `+1574`, `08D0:0025`, `12C6:02E6`,
`12C6:0314`, `12C6:0338`, `12C6:0317`, `12C6:032A`, and
`2CCD:0000`. At each reached stop, record
`EV CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS` and
`MEMDUMPBIN 0000 0000 A0000`; move `MEMDUMP.BIN` to a distinct
ignored name before resuming. Stop at u9B entry before its prologue.
The following local assertion reads ignored files but emits no bytes:

```sh
PYTHONPATH=tools/overlay python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
from resident_loader_map import unpack

base = Path('resources/oracle-runs')
r = base / 'slot-a-native-first-insert-cold-037'
s = base / 'slot-a-native-first-insert-handler-038'
h = lambda b: sha256(b).hexdigest()
read = lambda root, name: (root / name).read_bytes()
a = lambda seg, off: seg * 16 + off
w = lambda b, p: int.from_bytes(b[p:p + 2], 'little')
inputs = {
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for root in (r, s):
    for name, expected in inputs.items():
        b = read(root, 'POOLRAD/' + name)
        assert (len(b), h(b)) == expected
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), h(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
run37 = {
    '19e3': '033d7f9bcb3dda8b28b3922a63d208af7715cc56630b003f51ef5dcf7fa2244b',
    '1446': 'e8d15be71b5c02eaea8cca6fbfb2d4fe7b3b8a31b4c1f6a5cb07672f7301732e',
    '1574': '0325518af845a1751f6bf40ba38991c8cc498f526cb3263e0a66c712dfdc80c7',
    'stub': '16eeb185b0e03b4293085b34ad7090661b8ae19a5b1f9a9e738b3f005a7c6383',
    'stub-patched': 'f8b8dab55b2d0c05010e7abf54d62c5f61d3cc42636bec9c0cf1f73274b327d0',
    'u9b-entry': 'f8b8dab55b2d0c05010e7abf54d62c5f61d3cc42636bec9c0cf1f73274b327d0',
}
run38 = {
    '1574': 'd162e45045a2a1b8c54c58ac69321a0bd0a250a15116ed1fe31e425d2f1df758',
    'stub': 'da26d1e2ba18ad9efc138cd8163df2394ade35a674364b45522eaff0529bf416',
    'handler-entry': '9168e596446be367a2190b0956fb6fc157fd7691331790a8708ebbaba7fd3cf5',
    'handler-preloader': 'de9fe269a5d64ed092982ca95e99ed70257506dd9dedb73be24e763ab46072cc',
    'loader-entry': '25ab3944a13b04dcfb8b43378fdab160f5f841aa2da812d9f5036aad00462b4f',
    'handler-postloader': '84b3901c8173ef048ea3051ba4345be909266ad31f9dba0518aff2d8d27ef226',
    'handler-preindirect': '290daa4a00aa7e8942092d05e87022126992cd8db607b1fc2fc427f1bf9d51c4',
    'stub-patched': '6e64a1a92f70b562bde20beadbabd40c3256d2b2b2829b5b9d9ed9f1a21c1552',
    'u9b-entry': '6e64a1a92f70b562bde20beadbabd40c3256d2b2b2829b5b9d9ed9f1a21c1552',
}
snap = {}
for root, expected in ((r, run37), (s, run38)):
    snap[root] = {}
    for name, digest in expected.items():
        b = read(root, 'ram-' + name + '.bin')
        assert len(b) == 655360 and h(b) == digest, (root, name)
        snap[root][name] = b
    rows = [line.split() for line in read(root, 'ev-numeric.tsv').decode().splitlines()]
    assert rows[0] == 'label CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS'.split()
    assert len(rows) == len(expected) + 1 and all(len(row) == 17 for row in rows)
    assert all(int(row[5], 16) == int(row[6], 16) == 0 for row in rows[1:])
assert [(int(x[1],16),int(x[2],16)) for x in read(r,'ev-numeric.tsv').decode().splitlines()[1:] for x in [x.split()]] == [
    (0x2A8E,0x19E3),(0x2A8E,0x1446),(0x2A8E,0x1574),(0x08D0,0x25),(0x08D0,0x25),(0x2CCD,0)]
assert [(int(x[1],16),int(x[2],16)) for x in read(s,'ev-numeric.tsv').decode().splitlines()[1:] for x in [x.split()]] == [
    (0x2A8E,0x1574),(0x08D0,0x25),(0x12C6,0x2E6),(0x12C6,0x314),(0x12C6,0x338),(0x12C6,0x317),(0x12C6,0x32A),(0x08D0,0x25),(0x2CCD,0)]
for i in (2, 3, 5):
    assert read(r,'ev-numeric.tsv').decode().splitlines()[i+1].split()[3:] == read(s,'ev-numeric.tsv').decode().splitlines()[{2:1,3:2,5:9}[i]].split()[3:]
game = read(r, 'POOLRAD/GAME.OVR')
before = snap[r]['1574']
u6f = game[0x1F42C:0x1F565]
u6f_loaded = before[a(0x2A8E,0x1446):a(0x2A8E,0x157F)]
assert (len(u6f),h(u6f),h(u6f_loaded),sum(x!=y for x,y in zip(u6f,u6f_loaded))) == (313,'370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245','cd7d4b58fff401796a53d4779c4c55e5a731215f5eecf16f304a25b4b48a9dad',6)
call = u6f[0x1574-0x1446:0x1579-0x1446]
runtime_call = before[a(0x2A8E,0x1574):a(0x2A8E,0x1579)]
assert (h(call),h(runtime_call),sum(x!=y for x,y in zip(call,runtime_call))) == ('abb0f3304fa196212ca007e1e16d7aa1983aabec90922c21d6ea93d50db07755','ebfa5b38184f9ef3235637ee56b73993ac80e11249023ba228d4fb790e7e8ecb',2)
assert (w(call,1),w(call,3),w(runtime_call,1),w(runtime_call,3)) == (0x25,0xAC,0x25,0x8D0)
original, *_ = unpack(read(r,'POOLRAD/START.EXE'))
stub_original = original[a(0xAC,0x25):a(0xAC,0x2A)]
stub_initial = snap[s]['stub'][a(0x8D0,0x25):a(0x8D0,0x2A)]
stub_patched = snap[s]['stub-patched'][a(0x8D0,0x25):a(0x8D0,0x2A)]
assert (h(stub_original),h(stub_initial),h(stub_patched),sum(x!=y for x,y in zip(stub_original,stub_patched))) == (
    'd67da1e87a1a5cac157296d1e710a6673b63b369f977f13bc9426973402e8103',
    'd67da1e87a1a5cac157296d1e710a6673b63b369f977f13bc9426973402e8103',
    'f52886ae72859b89ac8532f455e353655450490b518e17fc607ab12698fdea30',4)
for lo,hi,original_hash,loaded_hash,diff in [
    (0x2E6,0x338,'6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c','5c69a0e6a0a9141fae2d0381a4ebe4252981d54cbad01d335a3f5f73380fd6ef',[0x2F2,0x2F3]),
    (0x338,0x3E9,'58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991','58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991',[]),
]:
    src = original[a(0x0AA2,lo):a(0x0AA2,hi)]
    dst = snap[s]['1574'][a(0x12C6,lo):a(0x12C6,hi)]
    assert (h(src),h(dst),[lo+i for i,(x,y) in enumerate(zip(src,dst)) if x!=y]) == (original_hash,loaded_hash,diff)
assert (w(snap[s]['1574'],0xFC),w(snap[s]['1574'],0xFE)) == (0x2E6,0x12C6)
assert [w(snap[s]['handler-entry'],a(0x1D16,o)) for o in (0x3790,0x3792,0x3794,0x3796,0x3798,0x379A,0x379C)] == [0x27,0x8D0,0x7202,0x1579,0x2A8E,8,0x449A]
assert [w(snap[s]['u9b-entry'],a(0x1D16,o)) for o in (0x3790,0x3792,0x3794,0x3796,0x3798,0x379A,0x379C)] == [0x25,0x8D0,0x7202,0x1579,0x2A8E,8,0x449A]
u9b = game[0x2AA87:0x2AC36]
assert (len(u9b),h(u9b),h(snap[s]['u9b-entry'][a(0x2CCD,0):a(0x2CCD,0x1AF)])) == (431,'260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71','260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71')
def delta(x,y):
    changed = [i for i,(v,z) in enumerate(zip(x,y)) if v!=z]
    return len(changed), (1+sum(changed[i]>changed[i-1]+1 for i in range(1,len(changed))) if changed else 0)
assert delta(snap[r]['1574'],snap[r]['u9b-entry']) == (1214,20)
assert [delta(snap[s][x],snap[s][y]) for x,y in zip(run38,list(run38)[1:])] == [(4,1),(6,1),(23,2),(2,1),(1182,20),(4,2),(3,2),(0,0)]
assert delta(snap[s]['1574'],snap[s]['u9b-entry']) == (1214,20)
print('slot-A first-insertion cold CPU private assertions passed')
PY
```
