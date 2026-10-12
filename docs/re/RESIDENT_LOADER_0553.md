# Resident cold-loader helper `+0553`

The saved cold-loader path reaches loaded `12C6:0553` from
`12C6:0352 CALL +0553`. This helper is 12 bytes and five
instructions, with no nested calls or branches. Its near `RET` reaches
the caller at `12C6:0355`. The C stage covers the full helper body
from callee entry through that return, including CPU, flags, and
stack-pointer effects. It does not cover the later cold-loader body,
the `+0435` alternate path, or the handler tail. No full cold-loader
or game-identity count is claimed.

Before implementation, `deja "Poolrad saved cold loader 12C6 0553
callee native RAM CPU parity"` timed out during local index refresh
(exit 124). The merged [loader-prefix note](RESIDENT_LOADER_PREFIX_0338.md)
and [saved first-insertion CPU note](SLOT_A_NATIVE_FIRST_INSERT_COLD_CPU.md)
provided the saved caller and segment. This work independently
rechecked the exact original/loaded bytes and captured a fresh native
entry and return pair.

## Original and loaded span

| Evidence | Result |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Expanded original `0AA2:0553..055E` | 12 bytes, five 16-bit instructions; SHA-256 `e7102f215250c40e24f1944f4f12412f821e5b86d84ad543487963f2cd1a2ab6` |
| Loaded `12C6:0553..055E` | Identical to original in the fresh entry RAM |

`+0553` reads the word at `ES:[0008]` into AX. `+0557` adds `000F`
with 16-bit wrap; `+055A` sets CL to four; `+055C` shifts AX right
by CL; `+055E` returns near through the word at `SS:SP`. The saved
input word is `0463`, so AX becomes `0472` after ADD and `0047`
after SHR. The caller's return word is `0355`.

## Direct native gate and C parity

A fresh ignored run `slot-a-loader-0553-003` used the pinned slot-A
save with SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
It first stopped at the saved caller `2A8E:19E3`, then directly at
callee entry `12C6:0553` and caller return `12C6:0355`. The numeric
CPU transcript is 208 bytes, SHA-256
`6ba970b477060bbeb5cef267b7b71fd3a8028825ee686cd15b0de3eee5873b13`.

| CPU field | Entry | After RET |
| --- | --- | --- |
| `CS:IP` | `12C6:0553` | `12C6:0355` |
| `SS:SP` | `1D16:3776` | `1D16:3778` |
| `AX,CX,FLAGS` | `37AE,0007,7202` | `0047,0004,7216` |
| `DS,ES,BP,BX,DX,SI,DI` | `14A0,08D0,3794,0027,449A,44BA,0000` | unchanged |

`FS=GS=0000` at both stops. Both 655,360-byte RAM captures are
identical, SHA-256
`dca3585cff1400ba8fb5769a0fb8db92f57cdf774691a5392270fdd160121ff4`.
Compiled C reproduces every captured RAM byte and all 14 modeled CPU
fields at return from the same-run entry. The upper 384 KiB of the
C model's 1 MiB RAM space was not captured and is initialized to zero
for the audit; no native parity claim applies there.

SHR with count four defines CF, SF, ZF, and PF. AF and OF are
architecturally undefined for this count. The C model retains the
immediately preceding ADD's AF/OF, matching the observed saved DOSBox
flags; no native claim is made for those bits under other input words.
Synthetic tests cover the saved input, zero input, 16-bit ADD wrap,
near-return stack-word crossing at offset `FFFF`, and argument guards.
The saved input is the one directly native-tested path; the other
values are instruction-derived tests.

Reproduce locally using ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_0553_audit.py
cmake -S . -B /tmp/poolrad-loader-0553-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-0553-build
ctest --test-dir /tmp/poolrad-loader-0553-build --output-on-failure
```

The audit prints only hashes, addresses, and counts. Original bytes,
RAM captures, saves, and media remain under ignored `resources/`.
