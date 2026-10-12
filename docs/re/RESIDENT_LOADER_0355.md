# Saved cold-loader continuation `+0355..+0362`

After the reached `+0553` helper returns, the saved cold loader resumes
at loaded `12C6:0355`. This bounded C stage covers five instructions
through the first next nested call, stopping **before** `+0363 CALL
+0538`. It models the complete CPU and RAM state at that call
boundary. The `+0538` callee, alternate `+0435` path, loader tail,
and handler tail remain separate. No full loader or game-identity
count is claimed.

Before implementation, `deja "Poolrad saved cold loader 12C6 0355
0363 0538 native continuation"` timed out during local index refresh
(exit 124). The merged [loader prefix](RESIDENT_LOADER_PREFIX_0338.md)
and [+0553 helper](RESIDENT_LOADER_0553.md) supplied the caller path
and CPU lead. This work independently rechecked the original and
loaded bytes and captured a fresh same-run native entry/pre-call pair.

## Byte and instruction boundary

| Evidence | Result |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Expanded original `0AA2:0355..0362` | 14 bytes, five 16-bit instructions; SHA-256 `19e093876566c075385306f87770bf4e21321599beec2a2459aa04c48859e30c` |
| Loaded `12C6:0355..0362` | Exact match to original in fresh entry RAM |
| Following instruction | `+0363 CALL +0538`, not executed by the C stage |

`+0355` loads `ES:[000A]` into DX; `+035A` adds `000F` with 16-bit
wrap; `+035D` sets CL to four; `+035F` shifts DX right by four; and
`+0361` adds AX to DX. The saved input word `ES:[000A]` is zero and
AX is `0047` from the merged `+0553` helper, so DX becomes `0047`.
The final ADD defines the arithmetic flags at the stop, so no
undefined intermediate SHR flags are exposed by this stage.

## Direct native parity

A fresh ignored run `slot-a-loader-0355-004` used pinned slot-A
`SAVGAMA.DAT`, SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
It first stopped at saved caller `2A8E:19E3`, then directly at
`12C6:0355` and pre-call `12C6:0363`. Its 198-byte numeric CPU
transcript has SHA-256
`5db88ac2de6f7227697f3553f901e879d8cd3ac1b73545aca7f55fefe82c70ee`.

| CPU field | `+0355` | Pre-call `+0363` |
| --- | --- | --- |
| `CS:IP` | `12C6:0355` | `12C6:0363` |
| `DX,FLAGS` | `449A,7216` | `0047,7206` |
| `AX,CX,SS:SP` | `0047,0004,1D16:3778` | unchanged |
| `DS,ES,BP,BX,SI,DI` | `14A0,08D0,3794,0027,44BA,0000` | unchanged |

`FS=GS=0000` at both stops. The two 655,360-byte RAM captures are
identical, SHA-256
`5c796d56b67719be33989782e2e26740ab68337cbb9fd70182dc32f6f5eacb7e`.
Compiled C reproduces every captured RAM byte and all 14 modeled CPU
fields at the pre-call stop. The upper 384 KiB of the C model's 1 MiB
RAM space was not captured, so no native parity claim applies there.

Synthetic tests cover the saved zero word, nonzero paragraph rounding,
16-bit size wrap, final ADD overflow and carry, and invalid arguments.
Other input values have instruction-derived synthetic evidence; the
saved zero route has the direct native pair.

Reproduce locally with ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_0355_audit.py
cmake -S . -B /tmp/poolrad-loader-0355-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-0355-build
ctest --test-dir /tmp/poolrad-loader-0355-build --output-on-failure
```

The audit prints only hashes, addresses, and counts. Original bytes,
RAM captures, saves, and media remain under ignored `resources/`.
