# Saved slot-A post-Up tile-loop gate at unit `0x3BD:+0300`

**Classification:** bounded five-instruction caller gate after the second
reached `2D14:02FB..0300` tile call. The C model reproduces the same-run
`+0300` to taken `+030E` RAM transition and measured `AX`, `FLAGS`, and next
IP. This is not a complete view, tile dispatcher, later page write, display
flush, or whole-CPU translation. No full-C count follows from this gate.

Before this investigation, `deja "Poolrad slot A post-Up second 2D14:0300
return next page writer caller branch U3BD one Up offscreen frame"` was run
with a 25-second bound and returned no reusable result. The checked-in
[post-Up two-tile evidence](SLOT_A_POST_UP_02FB_TWO_TILE_GATE.md) supplied
the immediately preceding call order and save identity. The new native
run captured the post-second-return branch independently.

## Image and exact bounded body

Ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

Unit `0x3BD:+0300..+030E` is `GAME.OVR [0x2EBF7,0x2EC05)`, 14 bytes and
five aligned 16-bit instructions, SHA-256
`c7a4acefbc3e6072b229a18262fe91a006931d752b4c70bf3f8d00ac5e334e62`.
The loaded `2D14:0300..030E` span has the same 14-byte SHA-256. Its
instruction sequence is:

| Offset | Operation | Bounded effect |
| --- | --- | --- |
| `+0300` | `INC byte SS:[BP-9]` | Wrap one local byte modulo 256. |
| `+0303` | `MOV AX,SS:[BP-6]` | Read the loop word. |
| `+0306` | `CMP AX,SS:[BP-13]` | Set arithmetic flags from 16-bit subtraction. |
| `+0309` | `JE +030E` | Go to the next outer-loop comparison when equal. |
| `+030B` | `JMP +0288` | Repeat the tile loop when unequal. |

The C boundary covers those five instructions and both branch outcomes.
Every stack operand is bounds-checked before any mutation. `CMP` replaces
the arithmetic flags from `INC`; other flag bits are preserved. Offset
arithmetic wraps at 16 bits as on the guest CPU.

## Same-run native branch

A private DOSBox-X run loaded the saved slot at visible `(15,4,N; 10:50)`.
With only `BP 2D14:0300` armed, exactly one Up was sent. The first and
second `0300` stops occurred in order; the second is the direct return from
the second `02FB` call established in the preceding note. At that stop,
`SS:SP=1D16:3EAC`, `BP=3EBA`, `DS=14A0`, and `FLAGS=0x7246`.
The local byte `SS:[BP-9]` was 3; the words `SS:[BP-6]` and
`SS:[BP-13]` were both 4. The destination object and dirty-metadata
hashes matched the earlier post-second-call capture, even though other
transient guest RAM differed between replays.

The second `0300` RAM SHA-256 is
`89a6ab407d88a0f605fefb7318d5420a0ea2c978bb45c06699afa9421299d273`.
After arming `BP 2D14:030E` and resuming, the debugger stopped at `030E`
without another `0300` hit. Its RAM SHA-256 is
`02f41d4926ff71bcbcfb0f69e438162962711d42d40b85a1972488a71ecbe473`.
The two full 640 KiB snapshots differ at **one byte**, physical `0x21011`
(`SS:[BP-9]`), from 3 to 4. At `030E`, `AX=4` and `FLAGS=0x7246`.
This establishes the reached equal branch. The first `0300` stop's RAM
SHA-256 is
`cf702f93ea85be3bc65dfbd36c31aafbf24c60ade65f094024ab4d8352df368f`;
it is retained under ignored `resources/` to pin the ordinal. These are
value differences, not write counts.

The new `pr_u3bd_post_up_tile_loop_gate` model accepts the second-return
RAM segment and measured `AX=7`, `FLAGS=0x7246`. It reaches `+030E` with
`AX=4`, `FLAGS=0x7246`, and **655,360/655,360 RAM bytes** matching the
same-run native target. A synthetic CTest checks the equal branch, unequal
loop branch, byte wrap, 16-bit offset wrap, and no-mutation boundary status.
The model does not claim parity for unmodeled CPU registers or later code.

## Reproduction

```sh
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
span = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()[0x2EBF7:0x2EC05]
print(len(span), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-after02fb-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-after02fb-p9 --target \
  poolrad_overlay_u3bd_post_up_tile_loop_gate_tests
ctest --test-dir resources/build-post-up-after02fb-p9 \
  -R '^poolrad_overlay_u3bd_post_up_tile_loop_gate_tests$' --output-on-failure
resources/build-post-up-after02fb-p9/poolrad_overlay_u3bd_post_up_tile_loop_gate_tests \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/after02fb-second-0300-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/after02fb-second-030e-ram-private.bin
```

The private replay's debugger command sequence is `BP 2D14:0300`, `BPLIST`,
`RUN`, one Up, capture first `0300`, `RUN`, capture second `0300`,
`BP 2D14:030E`, `BPLIST`, `RUN`, capture `030E`. Each capture uses
`MEMDUMPBIN 0000 0000 A0000`. Original media, save, RAM dumps, emulator
trace, and screenshots remain ignored under `resources/`.
