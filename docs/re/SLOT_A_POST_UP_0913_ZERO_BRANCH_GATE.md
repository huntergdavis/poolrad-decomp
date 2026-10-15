# Saved slot-A post-Up zero branch at `0x3BD:+0913`

**Classification:** bounded caller continuation after the `+047B` helper
returns. The C gate executes only `+0913..+091A` and stops at `+0951` when
AL is zero or `+091C` otherwise. It does not execute the callee, either
successor body, the art arm, or the later printer. This is no full-C view
or page parity claim.

Before analysis, `deja "Poolrad slot A post-Up U3BD 2D14:0913 +047B return
zero JE 0951 caller branch"` was run with a 25-second bound and returned
no reusable result. The separately [reviewed +0903 call gate]
(SLOT_A_POST_UP_0903_HELPER_CALL_GATE.md) supplied the return-address
lead; this span and the live +0913/+0951 pair were checked in a fresh run.

## Original-byte and loaded bound

Ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
the pinned save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

The three instructions `+0913..+091C` occupy original `GAME.OVR`
`[0x2F20A,0x2F213)`: **9 bytes**, SHA-256
`63818af39243c81540f6bdecba8d75c46efb4527e2d16b86ed5c32f8ad070657`.
Loaded `2D14` bytes in the native +0913 RAM match this span exactly.
The ordered effects are `MOV SS:[BP-15],AL`, `CMP byte SS:[BP-15],0`, and
`JE +0951`; fallthrough is `+091C`.

## Measured one-Up route

A private heavy-debug replay loaded the pinned visible `(15,4,N; 10:50)`
frame. The sole route anchor `2D14:0938` was reached by exactly one
gameplay Up. It was replaced by `2D14:0913`, the immediate return of the
separately modeled `+047B` helper. From +0913, both `+0951` and the
adjacent `+093A` art arm were armed; native stopped first at `+0951`.

| Stop | AX | BP | SP | FLAGS | Full 640 KiB RAM SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- |
| `2D14:0913` | 0 | `3EE0` | `3EC8` | `7256` | `bc7b9ae947fb74d40c88ccdef34fca5219730bc9a80b37aeaf0502ab5844a634` |
| `2D14:0951` | 0 | `3EE0` | `3EC8` | `7246` | `08a74dd745191e67957caa884746e5f1b6b17045e5b48d30d48fd59069400f61` |

`SS=1D16`, `CS=2D14`. Exactly one RAM byte **value** changed:
physical `0x2102B`, `SS:[BP-15]`, from 14 to 0. The zero comparison
cleared AF and set ZF/PF while preserving unrelated flags. This proves
only the reached zero arm for this saved one-Up route; synthetic tests
cover the nonzero fallthrough as well.

## C boundary and verification

`pr_u3bd_post_up_0913_zero_branch` writes the reached byte, sets all
arithmetic flags for byte comparison with zero, and selects the original
next IP. It uses real-mode 20-bit physical and 16-bit offset wrap. A
missing reached RAM write returns `BOUNDARY` at +0913 without effects.

The strict synthetic test checks zero and nonzero branch outcomes,
sign/parity flags, a missing reached write, and invalid entry state.
Starting from the same-run private +0913 RAM and registers, C reaches
`+0951` with **655,360/655,360 RAM bytes** equal to the native target,
plus exact AX, BP, SP, FLAGS, and IP. All original media, captures, and
RAM dumps remain ignored under `resources/`.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
image = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
lo, hi = 0x0913, 0x091C
span = image[0x2E8F7 + lo:0x2E8F7 + hi]
ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, lo))
assert ins[-1].address + ins[-1].size == hi
print(len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-0903-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-0903-p9 --target \
  poolrad_overlay_u3bd_post_up_0913_zero_branch_tests
ctest --test-dir resources/build-post-up-0903-p9 \
  -R '^poolrad_overlay_u3bd_post_up_0913_zero_branch_tests$' --output-on-failure
resources/build-post-up-0903-p9/poolrad_overlay_u3bd_post_up_0913_zero_branch_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/branch-0913-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/branch-0951-post-ram-private.bin
```
