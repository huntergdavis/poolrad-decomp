# Resident cold-loader helper `+0538`

The restored slot-A cold-loader route reaches loaded `12C6:0538` from
`12C6:0363 CALL +0538`. This C stage covers the complete 27-byte helper
through its near `RET` to `12C6:0366`. The larger cold loader and resident
handler remain separate; this change makes no full-loader, game-identity, or
project count claim.

Before implementation, `deja "Poolrad saved cold loader 12C6 0538 0363
native helper CPU RAM"` timed out during local index refresh (exit 124).
The merged [+0355 continuation](RESIDENT_LOADER_0355.md) supplied the
saved caller boundary, and the merged [+0553 helper](RESIDENT_LOADER_0553.md)
supplied the private entry/return audit pattern. This work independently
checked the original and loaded instructions and captured a fresh native
entry/return pair.

## Byte and control-flow boundary

| Evidence | Result |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Expanded original `0AA2:0538..0552` | 27 bytes, ten 16-bit instructions, two conditional branches, no nested calls; SHA-256 `42bb665c1e5b081cbc73aea256d7973ee33e38aa8d181e465807adcad450791d` |
| Loaded `12C6:0538..0552` | Identical to original in the fresh entry RAM |

At `+0538`, AX receives `DS:[43FA]`. `+053D JE +054B` sends a zero
selector to the fallback subtraction. Otherwise `+053F` assigns it to ES,
`+0541` reads `ES:[0010]`, and `+0545` subtracts `DS:[43F6]`. The
unsigned no-borrow branch at `+0549 JAE +0552` returns this direct
difference. On borrow, `+054B` instead loads `DS:[43F8]`, then `+054E`
subtracts the same base. `+0552` returns near through `SS:SP` on all
paths. ES changes on both nonzero-selector paths, including fallback;
the zero-selector path retains incoming ES. The final SUB defines the
exposed arithmetic flags. No RAM writes occur.

## Direct native gate and C parity

A fresh ignored run `slot-a-loader-0538-005` used pinned slot-A
`SAVGAMA.DAT`, SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
It first stopped at saved caller `2A8E:19E3`, then directly at helper
entry `12C6:0538` and caller return `12C6:0366`. The numeric CPU
transcript is 242 bytes, SHA-256
`a705ceb58dcb250b58237bb4dd4bb76c4414d3509ca529c5ee2f80398767d50d`.

| CPU field | Entry | After RET |
| --- | --- | --- |
| `CS:IP` | `12C6:0538` | `12C6:0366` |
| `SS:SP` | `1D16:3776` | `1D16:3778` |
| `AX,ES,FLAGS` | `0047,08D0,7206` | `0102,085C,7202` |
| `DS,BP,BX,CX,DX,SI,DI` | `14A0,3794,0027,0004,0047,44BA,0000` | unchanged |

`FS=GS=0000` at both stops. The saved words are `DS:[43F6]=2CCD`,
`DS:[43F8]=3996`, `DS:[43FA]=085C`, and `085C:[0010]=2DCF`.
Therefore the reached path computes `2DCF - 2CCD = 0102`, with no
unsigned borrow. The saved return word at `SS:3776` is `0366`.

Both 655,360-byte RAM captures are identical, SHA-256
`228032a610412cbde1922f1dc817584d2c36599ddf6ba47f71cccd1770708669`.
Compiled C reproduces every captured RAM byte and all 14 modeled CPU
fields from same-run entry to return. The upper 384 KiB of the C model's
1 MiB RAM space was not captured and is initialized to zero for the
audit; no native parity claim applies there. Only the no-borrow path was
directly native-tested. Synthetic tests exercise the zero-selector
fallback, nonzero borrowed fallback, equality, signed overflow with
no unsigned borrow, 16-bit stack-offset wrap, and invalid arguments.
Those paths are instruction-derived.

Reproduce locally using ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_0538_audit.py
cmake -S . -B /tmp/poolrad-loader-0538-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-0538-build
ctest --test-dir /tmp/poolrad-loader-0538-build --output-on-failure
```

The audit prints only hashes, addresses, and counts. Original bytes,
RAM captures, saves, and media remain under ignored `resources/`.
