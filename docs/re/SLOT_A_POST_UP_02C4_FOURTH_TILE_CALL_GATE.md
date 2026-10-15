# Saved slot-A post-Up fourth tile call at unit `0x3BD:+02C4`

**Classification:** bounded caller preparation and typed callback gate from
`+02C4` through far call `+02FB`. The C prefix matches all 655,360 native
RAM bytes at the fourth pre-call stop. The existing tile-plane callback
matches the direct-return 14,135-byte object and 1,176-byte dirty-metadata
span. The external callee's remaining register, stack-history, and RAM
effects are outside this model; this is not a complete view or full-C
parity claim.

Before investigation, `deja "Poolrad slot A post Up U3BD 02C4 tile
argument callback 02FB third outer view item19 native caller frame"`
was run with a 25-second bound and returned no reusable result. The
[preceding view-table gate](SLOT_A_POST_UP_02A3_VIEW_TABLE_GATE.md)
supplied the reached item-19 lead. The
[third tile-call gate](SLOT_A_POST_UP_02FB_THIRD_PAGE_WRITER_GATE.md)
supplied the earlier call order and existing tile-plane adapter. This
fourth tuple and direct return were measured in a fresh replay.

## Pinned source and argument order

Ignored DOS v1.3 `GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
`START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
the ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

| Span | Original file range | Bound | SHA-256 |
| --- | --- | --- | --- |
| `0x3BD:+02C4..+02FB`, preparation | `GAME.OVR [0x2EBBB,0x2EBF2)` | 55 bytes / 25 aligned instructions | `705feb4d2fb63a6a03ae3479bb7a1a45cdd1cf895329bc7b31917f6814beb0e2` |
| `0x3BD:+02FB..+0300`, far call | `GAME.OVR [0x2EBF2,0x2EBF7)` | 5 bytes / one aligned instruction | `64940f6941110590c5125977fe8e435e68b57a1fc5b998a494f76b176f0050ec` |
| whole bounded caller | `GAME.OVR [0x2EBBB,0x2EBF7)` | 60 bytes / 26 aligned instructions | `6fc892d3c2204e81f390091e7acf3f54c4db4ebee6d5bc77305cbf6c07b3bfaa` |

The caller reads inner `SS:[BP-6]` and outer `SS:[BP-8]`, adds two to
each via `ADD AX,3; DEC AX`, and pushes those coordinate words. It then
recomputes the same far `[DS:6A58]` piece-buffer address used in the
preceding gate, zero-extends the item byte, and pushes item, 1, 1. At
`+02FB`, the original image encodes a far call whose loaded target in this
run is `0C4C:002A`. Its typed callback receives `(x,y,item,1,1)` in
named order. A missing callback returns `CALLBACK_REQUIRED` after all five
pushes, preserving the pre-call RAM and SP. A successful callback reports
the direct `+0300` return and restored caller SP. The model does not assign
post-callee values to the other registers.

## Fresh native exactly-one-Up route

A fresh private DOSBox-X heavy-debug run loaded visible `(15,4,N; 10:50)`
from the pinned save. Only `BP 2D14:02C4` was armed at that playable
frame, then exactly one gameplay Up was sent. Four hits in the
`BP=3EBA`, `SS=1D16` frame had outer words 3, 4, 5, and 6. The fourth
entry had inner word 4 and `SS:SP=1D16:3EAC`; its 640 KiB RAM SHA-256
was `6622eccdec21e6cf2db5ec6a609819a95d5f4bfbae12a4b3575ba633756fb227`.

After arming `BP 2D14:02FB`, native stopped before the fourth far call
with `SS:SP=1D16:3EA2`. Stack words at SP+0..+8 were
`(1,1,19,8,6)`, proving `(x,y,item,1,1)=(6,8,19,1,1)`. Its RAM
SHA-256 was `7daf83a986fe5717eebf8acc852b8d96ce7ee338d9a4c03d38dd160699aa9ba8`.
Only two RAM byte **values** changed from the fourth +02C4 entry; other
argument stores wrote values already present at those addresses.

The far call returned directly to `2D14:0300` with SP restored to
`3EAC`. Return RAM SHA-256 was
`505ffa7cbfbeae0bb27d64d6b033e55ebbb742c45f6eea5837ad48df098b7247`.
At live destination far `3DE4:0008` (physical `0x3DE48`), the
14,135-byte offscreen object changed eight byte values; its pre/post
SHA-256 values were
`bfa58e2aaa9d5089a49e0b367655746d5720fd9372b500e866ab8fe6a82cee50`
and `d477772f7b02d7fe698d9c3dd96000e51611ca19c1083201544d68f5cf4acab6`.
The 1,176-byte `DS:4A28..4EBF` dirty-metadata span had zero value
changes, SHA-256
`c98d0d825711966e14b4da5933a8e53cd371111c6e537f7642cb6658c30028ea`.
Full RAM had 18 changed byte values across the call; no claim is made
that the callback model reproduces those other bytes.

## C/native checks and reproduction

The private test starts from the exact fourth +02C4 RAM. At callback
entry it compares **655,360/655,360 bytes** with the same-run native
+02FB RAM and checks the stack words. It then calls the existing
`pr_u40c_entry2_mode1_tile_plane` through the typed callback, deriving
source, auxiliary, and destination far pointers from the live RAM.
The model performs 32 plane-byte writes, changes eight object values,
and matches **14,135/14,135 object bytes** plus **1,176/1,176 metadata
bytes** with the direct native +0300 return. Synthetic tests cover the
callback-required boundary, one successful and one failed callback, and
an unavailable item after both coordinate pushes. The external callback's
other effects remain an explicit boundary.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
image = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
span = image[0x2EBBB:0x2EBF7]
ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, 0x02C4))
assert len(ins) == 26 and ins[-1].address + ins[-1].size == 0x0300
print(len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-02c4-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-02c4-p9 -j 8
ctest --test-dir resources/build-post-up-02c4-p9 --output-on-failure
resources/build-post-up-02c4-p9/poolrad_overlay_u3bd_post_up_02c4_tile_call_tests \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/02c4-fourth-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/02c4-fourth-02fb-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-view-writer-p9-118/02c4-fourth-0300-ram-private.bin
```

Original media, save, debugger trace, screenshots, and RAM captures remain
ignored under `resources/`.
