# Saved slot-A post-insertion call and loop gate

In one native DOS replay of the hash-pinned saved slot A, the heavy debugger
stopped at unit `0x6F:+0x19E6`, then `+0x19E7`, then `+0x19EA`. The second
stop is immediately before the near call to `+0x02AE`; the third is its
first post-return instruction. The retained local word at `SS:0x37B0` was
`5` at the third stop, and the host run had no new directories or modified
input files. This measures this continuation only; it supplies no one-Up
parity result or strict C routine count.

Before this replay, `deja "Poolrad slot A u6f 19E7 19EA post insertion
directory IOResult native replay"` found no matching indexed session.
The merged [insertion-return watch](SLOT_A_NATIVE_19E3_INSERTION_RETURN.md)
supplied the previous breakpoint and live pointers. The merged
[post-insertion callee map](U6F_POSTINSERT_02AE.md) supplied the bounded
source and runtime address lead. Original and loaded code were checked
again against the private images and RAM here.

## Hash-locked inputs and code

| Private input or span | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 bytes | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 bytes | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Saved slot-A `SAVGAMA.DAT` | 13,137 bytes | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original caller `+0x19E2..+0x19E9`, file `0x1F9C8..0x1F9CF` | 8 bytes | `bc2dc35bdcfe1ed3f9556b33dfa58e65967b7df3b59180bf5ab21a52c1f4dba9` |
| Loaded caller `2A8E:19E2..19E9`, at `+0x19E7` | 8 bytes | `bc2dc35bdcfe1ed3f9556b33dfa58e65967b7df3b59180bf5ab21a52c1f4dba9` |
| Original `+0x02AE..+0x032E`, file `0x1E294..0x1E314` | 129 bytes | `ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539` |
| Loaded `2A8E:02AE..032E`, at `+0x19E7` | 129 bytes | `70114e09285ff85b5c6176903f5d5640e427bed0ea52b41786aa19793c2629d7` |
| Private heavy DOSBox-X executable | 47,259,176 bytes | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The loaded and original caller spans are identical: `PUSH CS; CALL +0x1446;
PUSH CS; CALL +0x02AE`. In the callee, 16 bytes differ between original and
loaded images at eight far-call operands, ending at offsets `+0x031D..031E`.
The original 129-byte body decodes to 51 x86-16 instructions. Its checked
instructions at `+0x02D5` call SYSTEM `0xAF8:0x04A2`; `+0x02DA` stores AX
at `[BP-2]`; `+0x02DD..+0x02E7` tests that word against `0` and `5`;
`+0x031F..+0x032E` rechecks it and returns for either value. The separately
checked `+0x02D0` SYSTEM call is the directory operation described in the
callee map. The observed return and stored word are consistent with its
IOResult-5 exit. This interpretation relies on the source instruction map;
the breakpoint sequence did not stop inside SYSTEM.

## Same-run observations

Breakpoints `BP 2A8E:19E6`, `BP 2A8E:19E7`, and `BP 2A8E:19EA` were armed
at the visible load-slot prompt before choosing `a`. They fired in that
order, all before their named instructions. Register values are hexadecimal.

| Stop | `SS:SP` | `AX`; `BX`; `CX`; `DX` | `ES:DI`; flags |
| --- | --- | --- | --- |
| `2A8E:19E6` | `1D16:37B8` | `0007`; not sampled; not sampled; `0011` | `449A:0008`; `7246` |
| `2A8E:19E7` | `1D16:37B6` | `0007`; `0004`; `0005`; `0011` | `449A:0008`; `7246` |
| `2A8E:19EA` | `1D16:37B8` | `0005`; `35A0`; `0000`; `3526` | `1D16:3531`; `7246` |

`DS=14A0` and `BP=3DCA` at all three stops. The `+0x19E7` stack top
`SS:37B6=2A8E` is the caller's pushed CS. The `+0x19EA` RAM retains the
callee local `SS:37B0=0005`, where the checked `+0x02DA` instruction stores
AX. `AX=0005` is independently observed at the caller return.

The private full-RAM snapshots are each 655,360 bytes:

| Stop | SHA-256 |
| --- | --- |
| `+0x19E6` | `20dfc70cd976f6bb8d357f223954aed53fd1a47484880b8ed179f0bf236e52da` |
| `+0x19E7` | `53b2ac64d44324c5c7ff8f9325caecb88ea8d69eac47eae2d33d5f1e1ca24c36` |
| `+0x19EA` | `3ed5cea594eea502fcf1bbd33bdc51c01c250201283b1d7f7eb21a47e854976d` |

| Numeric field | At `+0x19E7` | At `+0x19EA` |
| --- | ---: | ---: |
| `DS:[49CE]` word | `0` | `0` |
| `DS:[537A]` Pascal length byte | `11` | `11` |
| `DS:[5D92]` active-character far pointer | `449A:0008` | `449A:0008` |
| `DS:[5D96]` party-head far pointer | `449A:0008` | `449A:0008` |
| `DS:[49D6]` window-1 far pointer | `3A16:0000` | `3A16:0000` |
| Window-1 word `+0x067C` | `1` | `1` |
| Allocated node `449A:0008 +0x00BF` byte | `0` | `0` |
| Retained callee local `SS:37B0` word | pre-call stack value | `5` |

The copied input tree had zero subdirectories; the run tree still had zero
at the `+0x19EA` stop. All 153 copied input files passed the prepared
SHA-256 manifest after the stop. These are host filesystem observations,
not proof that the directory operation was skipped. No other record bytes,
intermediate writes, later loop iterations, or one-Up state were measured.

## Private reproduction and numeric assertions

Prepare and run in an isolated checkout with ignored DOS inputs:

```sh
tools/oracle/prepare_run.sh slot-a-19e7-loop-gate-live-023
export PATH="$PWD/resources/dosbox-x-heavy-src/src:$PATH"
xvfb-run -a -s '-screen 0 1280x1024x24' \
  tools/oracle/launch.sh resources/oracle-runs/slot-a-19e7-loop-gate-live-023 \
  -time-limit 1200
```

The heavy debugger requires an interactive terminal. At the visible
`LOAD WHICH GAME?` prompt, press Alt+Pause, arm the three breakpoints above,
enter `RUN`, and choose `a`. At each stop use
`EV CS IP DS ES DI SS SP BP AX BX CX DX FLAGS`. At all three stops run
`MEMDUMPBIN 0000 0000 A0000`, moving each private `MEMDUMP.BIN` before
resuming; the names below are `return-19e6-ram.bin`,
`precall-19e7-ram.bin`, and `gate-19ea-ram.bin`.

The following checks input and snapshot hashes, source/loaded code spans,
and the numeric fields without emitting original bytes:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

r = Path('resources/oracle-runs/slot-a-19e7-loop-gate-live-023')
load = lambda name: (r / name).read_bytes()
digest = lambda data: sha256(data).hexdigest()
expected = {
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for name, pair in expected.items():
    data = load('POOLRAD/' + name)
    assert (len(data), digest(data)) == pair
heavy = Path('resources/dosbox-x-heavy-src/src/dosbox-x').read_bytes()
assert (len(heavy), digest(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
game = load('POOLRAD/GAME.OVR')
names = ('return-19e6-ram.bin', 'precall-19e7-ram.bin', 'gate-19ea-ram.bin')
hashes = (
    '20dfc70cd976f6bb8d357f223954aed53fd1a47484880b8ed179f0bf236e52da',
    '53b2ac64d44324c5c7ff8f9325caecb88ea8d69eac47eae2d33d5f1e1ca24c36',
    '3ed5cea594eea502fcf1bbd33bdc51c01c250201283b1d7f7eb21a47e854976d',
)
ram = [load(name) for name in names]
assert all((len(data), digest(data)) == (655360, h) for data, h in zip(ram, hashes))
original_caller = game[0x1F9C8:0x1F9D0]
original_callee = game[0x1E294:0x1E315]
loaded_caller = ram[1][(0x2A8E << 4) + 0x19E2:(0x2A8E << 4) + 0x19EA]
loaded_callee = ram[1][(0x2A8E << 4) + 0x02AE:(0x2A8E << 4) + 0x032F]
assert digest(original_caller) == digest(loaded_caller) == 'bc2dc35bdcfe1ed3f9556b33dfa58e65967b7df3b59180bf5ab21a52c1f4dba9'
assert digest(original_callee) == 'ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539'
assert digest(loaded_callee) == '70114e09285ff85b5c6176903f5d5640e427bed0ea52b41786aa19793c2629d7'
assert [0x02AE + i for i, (x, y) in enumerate(zip(original_callee, loaded_callee)) if x != y] == [0x02CE, 0x02CF, 0x02D3, 0x02D4, 0x02D8, 0x02D9, 0x02FA, 0x02FB, 0x0309, 0x030A, 0x030E, 0x030F, 0x0318, 0x0319, 0x031D, 0x031E]
word = lambda data, seg, off: int.from_bytes(data[(seg << 4) + off:(seg << 4) + off + 2], 'little')
far = lambda data, seg, off: (word(data, seg, off + 2), word(data, seg, off))
pre, post = ram[1:]
assert word(pre, 0x1D16, 0x37B6) == 0x2A8E
assert word(post, 0x1D16, 0x37B0) == 5
assert [word(data, 0x14A0, 0x49CE) for data in (pre, post)] == [0, 0]
assert [data[(0x14A0 << 4) + 0x537A] for data in (pre, post)] == [11, 11]
for off in (0x5D92, 0x5D96):
    assert [far(data, 0x14A0, off) for data in (pre, post)] == [(0x449A, 8)] * 2
assert [far(data, 0x14A0, 0x49D6) for data in (pre, post)] == [(0x3A16, 0)] * 2
assert [word(data, 0x3A16, 0x067C) for data in (pre, post)] == [1, 1]
assert [data[(0x449A << 4) + 8 + 0xBF] for data in (pre, post)] == [0, 0]
assert sum(p.is_dir() for p in (r / 'POOLRAD').rglob('*')) == 0
print('hash gates and numeric post-insertion assertions passed')
PY
(cd resources/oracle-runs/slot-a-19e7-loop-gate-live-023/POOLRAD && \
  sha256sum --status -c ../.oracle-input-sha256)
```

Original images, save, emulator captures, RAM dumps, and extracted code stay
under ignored `resources/`; only addresses, hashes, counts and numeric
observations enter this note.
