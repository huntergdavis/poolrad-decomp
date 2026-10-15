# Saved slot-A post-Up view bounds at unit `0x3BD:+028B`

**Classification:** bounded eight-instruction, read-only caller gate from
`+028B` to `+02A3` or `+0300`. The C model covers the four signed bounds
checks and matches the complete 655,360-byte RAM image for the reached
third post-Up arm. It ends before the `+02A3` view-table calculation; this
is not a complete view or full-C parity claim.

Before debugging, `deja "Poolrad slot A one Up U3BD 028B bounds checks after
third 026E outer arm next tile call 02FB offscreen view"` was run with a
25-second bound and returned no reusable result. The checked-in
[outer-arm gate](SLOT_A_POST_UP_026E_OUTER_ARM_GATE.md),
[outer-loop decision](SLOT_A_POST_UP_030E_OUTER_LOOP_GATE.md), and
[third tile call](SLOT_A_POST_UP_02FB_THIRD_PAGE_WRITER_GATE.md) supplied
the preceding call ordinal and caller-frame interpretation. A fresh DOS
one-Up run independently confirmed this gate.

## Pinned source and exact bounds

Ignored DOS v1.3 `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
`START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Unit `0x3BD:+028B..+02A3` is original `GAME.OVR [0x2EB82,0x2EB9A)`,
**24 bytes / eight aligned 16-bit instructions**, SHA-256
`b5a720208f560365604e1de6341c8bad4ce758ba80f365b8bb01b25a84631b9a`.

| Offset | Operation | Branch if true |
| --- | --- | --- |
| `+028B`, `+028F` | `CMP word SS:[BP-8],0`; signed `JL` | `+0300` |
| `+0291`, `+0295` | `CMP word SS:[BP-8],10`; signed `JG` | `+0300` |
| `+0297`, `+029B` | `CMP word SS:[BP-6],0`; signed `JL` | `+0300` |
| `+029D`, `+02A1` | `CMP word SS:[BP-6],10`; signed `JG` | `+0300` |

All four false branches fall through to `+02A3`. The model uses 16-bit SS
offset wrap, x86 word-CMP arithmetic flags, and ordered reads. A missing
reached word returns `BOUNDARY` with earlier CMP flags preserved. It does
not read the inner word if either outer test exits. This span writes no
register other than FLAGS and no memory.

## Fresh native one-Up route and bounded parity

A fresh private DOSBox-X heavy-debug run loaded visible `(15,4,N; 10:50)`.
With only `BP 2D14:026E` initially armed, exactly one gameplay Up reached
that address three times in the `BP=3EBA`, `SS=1D16` caller frame, with
outer words 3, 4, and 5. After the third hit, the preceding arm reached
`2D14:028B` with outer `SS:[BP-8]=6`, inner `SS:[BP-6]=4`, AX=4. The
pre-gate 640 KiB RAM SHA-256 was
`35e1eb89af4a189d13ac8592724b4504b713d15b3ebb2712b56303e1507e7775`.

After arming `BP 2D14:02A3` and resuming, native execution stopped at
`+02A3`. The debugger showed CF=1, ZF=0, SF=1, OF=0, AF=1, PF=1 after
the final `4-10` comparison. The post-gate RAM had the same SHA-256:
`35e1eb89af4a189d13ac8592724b4504b713d15b3ebb2712b56303e1507e7775`.
The exact value difference count was **zero**. The private C test initializes
the SS segment from that same pre-gate RAM, checks outer 6 and inner 4,
computes next IP `+02A3`, and compares all **655,360/655,360 RAM bytes**
against the same-run native target. With input FLAGS=`0x7246`, the model
computes FLAGS=`0x7297`; the full native FLAGS word was not dumped.

The synthetic test covers both inclusive endpoints, each of four signed
out-of-range exits, first-read and late-read boundaries, and unchanged SS
bytes. No media, save, screenshot, trace, or RAM image is tracked.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
span = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()[0x2EB82:0x2EB9A]
ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, 0x028B))
assert len(ins) == 8 and ins[-1].address + ins[-1].size == 0x02A3
print(len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-028b-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-028b-p9 -j 8
ctest --test-dir resources/build-post-up-028b-p9 --output-on-failure
resources/build-post-up-028b-p9/poolrad_overlay_u3bd_post_up_028b_bounds_gate_tests \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/028b-third-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/028b-third-02a3-ram-private.bin
```
