# Saved slot-A post-Up outer arm at unit `0x3BD:+026E`

**Classification:** bounded ten-instruction caller arm from `+026E` to
`+028B` or `+030E`. The C model covers both signed comparison outcomes and
matches the complete 655,360-byte guest RAM image for the reached third
post-Up arm. It ends before the `+028B` bounds tests; it is not a complete
view, display, CPU, or full-C parity claim.

Before debugging, `deja "Poolrad slot A exactly one Up U3BD +026E outer loop
after third 02FB tile return 030E 026E next native caller view"` was run
with a 25-second bound and returned no reusable result. The checked-in
[third tile-call gate](SLOT_A_POST_UP_02FB_THIRD_PAGE_WRITER_GATE.md) and
[outer decision gate](SLOT_A_POST_UP_030E_OUTER_LOOP_GATE.md) supplied the
preceding ordinal and `+030E→+026E` route. The new replay independently
measured the `+026E` continuation.

## Pinned source and bounded semantics

Ignored DOS v1.3 `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
`START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
Unit `0x3BD:+026E..+0288` is original
`GAME.OVR [0x2EB65,0x2EB7F)`, **26 bytes / ten aligned 16-bit
instructions**, SHA-256
`0d8fe83f3204d65855ad2ccf065866fbc628a920bad378a0b3f39c874d0e60f5`.
The live 16-bit debugger decodes `+027A` as `CBW`.

| Offset | Operation | Bounded effect |
| --- | --- | --- |
| `+026E` | `INC word SS:[BP-8]` | Increment outer word modulo 65536. |
| `+0271` | `MOV AX,SS:[BP-2]` | Load copied inner word. |
| `+0274` | `MOV SS:[BP-13],AX` | Set comparison word. |
| `+0277` | `MOV AL,SS:[BP+12]`; `+027A CBW` | Sign-extend source byte to AX. |
| `+027B` | `CMP AX,SS:[BP-13]` | Signed comparison flags. |
| `+027E` | `JLE +0283` | Take inner-word update when AX is signed less or equal. |
| `+0280` | `JMP +030E` | Skip update for signed greater. |
| `+0283` | `MOV SS:[BP-6],AX`; `+0286 JMP +028B` | Store inner word, continue bounds tests. |

The C model follows instruction order. A synthetic missing operand returns a
boundary status at the first reached access while retaining preceding register,
flag, and memory effects. SS offsets wrap at 16 bits. `INC` preserves CF;
the later `CMP` replaces all arithmetic flags on successful completion.
The separate `+0288` entry from the inner-loop unequal branch is outside this
arm.

## Fresh native one-Up route

A fresh private DOSBox-X heavy-debug run loaded visible `(15,4,N; 10:50)`.
Only `BP 2D14:026E` was armed at that playable frame, then exactly one
gameplay Up was sent. It hit three times in the `BP=3EBA` caller frame, with
`SS:[BP-8]` values 3, 4, and 5. The third stop is the continuation after
the third tile call and `+030E` unequal branch. At that stop,
`SS:SP=1D16:3EAC`, `SS:[BP-8]=5`, `SS:[BP-2]=4`, `SS:[BP-13]=4`,
`SS:[BP-6]=4`, and `SS:[BP+12]=4`. Its ignored 640 KiB RAM SHA-256 was
`4dfdbfe0bf1cb3cd32eb48893cf57f60d01a55ffc5f630a73a0a46a7d58ab11a`.

After arming `BP 2D14:028B` and resuming, the debugger stopped there
directly. The outer word was 6, AX was 4, and the visible arithmetic flag
bits were CF=0, ZF=1, SF=0, OF=0, AF=0, PF=1. Its RAM SHA-256 was
`17aae9abf155a14577f9bbb54fa80de1380d90321720932b0497caa62d3c3cc8`.
Exactly **one** RAM byte value differed: physical `0x21012`, from 5 to 6.
The copied and inner words already held 4, so their original-byte stores
did not change values. A value comparison does not count same-value writes.

## C/native boundary and reproduction

The synthetic test checks the reached signed-less-or-equal arm, signed-greater
exit, negative `CBW` input, whole-SS unchanged-value expectation for the
reached case, and early/late boundary effects. The private mode initializes
the C SS segment from the fresh third `+026E` RAM, runs the model once,
asserts outer 5→6, AX=4, next IP=`+028B`, and compares **655,360/655,360
RAM bytes** with the matching native target. With input FLAGS=`0x7297`,
the model computes FLAGS=`0x7246`; the full native FLAGS word was not dumped.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
span = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()[0x2EB65:0x2EB7F]
ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, 0x026E))
assert len(ins) == 10 and ins[-1].address + ins[-1].size == 0x0288
print(len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-026e-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-026e-p9 -j 8
ctest --test-dir resources/build-post-up-026e-p9 --output-on-failure
resources/build-post-up-026e-p9/poolrad_overlay_u3bd_post_up_026e_arm_tests \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/026e-third-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/026e-third-028b-ram-private.bin
```

Original media, save, debugger trace, screenshots, and RAM captures remain
ignored under `resources/`.
