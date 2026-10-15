# Saved slot-A post-Up caller control from `0x3BD:+0938`

**Classification:** bounded reached caller path across three noncontiguous
original-byte spans, ending at the first unsigned loop decision. The C gate
matches the same-run 640 KiB native RAM at `2D14:0903`, plus the measured
AX, DI, SP, and FLAGS values. It stops before the `+0903` helper setup and
never enters the adjacent `+093A..+0950` art-call arm. This is not a
complete view, object-to-frame transfer, or full-C parity claim.

Before analysis, `deja "poolrad slot-A exactly one Up U3BD return
2D14:0938 caller first control branch before object-to-frame p9 p5"`
was run with a 25-second bound and returned no reusable result. The
[preceding caller exit](SLOT_A_POST_UP_0319_CALLER_EXIT_GATE.md) provided
the return-address lead; the addresses, branch, and state here were checked
in a fresh exactly-one-Up replay. p5 was notified that this gate excludes
the art transfer and later page conversion.

## Original bytes and reached control path

Ignored DOS v1.3 `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` SHA-256 is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The ignored slot-A save SHA-256 is
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
All three loaded `2D14` spans match the corresponding original bytes.

| Reached unit span | Original `GAME.OVR` range | Bound | SHA-256 |
| --- | --- | --- | --- |
| `+0938..+093A` | `[0x2F22F,0x2F231)` | 2 bytes / 1 instruction | `78ea03653da8d709194b9d4b6797122719ccb33159e2c25720f49d00d4cbf827` |
| `+0951..+097F` | `[0x2F248,0x2F276)` | 46 bytes / 17 instructions | `7f318f344e9e134a540795af7731bfec7af0751a5054c4a743c9d00fab19d96b` |
| `+08FD..+0903` | `[0x2F1F4,0x2F1FA)` | 6 bytes / 2 instructions | `c78c6abc35d1e8414221738883a556620ae23838b28b88fb24cfacbd3a6613cc` |

The reached `+0938 JMP +0951` skips the `+093A..+0950` arm containing
the `+0211` art call. At `+0951`, the caller increments `SS:[BP-0x10]`,
subtracts two from `SS:[BP-0x12]`, indexes DS direction bytes from
`DS:[0x2736+index]` and `DS:[0x273F+index]`, and adds the signed values
to local x/y words. The live DOSBox-X 16-bit debugger decodes `+0963`
and `+0975` as `CBW`; the model sign-extends AL at both sites. `+097C`
jumps to `+08FD`, where `CMP byte SS:[BP-0x10],3` and `JAE +097F`
select `+097F` for unsigned greater-or-equal and `+0903` otherwise.
No reached call occurs before this boundary.

## Fresh native exactly-one-Up path

A dedicated heavy-debug replay loaded visible `(15,4,N; 10:50)` from the
pinned save. At the playable frame, only `BP 2D14:0938` was armed; exactly
one gameplay Up reached that stop. Native had `SS:BP=1D16:3EE0`,
`SP=3EC8`, `DS=14A0`, `AX=6`, `DI=0x8E`, and `FLAGS=7246`. The +0938 RAM
SHA-256 was
`3c0173e0310cbbb7e71fd4155597640801931a09d2f3e05b83b43c96c5837abe`.
Replacing the breakpoint with `+0951` reached the direct jump target
with the same full RAM hash and zero changed values.

At `+0951`, the local loop byte was 0, step word 0, x/y words 12/4,
and local table index 4. The reached direction bytes were 0 and 1.
At the next stop `+097C`, the loop byte was 1, step word 65534, and
x/y words 12/5. Native had `AX=5`, `DI=4`, `FLAGS=7206`; full RAM SHA-256
was
`d5e110dcab3d4af1c4bd1f2515b215c3356f839d7e08cd3a5618500b024224bf`.
Compared with same-run `+0951`, exactly four local byte **values** changed,
at physical `0x2102E..0x21031`. The `+097C` jump reached `+08FD`.
With both branch destinations armed, native stopped at `+0903`, confirming
`JAE +097F` was not taken for loop byte 1. At `+0903`, `AX=5`, `DI=4`,
`SP=3EC8`, `FLAGS=7293`, and RAM retained the `+097C` hash. The debugger
did not enter the art arm or the subsequent `+047B` helper.

## C/native boundary and tests

`pr_u3bd_post_up_0938_control` follows the original instruction order,
including 16-bit segment-offset wrap, signed `CBW`, arithmetic flags,
and the unsigned `JAE`. Each guest memory operation checks availability
when reached; a `BOUNDARY` leaves prior operations' effects and the failing
IP visible. `XOR AH,AH` leaves AF architecturally undefined, recorded by
`known_arith_flags`; the reached final `CMP` defines all arithmetic flags.
The model ends at `+0903` or `+097F` before either successor body.

The private test initializes C from the same-run `+0938` RAM and measured
registers. It reaches `+0903` with **655,360/655,360 RAM bytes** identical
to the native target, plus exact AX=5, DI=4, SP=3EC8, and FLAGS=7293.
Synthetic strict tests cover both branch outcomes, signed negative table
bytes, missing first RAM and table reads with prior effects retained, and
the successful frame. No object-to-frame or whole-page result is inferred.

```sh
PYTHONPATH=/tmp/poolrad-capstone python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
image = Path('resources/DOS/POOLRAD/GAME.OVR').read_bytes()
base = 0x2EC10 - 0x0319
for lo, hi in ((0x0938, 0x093A), (0x0951, 0x097F), (0x08FD, 0x0903)):
    span = image[base + lo:base + hi]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, lo))
    assert ins[-1].address + ins[-1].size == hi
    print(hex(lo), len(span), len(ins), sha256(span).hexdigest())
PY
cmake -S . -B resources/build-post-up-0938-p9 \
  -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror' -DBUILD_TESTING=ON
cmake --build resources/build-post-up-0938-p9 --target \
  poolrad_overlay_u3bd_post_up_0938_control_tests
ctest --test-dir resources/build-post-up-0938-p9 \
  -R '^poolrad_overlay_u3bd_post_up_0938_control_tests$' --output-on-failure
resources/build-post-up-0938-p9/poolrad_overlay_u3bd_post_up_0938_control_tests \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/control-0938-pre-ram-private.bin \
  resources/oracle-runs/slot-a-post-up-exit-p9-119/control-0903-branch-ram-private.bin
```

Original media, save, screenshots, debugger trace, and RAM dumps remain
ignored under `resources/`.
