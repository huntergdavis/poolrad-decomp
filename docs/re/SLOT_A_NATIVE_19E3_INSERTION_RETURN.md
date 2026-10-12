# Native slot-A first party-insertion return

One native DOS v1.3 run of the hash-pinned saved slot A stopped at loaded unit
`0x6F:+0x19DA`, `+0x19E3`, and `+0x19E6`, in that order. The record loader
returned before the caller reached its near call at `+0x19E3` to unit
`+0x1446`; that call returned to `+0x19E6`. The same-run RAM snapshots show
the party-head and active-character far pointers becoming `449A:0008`, and
the window-1 word at `+0x067C` changing from 0 to 1. The allocated node's
`+0xBF` byte was 0 at both snapshots. These are values at two breakpoints;
they do not reveal intermediate writes or establish later party, directory,
or one-Up behavior. No strict-C routine count is claimed.

Before debugging, `deja "Poolrad slot A u6f 19E3 insertion 19E6 party head
5D96 tail 5D92 window1 67C native"` found no indexed match. The merged
[caller continuation map](U6F_PARTY_CONTINUATIONS.md) supplied the call
lead, [prior native record return](SLOT_A_NATIVE_19BA_RECORD_RETURN.md)
supplied the preceding gate, and the
[native window-word watch](SLOT_A_NATIVE_PARTY_COUNTER_67C.md) supplied the
`DS:[49D6]+067C` address lead. The original code and live far pointers were
checked again here. Existing source maps identify `DS:5D96/5D98` as the
party-list **head** and `DS:5D92/5D94` as the **active character**; `5D92`
is not described as a separate tail pointer in this note.

## Hash-locked inputs and bounded code

| Private input or span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Saved slot-A `SAVGAMA.DAT` | 13,137 bytes | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original caller `+0x19E2..+0x19E9`, GAME file `0x1F9C8..0x1F9CF` | 8 bytes, 4 instructions | `bc2dc35bdcfe1ed3f9556b33dfa58e65967b7df3b59180bf5ab21a52c1f4dba9` |
| Loaded caller `2A8E:19E2..19E9`, sampled before insertion | 8 bytes, 4 instructions | `bc2dc35bdcfe1ed3f9556b33dfa58e65967b7df3b59180bf5ab21a52c1f4dba9` |
| Insertion callee `+0x1446..+0x157E`, GAME file `0x1F42C..0x1F564` | 313 bytes | `370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245` |
| Private heavy DOSBox-X executable | 47,259,176 bytes | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The original and loaded eight-byte caller spans are identical. They decode
as `PUSH CS; CALL +0x1446; PUSH CS; CALL +0x02AE`. Thus `+0x19E3` is before
the insertion call and `+0x19E6` is its first post-return instruction,
before the distinct `+0x19E7` post-insertion call. The latter call and the
common loop gate at `+0x19EA` were not watched in this run.

## Same-run numeric observations

At the visible load-slot prompt, before choosing `a`, the heavy debugger
armed `BP 2A8E:19DA`, `BP 2A8E:19E3`, and `BP 2A8E:19E6`. All three fired
in that order. Register values are hexadecimal; `CS=2A8E`, `DS=14A0`,
`SS=1D16`, and `BP=3DCA` at all three stops.

| Stop before instruction | `SS:SP` | `AX`; `DX`; `ES:DI`; `FLAGS` | Observation |
| --- | --- | --- | --- |
| `+0x19DA` | `1D16:37B8` | `3E01`; `0000`; not sampled; `7206` | Preceding record loader returned in this run. |
| `+0x19E3` | `1D16:37B2` | `3E01`; `0000`; `1D16:3626`; `7206` | Insertion call ready; top stack words `2A8E,0008,449A`. |
| `+0x19E6` | `1D16:37B8` | `0007`; `0011`; `449A:0008`; `7246` | Insertion callee returned; caller SP restored. |

The two full private RAM dumps, both 655,360 bytes, have SHA-256
`6ceed694294d93c2cdbd2684626f2245b04af0665b0b5bfeeda284bda5d8948a`
at `+0x19E3` and
`5d18bcca937401d24b77480351f0a3107305a1e97662aad55145385ea85095a0`
at `+0x19E6`. Reading only the requested numeric fields yields:

| Field | Before call `+0x19E3` | After return `+0x19E6` |
| --- | --- | --- |
| `DS:[5D92]` active-character far pointer | `0000:0000` | `449A:0008` |
| `DS:[5D96]` party-head far pointer | `0000:0000` | `449A:0008` |
| `DS:[49D6]` window-1 far pointer | `3A16:0000` | `3A16:0000` |
| Window-1 word at that pointer `+067C` | `0` | `1` |
| Allocated node `449A:0008 +00BF` byte | `0` | `0` |
| Caller local `SS:[BP-1CD/-1CB]` far pointer | `449A:0008` | `449A:0008` |

This directly measures a returned call and the listed state differences.
It does not classify `AX=0007` as a success code, infer a tail pointer, or
claim the node's `+0xBF` byte was never written between stops. The replay
was stopped before `+0x19E7`; no directory effect or Up parity is measured.

## Private reproduction and numeric assertions

Use an isolated checkout with ignored pinned DOS inputs and the private
heavy debugger:

```sh
tools/oracle/prepare_run.sh slot-a-19e3-insertion-live-022
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s "-screen 0 1280x1024x24" \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-19e3-insertion-live-022 \
  -time-limit 1200
```

Wait for the visible code-word prompt, enter Return, select `l` at the main
menu, then pause at the visible load-slot prompt with Alt+Pause. Arm the
three breakpoints above, enter `RUN`, and choose `a`. At each stop record
`EV CS IP DS ES DI SS SP BP AX DX FLAGS`. At `+0x19E3` and `+0x19E6`,
run `MEMDUMPBIN 0000 0000 A0000` and move each private `MEMDUMP.BIN`
before resuming. The ignored dump names below are
`insertion-call-19e3-ram.bin` and `insertion-return-19e6-ram.bin`.

With those private files, the following asserts hashes and numeric fields
without emitting original bytes:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

run = Path('resources/oracle-runs/slot-a-19e3-insertion-live-022')
load = lambda name: (run / name).read_bytes()
digest = lambda data: sha256(data).hexdigest()
game, start, save = (load(name) for name in ('POOLRAD/GAME.OVR', 'POOLRAD/START.EXE', 'POOLRAD/SAVGAMA.DAT'))
assert (len(game), digest(game)) == (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614')
assert (len(start), digest(start)) == (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd')
assert (len(save), digest(save)) == (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1')
pre, post = load('insertion-call-19e3-ram.bin'), load('insertion-return-19e6-ram.bin')
assert (len(pre), digest(pre)) == (655360, '6ceed694294d93c2cdbd2684626f2245b04af0665b0b5bfeeda284bda5d8948a')
assert (len(post), digest(post)) == (655360, '5d18bcca937401d24b77480351f0a3107305a1e97662aad55145385ea85095a0')
original = game[0x1F9C8:0x1F9D0]
loaded = pre[(0x2A8E << 4) + 0x19E2:(0x2A8E << 4) + 0x19EA]
assert digest(original) == 'bc2dc35bdcfe1ed3f9556b33dfa58e65967b7df3b59180bf5ab21a52c1f4dba9'
assert loaded == original
assert digest(game[0x1F42C:0x1F565]) == '370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245'
word = lambda ram, seg, off: int.from_bytes(ram[(seg << 4) + off:(seg << 4) + off + 2], 'little')
far = lambda ram, seg, off: (word(ram, seg, off + 2), word(ram, seg, off))
assert [far(ram, 0x14A0, 0x5D92) for ram in (pre, post)] == [(0, 0), (0x449A, 8)]
assert [far(ram, 0x14A0, 0x5D96) for ram in (pre, post)] == [(0, 0), (0x449A, 8)]
assert [far(ram, 0x14A0, 0x49D6) for ram in (pre, post)] == [(0x3A16, 0)] * 2
assert [word(ram, 0x3A16, 0x067C) for ram in (pre, post)] == [0, 1]
assert [ram[(0x449A << 4) + 8 + 0xBF] for ram in (pre, post)] == [0, 0]
assert [far(ram, 0x1D16, 0x3BFD) for ram in (pre, post)] == [(0x449A, 8)] * 2
assert [word(pre, 0x1D16, 0x37B2 + 2 * i) for i in range(3)] == [0x2A8E, 8, 0x449A]
print('active and head:', [far(ram, 0x14A0, 0x5D92) for ram in (pre, post)], [far(ram, 0x14A0, 0x5D96) for ram in (pre, post)])
print('window1 +67C:', [word(ram, 0x3A16, 0x067C) for ram in (pre, post)])
print('node +BF:', [ram[(0x449A << 4) + 8 + 0xBF] for ram in (pre, post)])
PY
```

Original images, save, screenshots, RAM dumps and debugger stream remain
under ignored `resources/`; no game bytes or captures enter Git.
