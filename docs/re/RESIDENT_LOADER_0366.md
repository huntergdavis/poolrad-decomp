# Saved cold-loader continuation `+0366` to first call

The restored slot-A route resumes at loaded `12C6:0366` after the
merged [+0538 helper](RESIDENT_LOADER_0538.md) returns. The saved path
reaches `12C6:03AA CALL FAR [8504]`; this C stage stops **before** that
call. An alternate unsigned branch stops before the internal
`+0369 CALL +03E9`, with its preceding `PUSH DX` applied. Neither
callee, the remaining loader, nor the alternate loop is translated
here. No full-loader, game-identity, or project count is claimed.

Before implementation, `deja "Poolrad saved cold loader 12C6 0366 next
call return native continuation"` found no matching indexed session.
The merged [+0355 continuation](RESIDENT_LOADER_0355.md) and
[+0538 helper](RESIDENT_LOADER_0538.md) supplied the caller chain and
CPU model convention. This stage independently rechecked original and
loaded instructions and captured a fresh native entry/pre-call pair.

## Original and loaded boundary

| Evidence | Result |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Expanded original `0AA2:0366..03A9` | 68-byte source window; SHA-256 `bc10e48da989984caa915f95651900f4ff21724debacd9448fae85877833803c` |
| Loaded `12C6:0366..03A9` | Exact match to original in fresh entry RAM |
| Next reached call | `+03AA CALL FAR [DS:8504]`, stopped before execution; saved pointer is `12C6:0268` |

The saved route executes eight instructions: `+0366 JMP +039C`,
`+039C SUB DX,AX`, `+039E JA +0368` (false), `+03A0 POP ES`,
`+03A1 MOV AX,[43F6]`, `+03A4 MOV ES:[0010],AX`, and `+03A8` and
`+03A9 PUSH ES`. On the alternate unsigned `DX > AX` input, `JA`
reaches `+0368 PUSH DX` and stops at `+0369 CALL +03E9`. The source
window contains the later internal loop and its calls; this stage
does not execute or claim parity for them.

## Direct native gate and C parity

A fresh ignored run `slot-a-loader-0366-006` used pinned slot-A
`SAVGAMA.DAT`, SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
It stopped first at saved caller `2A8E:19E3`, then directly at
`12C6:0366` and before the far call at `12C6:03AA`. The 232-byte
numeric CPU transcript has SHA-256
`3d2cc367f385d0dc991c6729f6ab697e5f28437eab22f8e6b7a8df5d9b96f338`.

| CPU field | Entry | Before far call |
| --- | --- | --- |
| `CS:IP` | `12C6:0366` | `12C6:03AA` |
| `SS:SP` | `1D16:3778` | `1D16:3776` |
| `AX,DX,ES,FLAGS` | `0102,0047,085C,7202` | `2CCD,FF45,08D0,7283` |
| `DS,BP,BX,CX,SI,DI` | `14A0,3794,0027,0004,44BA,0000` | unchanged |

`FS=GS=0000` at both stops. Saved `DX=0047` minus `AX=0102`
produces `FF45` with unsigned borrow, so `JA` is false. `POP ES`
restores `08D0` from `SS:3778`, and `DS:[43F6]=2CCD` becomes both AX
and `ES:[0010]`. The two `PUSH ES` instructions restore SP to `3776`.
The first overwrites its prior equal word at `SS:3778`; the second
replaces `SS:3776` from `0366` with `08D0`.

The 655,360-byte entry and pre-call RAM captures have SHA-256
`f5331f379ecfc6861781dc42b7128d85b48a262a439a50851a6620df4fe64245`
and `62aef24e08e15f99fe2627552a5e4ddc641ae493fbbbefa51f950d4c28c814e7`.
Exactly four captured bytes change: `08D0:0010..0011` and
`1D16:3776..3777`. Compiled C reproduces every captured RAM byte and
all 14 modeled CPU fields. The upper 384 KiB of the model's 1 MiB RAM
space was not captured, so no native parity claim applies there. The
saved borrow path is the only directly native-tested branch. Synthetic
tests cover that path, unsigned detour, equality, signed overflow,
stack-offset wrap, and invalid arguments.

Reproduce locally using ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_0366_audit.py
cmake -S . -B /tmp/poolrad-loader-0366-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-0366-build
ctest --test-dir /tmp/poolrad-loader-0366-build --output-on-failure
```

The audit prints only hashes, addresses, and counts. Original bytes,
RAM captures, saves, and media remain under ignored `resources/`.
