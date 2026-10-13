# Saved resident loader post-read branch `12C6:02A1`

The merged [DOS read event](RESIDENT_LOADER_DOS_READ_029F.md) stops at
loaded `12C6:02A1`. A fresh native slot-A run directly stopped there and
at `12C6:02E1`, before the loader's `POP DS` epilogue. The saved route
passes the first-read error and short-read gates, skips the optional
second DOS read because its count is zero, then clears AX. The bounded C
stage follows those gates and stops at `+02E1` or before the optional
`+02C0 INT 21h`. It does not complete the enclosing loader or claim a
full-C/game-identity count.

Before debugging, `deja "Poolrad saved resident loader 12C6 02A1 JB
after DOS AH3F read continuation native CPU RAM"` found no indexed
match after refreshing the index. The merged
[read setup](RESIDENT_LOADER_READ_GATE_0291.md),
[read event](RESIDENT_LOADER_DOS_READ_029F.md), and
[seek boundary](RESIDENT_LOADER_SEEK_028F.md) supplied the reached entry
and input provenance; this run rechecked its own hashes and breakpoints.

## Hash-locked span and live path

| Input or capture | Evidence |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Ignored slot-A `SAVGAMA.DAT` | 13,137 bytes; SHA-256 `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original `0AA2:02A1..02E0` and loaded `12C6:02A1..02E0` | Exact 64-byte match; 27 decoded instructions, no call; SHA-256 `d3acce3539cd3ca08b7378afe5d39747c6bfa2d4cb2b075375b7f49640991555` |
| Numeric CPU transcript | 188 bytes; SHA-256 `2ad3bc7d26531df2e675d5f1d4de5ae2a2563d4eee8cc88c02bf4e766ddf7b7a` |
| Native 640 KiB RAM at both breakpoints | Identical; SHA-256 `a4d69ce70aacf545a56e82742baf9b547b099643ca138f34c0ae198d8ce9d97f` |

The native debugger was armed at the saved-slot prompt, then selected
slot A. It stopped at both loaded addresses in one run. The decisive
observations are:

| Field | At `+02A1` | At `+02E1` |
| --- | --- | --- |
| `AX,CX,DX` | `0463,0463,0000` | `0000,0000,0000` |
| `DS,ES` | `2CCD,08D0` | `2D14,08D0` |
| `FLAGS` | `7246` (CF clear) | `7246` |
| `CS:IP` | `12C6:02A1` | `12C6:02E1` |

All other captured CPU fields are unchanged, and the 640 KiB RAM images
are identical. The saved `ES:[000A]` second-read count is zero;
`ES:[0010]=2CCD`. Thus `+02A1 JB` falls through on clear CF,
`+02A3 CMP AX,CX` compares equal and `+02A5 JB` falls through. The
round-up arithmetic yields `(0x0463+0x000F)>>4=0x0047`, so
`+02AE ADD AX,ES:[0010]` produces `DS=2D14`. After `+02B5 XOR DX,DX`,
`+02B7 MOV CX,ES:[000A]` sets zero; `+02BC JCXZ` takes `+02DA`, where
`XOR AX,AX` precedes the jump to `+02E1`. This is the only branch route
claimed as directly observed for this saved input.

## C boundary and remaining work

`pr_resident_loader_read_branch_02a1` models the initial CF gate, the
unsigned short-read comparison, paragraph rounding and segment update,
the second-count gate, and the zero-count completion. If `ES:[000A]`
is nonzero, it stops at `+02C0` with `AH=3Fh`, leaving that DOS service
unexecuted. If the first read failed, it stops at `+02E1` with the
original AX/flags; if the count was short, it sets AX to `0x0064` and
stops there. Synthetic tests cover these branches, real-mode memory
wrap, and invalid arguments. The short/error and nonzero second-count
cases are instruction-backed models, not native saved-route observations.
The `+02C0` service, relocation loop, `+02E1 POP DS`, and `+02E2 RETF 2`
remain separate dependencies.

The local audit compiles the C stage with strict warnings, reproduces
all 14 captured CPU fields, and compares every byte of the captured
640 KiB RAM image. The model reads a 1 MiB real-mode RAM space; the
upper 384 KiB was not captured by DOSBox.

Reproduce locally with ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_read_branch_02a1_audit.py
cmake -S . -B /tmp/poolrad-loader-read-branch-02a1-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-read-branch-02a1-build
ctest --test-dir /tmp/poolrad-loader-read-branch-02a1-build --output-on-failure
```

Original media, numeric debugger transcript, screen capture, and RAM
images remain under ignored `resources/`. The public audit prints only
addresses, hashes, and counts.
