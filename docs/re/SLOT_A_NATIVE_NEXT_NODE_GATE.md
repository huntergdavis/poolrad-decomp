# Saved slot-A next-node traversal and null gate

In a native DOS v1.3 saved slot-A load, the unit `0x6F` loop advanced from
`2A8E:1B65` through six records. Each resident selector call returned
`AX=0000` at `+1A83`, so each `JNE +1A8A` fell through to `+1A87` and
reached the link step at `+1B65`. The sixth record's link was
`0000:0000`; the caller local became null and execution reached
`+1A10`, the loop-exit jump to `+1B7E`. These are direct native
breakpoint and RAM observations on the saved load path. This note does
not establish a one-Up route, C parity, the meaning of selector result
`103`, or a new translated-routine count.

Before tracing, `deja "Poolrad saved slot A u6F 1B65 1A06 next linked
record 1A7E selector second node native branch"` found no indexed
session. The merged [selector branch watch](SLOT_A_NATIVE_POSTLOOP_SELECTOR_BRANCH.md)
provided the preceding `+1A87 → +1B65` stop and the candidate
`1026:02EB` selector call. The [resident selector recovery](RESIDENT_DAX_SELECTOR_02EB.md)
provided its bounded original-body map. This run independently checked
the next-node code, six live caller frames and returns, all six stores,
and the null exit.

## Inputs and instruction boundaries

All DOS media and 655,360-byte RAM dumps remain ignored under
`resources/oracle-runs/slot-a-next-node-gate-per-record-live-031` and
`resources/oracle-runs/slot-a-next-node-selector-writers-live-033`.
Both are fresh copies of the same pinned source tree. The heavy DOSBox-X
binary is 47,259,176 bytes, SHA-256
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A `SAVGAMA.DAT` | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |

Unit `0x6F` begins at `GAME.OVR` file offset `0x1DFE6` and is loaded
at `2A8E:0000`. The table uses inclusive unit-relative bounds. The
original and loaded bytes have the same hash for these spans; zero
bytes differ. Instruction counts come from an i8086 decode of each
exact bounded span, checked against the live debugger disassembly.

| Overlay span | Bytes / instructions | Original and loaded SHA-256 |
| --- | ---: | --- |
| Pointer gate `+1A06..+1A12` | 13 / 4 | `6d84e4f340468c0f8adab404902c6788bb37a1aa5851aa1cd3e0df01df17b3c1` |
| Selector AL branch `+1A83..+1A89` | 7 / 3 | `e5b12a33ad0b1396c90bf64e7d45c8a18e200229edc2ba7473b4c19b1568afef` |
| Far-link step `+1B65..+1B7D` | 25 / 6 | `229e4b6392de8935cf2bbfa8b42748f2ff40ac16dcc4b8a318f741227229a36d` |

At `+1B65`, `LES DI,[BP-01CD]` loads the current far pointer. The
next two instructions read words at record `+0x104` and `+0x106`;
`+1B73` and `+1B77` copy them to caller local `SS:[3BFD..3C00]`, then
`+1B7B` jumps to `+1A06`. That gate ORs the local offset and segment,
branches to `+1A13` for a nonzero pointer, and jumps from `+1A10` to
`+1B7E` for zero. The previously verified call chain at
`+1A13..+1A82` is traversed once per record. No effect is assigned
here to its intervening runtime calls.

The resident writer `1026:0334..0336` is 3 bytes / 1 instruction,
original `START.EXE` file `0x0838B..0838D`, with original and loaded
SHA-256
`390361f8eb46a3bd47a38a0a5e6fce7ee26c17602fffb09d166b8d69cd58d567`.
The complete selector body and relocated call hashes remain in the
[preceding watch](SLOT_A_NATIVE_POSTLOOP_SELECTOR_BRANCH.md).

## Six directly observed records

For each row in the first private replay, `+1A83` stopped with
`AX=0000`, then `+1A87` and `+1B65` both stopped. At the return and
link-entry stops, `SS=1D16`, `BP=3DCA`, and local
`SS:[3BFD..3C00]` named the row's current record. The link column
was read from that record's RAM words `+0x104/+0x106` at the link-entry
stop. The return and link-entry full-RAM images are identical for each
row; the next row's local pointer directly confirms the prior link was
copied. The six records are traversal instances, not six recovered
routine identities.

| Instance | Local current pointer | Link words form next pointer | `+1A83` and `+1B65` RAM SHA-256 |
| ---: | --- | --- | --- |
| 0 | `449A:0008` | `44BB:0008` | `742c811bbb09c23774b971ea8aa1e4ea866596892b61473934faabffb34c8bfa` |
| 1 | `44BB:0008` | `44D6:0008` | `7fae4359fe06406145a58d4c6d6435662801808058a9e421ed213219bcbade2a` |
| 2 | `44D6:0008` | `44F5:0008` | `c3d2bcab5b5e6f9efb6b5d60109faa8cbb94c2fa7eb7a61943799a63000ec7c8` |
| 3 | `44F5:0008` | `4513:0008` | `e3706bfb58423bff5f51e30177ca39b71cebc569b2570db9479a4d8c71f38853` |
| 4 | `4513:0008` | `4529:0008` | `b206dd7a26d6c4de993b34a4dbdd6625fcc1dd273caf305fb3c850d9503abe85` |
| 5 | `4529:0008` | `0000:0000` | `b63c9d5b330914282c8f5c94966d116891ab9012d5e65e690f66150ceb61401d` |

After instance 5, the debugger stopped at `2A8E:1A10` with
`AX=0000`, `SS:[3BFD..3C00]=0000:0000`, and `DS:[49CE]=103`.
That RAM image has SHA-256
`2bd8f86c30c7933b3ecf53fcf34b779a2c4a5e56592e7f654a3dea5376863ea6`.
The preceding branch test at `+1A0E` was not separately stopped in
this replay; the `+1A10` stop and decoded `JNE +1A13` establish the
zero-path outcome. This run stops before executing `+1A10`, so it
assigns no effects beyond the null gate.

## Selector stores on this traversal

In a second clean replay, the debugger first stopped at overlay
`+1A7E`; only then were resident writer breakpoints armed at
`1026:0334` before and `1026:0337` after the store. Each pair had
`DS=14A0`, `SS=1D16`, `BP=37AE`, and `AX=0067` (103). Its caller
local matched the record pointer below. Instance 0 changed exactly one
RAM byte at physical `0x193CE`, the low byte of `DS:[49CE]`, from
`0` to `103`; instances 1 through 5 wrote `103` over `103` and
their full pre/post RAM images are identical. After the sixth pair,
the next armed stop was the overlay null exit `+1A10`. These are
direct writer observations; the startup writer seen in a discarded
ungated attempt is not included.

| Instance | Caller local | Pre-store RAM SHA-256 | Post-store RAM SHA-256 |
| ---: | --- | --- | --- |
| 0 | `449A:0008` | `f796af6ba9cba53500fba2657cbdb9c810c601ebbd4ff7f141e52b964cdd0467` | `e1411d70ed6f9e72a8738d55b78e6c7d78fd2143e98a109bd005a5bd006c9d79` |
| 1 | `44BB:0008` | `1bd4283c7aae029a4a2d2d44bbe720a7fe146dc36f3885f96547451cefed8791` | same |
| 2 | `44D6:0008` | `192abbf1814d1e696db12fd093a8b42406209e23521734422dce706ba2c29412` | same |
| 3 | `44F5:0008` | `5e8f323f5f3df1d06312e8521b0312eea7c2caeb2eabbd7da065d2ee06314dd3` | same |
| 4 | `4513:0008` | `a375b0abfc8854c0db60430e552260200f7eaa6eccb35f536ca2f78d2e34c40f` | same |
| 5 | `4529:0008` | `7fa369c97dbe3fed2f555b3f204dac7983d0290bef3472c2efea1d7b3643fbc5` | same |

## Private reproduction and assertions

Prepare two new ignored runs from the pinned save with
`tools/oracle/prepare_run.sh`. With the hash-pinned heavy DOSBox-X on
`PATH`, launch each through `xvfb-run -a -s '-screen 0 1280x1024x24'
tools/oracle/launch.sh <run-directory> -time-limit 1200`. At the
startup/load prompt, enter the debugger with Alt+Pause. For the first
run arm `BP 2A8E:1A83`, `BP 2A8E:1A87`, `BP 2A8E:1A8A`,
`BP 2A8E:1B65`, and `BP 2A8E:1A10`; resume and select slot A via
Enter, `l`, comma, `a`. At every return and link stop, use
`EV CS IP AX DS SS BP` and `MEMDUMPBIN 0000 0000 A0000`, moving
`MEMDUMP.BIN` to the indicated ignored filename before resuming.
For the second run, initially arm only `BP 2A8E:1A7E`; after that
stop, replace it with `BP 1026:0334`, `BP 1026:0337`, and
`BP 2A8E:1A10`. Dump before and after each of six stores.

The following private assertions check the pinned inputs, bounded
code spans, every link and writer pair, and the null local without
printing game content:

```sh
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256

base = Path('resources/oracle-runs')
r = base / 'slot-a-next-node-gate-per-record-live-031'
w = base / 'slot-a-next-node-selector-writers-live-033'
read = lambda p: p.read_bytes()
digest = lambda b: sha256(b).hexdigest()
word = lambda b, p: int.from_bytes(b[p:p + 2], 'little')
addr = lambda seg, off: seg * 16 + off
inputs = {
    'START.EXE': (65231, 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'),
    'GAME.OVR': (206105, '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'),
    'SAVGAMA.DAT': (13137, 'abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1'),
}
for root in (r, w):
    for name, expected in inputs.items():
        b = read(root / 'POOLRAD' / name)
        assert (len(b), digest(b)) == expected
heavy = read(Path('resources/dosbox-x-heavy-src/src/dosbox-x'))
assert (len(heavy), digest(heavy)) == (47259176, '8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92')
game = read(r / 'POOLRAD/GAME.OVR')
loaded = read(r / 'ram-n0-return.bin')
spans = (
    (0x1A06, 0x1A13, '6d84e4f340468c0f8adab404902c6788bb37a1aa5851aa1cd3e0df01df17b3c1'),
    (0x1A83, 0x1A8A, 'e5b12a33ad0b1396c90bf64e7d45c8a18e200229edc2ba7473b4c19b1568afef'),
    (0x1B65, 0x1B7E, '229e4b6392de8935cf2bbfa8b42748f2ff40ac16dcc4b8a318f741227229a36d'),
)
for lo, hi, expected in spans:
    original = game[0x1DFE6 + lo:0x1DFE6 + hi]
    runtime = loaded[addr(0x2A8E, lo):addr(0x2A8E, hi)]
    assert digest(original) == digest(runtime) == expected
exe = read(r / 'POOLRAD/START.EXE')
writer = exe[0x0838B:0x0838E]
assert digest(writer) == digest(loaded[addr(0x1026, 0x0334):addr(0x1026, 0x0337)]) == '390361f8eb46a3bd47a38a0a5e6fce7ee26c17602fffb09d166b8d69cd58d567'
links = (0x449A, 0x44BB, 0x44D6, 0x44F5, 0x4513, 0x4529, 0)
return_hashes = (
    '742c811bbb09c23774b971ea8aa1e4ea866596892b61473934faabffb34c8bfa',
    '7fae4359fe06406145a58d4c6d6435662801808058a9e421ed213219bcbade2a',
    'c3d2bcab5b5e6f9efb6b5d60109faa8cbb94c2fa7eb7a61943799a63000ec7c8',
    'e3706bfb58423bff5f51e30177ca39b71cebc569b2570db9479a4d8c71f38853',
    'b206dd7a26d6c4de993b34a4dbdd6625fcc1dd273caf305fb3c850d9503abe85',
    'b63c9d5b330914282c8f5c94966d116891ab9012d5e65e690f66150ceb61401d',
)
pre_hashes = (
    'f796af6ba9cba53500fba2657cbdb9c810c601ebbd4ff7f141e52b964cdd0467',
    '1bd4283c7aae029a4a2d2d44bbe720a7fe146dc36f3885f96547451cefed8791',
    '192abbf1814d1e696db12fd093a8b42406209e23521734422dce706ba2c29412',
    '5e8f323f5f3df1d06312e8521b0312eea7c2caeb2eabbd7da065d2ee06314dd3',
    'a375b0abfc8854c0db60430e552260200f7eaa6eccb35f536ca2f78d2e34c40f',
    '7fa369c97dbe3fed2f555b3f204dac7983d0290bef3472c2efea1d7b3643fbc5',
)
for i, seg in enumerate(links[:-1]):
    ret = read(r / f'ram-n{i}-return.bin')
    link = read(r / f'ram-n{i}-link.bin')
    pre = read(w / f'ram-w{i}-pre.bin')
    post = read(w / f'ram-w{i}-post.bin')
    assert all(len(b) == 655360 for b in (ret, link, pre, post))
    assert digest(ret) == digest(link) == return_hashes[i]
    assert digest(pre) == pre_hashes[i]
    for b in (ret, link, pre, post):
        assert (word(b, addr(0x1D16, 0x3BFF)), word(b, addr(0x1D16, 0x3BFD))) == (seg, 8)
        assert (word(b, addr(seg, 8 + 0x106)), word(b, addr(seg, 8 + 0x104))) == (links[i + 1], 8 if links[i + 1] else 0)
    assert word(pre, addr(0x14A0, 0x49CE)) == (0 if i == 0 else 103)
    assert word(post, addr(0x14A0, 0x49CE)) == 103
    changed = [p for p, (a, b) in enumerate(zip(pre, post)) if a != b]
    assert changed == ([0x193CE] if i == 0 else [])
    assert digest(post) == ('e1411d70ed6f9e72a8738d55b78e6c7d78fd2143e98a109bd005a5bd006c9d79' if i == 0 else pre_hashes[i])
end = read(r / 'ram-null-exit.bin')
assert (len(end), digest(end)) == (655360, '2bd8f86c30c7933b3ecf53fcf34b779a2c4a5e56592e7f654a3dea5376863ea6')
assert (word(end, addr(0x1D16, 0x3BFF)), word(end, addr(0x1D16, 0x3BFD))) == (0, 0)
assert word(end, addr(0x14A0, 0x49CE)) == 103
print('slot-A next-node private assertions passed')
PY
```
