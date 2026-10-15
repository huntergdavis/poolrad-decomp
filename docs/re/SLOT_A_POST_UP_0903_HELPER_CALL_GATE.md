# Saved slot-A post-Up helper call at `0x3BD:+0903`

**Classification:** bounded reached caller transfer. The C gate executes the
eight instructions at `+0903..+0910` and stops at the merged `+047B` callee
entry. It matches a fresh exactly-one-Up native RAM pair byte for byte at
that boundary. The native callee return and the caller's zero branch are
observed here, but are outside this C gate. No art transfer, whole page, or
full-C view claim follows.

Before analysis, `deja "Poolrad U3BD 2D14:0903 helper call 047B post-Up
first-view object page"` was run with a 25-second bound and returned no
result. The [preceding +0938 gate](SLOT_A_POST_UP_0938_CALLER_CONTROL_GATE.md)
supplied the +0903 lead; the bytes and live states below were independently
checked in a new exactly-one-Up run. The merged [entry-2 model]
(U3BD_NIBBLE_ENTRY2.md) establishes the callee's byte-argument API and
its own semantics; this gate does not reproduce its CPU stack effects.

## Original and loaded bounds

Ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
the pinned save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.

| Unit span | Original file range | Bound | SHA-256 | Loaded |
| --- | --- | --- | --- | --- |
| `+0903..+0913` | `[0x2F1FA,0x2F20A)` | 16 bytes / 8 instructions | `ea612805f165c5f8f8661b886067e7fd483fb4bc5b165c14492f6a9efeff0447` | identical at `2D14` in the +0903 RAM |
| observed return branch `+0913..+091C` | `[0x2F20A,0x2F213)` | 9 bytes / 3 instructions | `63818af39243c81540f6bdecba8d75c46efb4527e2d16b86ed5c32f8ad070657` | identical at `2D14` in the +0913 RAM |

The caller loads `SS:[BP-0D]`, `SS:[BP-0F]`, and `SS:[BP-01]` into AL,
and pushes full AX after each load. It then pushes CS and near-calls
`+047B`, pushing return IP `+0913`. AH and FLAGS are unchanged by all
eight instructions. The callee receives byte arguments x, y, selector in
that order through the five-word call frame.

## Native exactly-one-Up frame

A dedicated heavy-debug replay loaded visible `(15,4,N; 10:50)` from the
pinned save. With only `2D14:0938` armed, exactly one gameplay Up reached
the preceding caller. Replacing that stop with `+0903`, then `+047B`,
then `+0913`, preserved the same run and call ordinal. At +0903,
`AX=5`, `BP=3EE0`, `SP=3EC8`, `SS=1D16`, `CS=2D14`, and `FLAGS=7293`.
The caller locals supplied x=12, y=5, selector=4.

| Stop | AX | SP | FLAGS | Full 640 KiB RAM SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `2D14:0903` | 5 | `3EC8` | `7293` | `4cb016560dcf10d86762565b5bd4156cd2411d30cee57967ae0aa0d163eaac1e` |
| `2D14:047B` | 4 | `3EBE` | `7293` | `76db12f5a26f345bbfdefb7abaa5906c4cd8887fe88ff50fba5fa3f8b2f448ca` |
| `2D14:0913` | 0 | `3EC8` | `7256` | `0dbc347d39a02ab3805297442f12414a1d522afa1ae4e30ae8b99a1fafaedb7b` |

At +047B, the words starting at `SS:SP` are `0913`, `2D14`, `0004`,
`0005`, `000C`. Exactly seven physical RAM byte **values** differ between
the +0903 and +047B captures, all in the stack interval
`0x2101E..0x21026`; same-value writes within that interval are not
visible in a byte diff. The native +047B return has AL=0. Breakpoints
at both `+0951` and the adjacent art arm `+093A` then stopped at `+0951`,
consistent with the decoded `+0913` local store, zero comparison, and
`JE +0951`. This is one measured call outcome, not a general branch claim.

## C boundary and verification

`pr_u3bd_post_up_0903_helper_call` retains AH, FLAGS, and the original
instruction order. Guest addresses use 20-bit physical and 16-bit offset
wrap. A missing reached read or stack write returns `BOUNDARY` with the
prior effects and failing IP visible. Successful transfer returns the
explicit `CALLEE_047B` status and stops before the callee executes.

Synthetic strict tests cover the five pushed words, AH and FLAGS
preservation, an unavailable first read, and an unavailable first stack
write. A private test starts C from the same-run +0903 RAM and measured
registers; it reaches +047B with **655,360/655,360 RAM bytes** identical
to the native entry, plus exact AX, SP, FLAGS, and IP. The ignored private
captures and original media are not tracked.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
image = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
base = 0x2E8F7
for lo, hi in ((0x0903, 0x0913), (0x0913, 0x091C)):
    span = image[base + lo:base + hi]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, lo))
    assert ins[-1].address + ins[-1].size == hi
    print(hex(lo), len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-0903-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-0903-p9 --target \
  poolrad_overlay_u3bd_post_up_0903_helper_call_tests
ctest --test-dir resources/build-post-up-0903-p9 \
  -R '^poolrad_overlay_u3bd_post_up_0903_helper_call_tests$' --output-on-failure
resources/build-post-up-0903-p9/poolrad_overlay_u3bd_post_up_0903_helper_call_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/helper-0903-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/helper-047b-entry-ram-private.bin
```
