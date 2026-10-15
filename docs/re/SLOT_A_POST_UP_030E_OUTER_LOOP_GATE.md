# Saved slot-A post-Up outer-loop gate at unit `0x3BD:+030E`

**Classification:** four-instruction, call-free caller decision after the
third reached `2D14:02FB` tile call. The C model covers both `+030E`
comparison outcomes and matches all 655,360 guest RAM bytes on the native
one-Up unequal path to `+026E`. This gate ends before the `+026E` instruction;
it is not a complete view, display, CPU, or full-C parity claim.

Before debugging, `deja "Poolrad slot A one Up third 2D14:02FB call return
+0300 +030E next tile caller fourth view page writer"` was run with a
25-second bound and returned no reusable result. The checked-in
[third tile-call gate](SLOT_A_POST_UP_02FB_THIRD_PAGE_WRITER_GATE.md) and
[inner loop gate](SLOT_A_POST_UP_0300_TILE_LOOP_GATE.md) supplied the
preceding ordinal and exact `+0300..+030E` semantics. This fresh replay
independently observed the next branch.

## Pinned source and bounded body

Ignored DOS v1.3 `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
`START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Unit `0x3BD:+030E..+0319` is original `GAME.OVR [0x2EC05,0x2EC10)`,
**11 bytes / four aligned 16-bit instructions**, SHA-256
`b7aea1dbbb51cce5b72947c3dc39c36814b9bc8af82f4116374650456823f69f`:

| Offset | Operation | Effect |
| --- | --- | --- |
| `+030E` | `MOV AX,SS:[BP-8]` | Load the outer-loop word. |
| `+0311` | `CMP AX,SS:[BP-11]` | Replace arithmetic flags from 16-bit subtraction. |
| `+0314` | `JE +0319` | Equal: return through the caller epilogue. |
| `+0316` | `JMP +026E` | Unequal: continue the outer loop. |

The C model checks both two-byte SS reads before changing AX or flags, wraps
each guest offset at 16 bits, preserves non-arithmetic flag bits, and returns
the reached next IP. It makes no RAM writes.

## Fresh native one-Up branch

A private DOSBox-X heavy-debug run loaded visible `(15,4,N; 10:50)`.
A load-time `0300` breakpoint hit during view drawing and was deleted;
the debugger breakpoint list was empty when execution resumed to that
visible frame. At the frame, Alt+Pause armed only `BP 2D14:0300`, and
exactly one gameplay Up was sent. The breakpoint then hit three post-Up
returns in order, each in the `BP=3EBA` caller frame. The third return had
`SS:SP=1D16:3EAC`; at `SS:[BP-9]` the byte was 4, and the inner words
`SS:[BP-6]` and `SS:[BP-13]` were both 4. Its ignored 640 KiB RAM SHA-256
was
`1b5992c8d43a154c790c1dcd9df7d5fa61517f31d7f2f628e55f0debf3817029`.

A breakpoint at `2D14:030E` was reached next. At that stop,
`SS:[BP-9]` was 5, `SS:[BP-8]` was 5, and `SS:[BP-11]` was 6.
Its full RAM SHA-256 was
`1431dbdb830316f01a5a45356deebbfbd9b19a4f8b2bbbebbf534f395125d837`.
Compared with the third return, exactly one guest RAM byte value changed:
physical `0x21011` (`SS:[BP-9]`) from 4 to 5. After arming
`BP 2D14:026E` and resuming, the debugger stopped there, confirming the
unequal branch. Its RAM SHA-256 was the same
`1431dbdb830316f01a5a45356deebbfbd9b19a4f8b2bbbebbf534f395125d837`;
**zero** RAM byte values changed over `+030E..+0319`. The native debugger
displayed CF=1, ZF=0, SF=1, AF=1 at `+026E`. These are observed flag bits;
the complete FLAGS word was not independently dumped at that stop.

## C/native result and reproduction

The synthetic test checks the reached 5-versus-6 unequal branch, the equal
epilogue branch, untouched SS memory, bounds rejection without state change,
and 16-bit offset wrap. The private mode initializes C SS memory from the
fresh `+030E` RAM and uses `0x7246` as the input FLAGS word, then runs the model once,
checks AX=5 and next IP=`+026E`, then compares **655,360/655,360 RAM bytes**
with the native target. The model computes FLAGS=`0x7297` from the input
`0x7246` by 8086 `CMP` semantics; no full native FLAGS parity is claimed.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
span = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()[0x2EC05:0x2EC10]
ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, 0x030E))
assert ins[-1].address + ins[-1].size == 0x0319
print(len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-outer-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-outer-p9 -j 8
ctest --test-dir resources/build-post-up-outer-p9 --output-on-failure
resources/build-post-up-outer-p9/poolrad_overlay_u3bd_post_up_outer_loop_gate_tests \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/after-third-030e-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/after-third-026e-ram-private.bin
```

Original media, save, screenshots, debugger trace, and RAM captures remain
ignored under `resources/`.
