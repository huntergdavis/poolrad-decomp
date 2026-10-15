# Saved slot-A post-Up view-table gate at unit `0x3BD:+02A3`

**Classification:** bounded twelve-instruction caller gate from `+02A3`
through the `+02C2` branch. The reached item is 19, so native execution
falls through to `+02C4`. The C model covers the stack inputs, 16-bit
unsigned multiply, far-pointer load, effective offset, item read, and both
zero/nonzero branch outcomes. It ends before the tile-call argument setup;
this is not a complete view or full-C parity claim.

Before investigation, `deja "Poolrad slot A post Up U3BD 02A3 view table
bounds third outer arm next 02BC branch native parity"` was run with a
25-second bound and returned no reusable result. The checked-in
[preceding bounds gate](SLOT_A_POST_UP_028B_BOUNDS_GATE.md) supplied the
third-iteration entry and caller-frame lead. The
[first-wall tile model](SLOT_A_U3BD_FIRST_WALL_TILE.md) supplied a generic
piece-buffer indexing lead; the saved post-Up operands and route below were
independently measured in a fresh one-Up run.

## Original bytes and bounded semantics

Ignored DOS v1.3 `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
`START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
the ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Unit `0x3BD:+02A3..+02C4` is original `GAME.OVR [0x2EB9A,0x2EBBB)`,
**33 bytes / twelve aligned 16-bit instructions**, SHA-256
`2e2887d3abde58d70bfa5f5ff5d4d65a9fae4960763000a3fe3ca6daa8a7f08d`.

| Offset | Operation | Bounded effect |
| --- | --- | --- |
| `+02A3..+02A8` | `MOV AL,SS:[BP-9]`; `XOR AH,AH`; `MOV CX,AX` | Zero-extend column byte into CX. |
| `+02AA..+02B2` | `MOV AL,SS:[BP+8]`; `XOR AH,AH`; `MOV DX,009C`; `MUL DX` | Zero-extend slice byte and compute unsigned `DX:AX = slice × 156`. |
| `+02B4..+02BA` | `LES DI,DS:[6A58]`; `ADD DI,AX`; `ADD DI,CX` | Load the live piece-buffer far pointer and advance its 16-bit offset. |
| `+02BC..+02C2` | `CMP byte ES:[DI-009C],0`; `JBE +0300` | Zero item branches to `+0300`; positive byte falls through to `+02C4`. |

The C model follows instruction order and returns `BOUNDARY` at the first
reached unavailable RAM operand, preserving earlier register effects. It
tracks which arithmetic flags are defined after `XOR` and `MUL`; the final
byte `CMP` defines all arithmetic flags. It uses a 20-bit real-mode physical
address and 16-bit effective-offset arithmetic. The span writes no guest RAM.

## Fresh native one-Up operand and branch proof

A fresh private DOSBox-X heavy-debug run loaded visible `(15,4,N; 10:50)`
from the pinned save. Only `BP 2D14:026E` was initially armed at that
playable frame, then exactly one gameplay Up was sent. Its three hits in
the `BP=3EBA`, `SS=1D16` caller frame had outer words 3, 4, and 5.
The third continuation reached `2D14:02A3` with
`SS:[BP-9]=5`, `SS:[BP+8]=14`, and `DS:[6A58]` equal to far
`3D0E:0008`. The pre-gate 640 KiB RAM SHA-256 was
`8a920fe3b8692cb0334e6eadba770dcee7d37136612f3de138c05ddb47ce87b2`.

The original arithmetic gives `14 × 156 = 2184 = 0x0888` and
`DI = 0x0008 + 0x0888 + 5 = 0x0895`. Native stopped at `2D14:02BC`
with `DI=0x0895`; the debugger displayed `ES:[0x07F9]=0x13` and
`JBE +0300 (no jmp)`. The byte is **19** at real-mode physical
`0x3D8D9`. Native then stopped directly at `2D14:02C4` with arithmetic
flag bits CF=0, ZF=0, SF=0, OF=0, AF=0, PF=0. RAM SHA-256 at both
stops was again
`8a920fe3b8692cb0334e6eadba770dcee7d37136612f3de138c05ddb47ce87b2`:
**zero changed byte values** across all three same-run images.

The private C test initializes the full 640 KiB RAM image at the same
`+02A3` stop. It computes column 5, slice 14, item 19, item offset
`0x07F9`, AX=`0x0888`, CX=5, DX=0, ES=`0x3D0E`, DI=`0x0895`,
and next IP=`+02C4`, then compares **655,360/655,360 RAM bytes** with
the native +02C4 image. With input FLAGS=`0x7297`, the model computes
FLAGS=`0x7202`; the full native FLAGS word was not dumped. Synthetic
tests cover item zero, first and later missing operands, and read-only RAM.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
span = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()[0x2EB9A:0x2EBBB]
ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, 0x02A3))
assert len(ins) == 12 and ins[-1].address + ins[-1].size == 0x02C4
print(len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-02a3-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-02a3-p9 -j 8
ctest --test-dir resources/build-post-up-02a3-p9 --output-on-failure
resources/build-post-up-02a3-p9/poolrad_overlay_u3bd_post_up_02a3_view_table_tests \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/02a3-third-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/02a3-third-02c4-ram-private.bin
```

Original media, save, debugger trace, screenshots, and RAM captures remain
ignored under `resources/`.
