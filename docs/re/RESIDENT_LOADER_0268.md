# Resident loader far-call target prefix `+0268`

The saved slot-A cold loader reaches `12C6:03AA CALL FAR [DS:8504]`.
The live pointer is `12C6:0268`, and a fresh native stop confirms that
callee entry. This C stage translates the 39-byte target prefix
`12C6:0268..028E` and stops **before** `+028F INT 21h`. It prepares a
DOS seek request; the DOS service, later reads, relocation loop, and
far return are outside this stage. No enclosing-loader or full-function
count is claimed.

Before implementation, `deja "Poolrad saved loader 12C6 03AA far call
0268 target native CPU RAM"` timed out during local index refresh
(exit 124). The merged [+0366 continuation](RESIDENT_LOADER_0366.md)
supplied the reached caller and saved pointer lead. This work
independently pinned original/loaded bytes and captured the far call,
target entry, and first DOS-service boundary in one fresh run.

## Instruction and caller boundary

| Evidence | Result |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Expanded original `0AA2:0268..028E` | 39 bytes, 11 complete 16-bit instructions; SHA-256 `1aa7e6650b22df729a0529f2723bc860c14f54ef5e79b24d79d7feecd4ae9e0b` |
| Loaded `12C6:0268..028E` | Exact match to original at fresh callee entry |
| Original/loaded `+03AA CALL FAR [8504]` | Four bytes, SHA-256 `685bf9d927e06f3383fb8feb01a79bdf24d6432cae1ee834f78ed811658a8a99`; saved pointer `12C6:0268` |

The far call changes `SS:SP` from `1D16:3776` to `1D16:3772` and
places return offset `03AE` and segment `12C6` at `SS:3772..3775`.
The prior `SS:3776` word is `08D0`, used by target `+026A MOV ES,
SS:[BX+4]` after `+0268 MOV BX,SP`. `+026E PUSH DS` saves `14A0`
at `SS:3770`.

The target reads `DS:[43FC]` into BX, `DS:[4422] + 0010` into DI,
and the dword formed by `ES:[0006]:ES:[0004]` into `CX:DX`.
`+0284 ADD DX,[43E8]` carries into `+0288 ADC CX,[43EA]`.
`+028C MOV AX,4200` prepares the DOS absolute seek call, then the
model stops at `+028F`. The target's later DOS and loader effects
are not modeled.

## Direct native gate and C parity

A fresh ignored run `slot-a-loader-0268-007` used pinned slot-A
`SAVGAMA.DAT`, SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
It stopped at saved caller `2A8E:19E3`, call site `12C6:03AA`,
callee entry `12C6:0268`, and pre-DOS stop `12C6:028F`. The
318-byte numeric CPU transcript has SHA-256
`5031f354a224a11fe3556b7b869f2afb8b7d7ea06c190410537cff383cf34bcc`.

| CPU field | Callee entry | Before INT 21h |
| --- | --- | --- |
| `CS:IP` | `12C6:0268` | `12C6:028F` |
| `SS:SP` | `1D16:3772` | `1D16:3770` |
| `AX,BX,CX,DX,DI,FLAGS` | `2CCD,0027,0004,FF45,0000,7283` | `4200,0005,0002,AA87,0824,7202` |
| `DS,ES,BP,SI` | `14A0,08D0,3794,44BA` | unchanged |

`FS=GS=0000` at both stops. Saved `DS:[43FC]=0005`,
`DS:[4422]=0814`, `ES:[0006]:ES:[0004]=0002:AA87`, and
`DS:[43EA]:DS:[43E8]=0000:0000`. The seek request therefore uses
handle `0005` and offset `0002AA87`; DI becomes `0824`.

The 655,360-byte callee-entry and pre-DOS RAM captures have SHA-256
`c835697cf11c721678c3568524bbb3f2c24b5ecaf6c1475d18f21a52cc3d408c`
and `bf84dde80f3e5b29829d177968ec6298e6231075b3636c8ed86d32cbe5a79576`.
Exactly two captured bytes change: `SS:3770..3771` from `0008` to
`14A0`. Compiled C reproduces every captured RAM byte and all 14
modeled CPU fields at the pre-DOS stop. The upper 384 KiB of the
model's 1 MiB RAM space was not captured; no native parity claim
applies there. Synthetic tests cover saved seek setup, low-word carry,
high-word overflow and carry, DI and stack-offset wrap, and invalid
arguments. Those alternate values are instruction-derived.

Reproduce locally using ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_0268_audit.py
cmake -S . -B /tmp/poolrad-loader-0268-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-0268-build
ctest --test-dir /tmp/poolrad-loader-0268-build --output-on-failure
```

The audit prints only hashes, addresses, and counts. Original bytes,
RAM captures, saves, and media remain under ignored `resources/`.
