# Saved resident loader read setup `+0291..+029E`

After the [saved DOS seek](RESIDENT_LOADER_SEEK_028F.md) returns, the
restored slot-A route reaches loaded `12C6:0291`. Four instructions
prepare a DOS `AH=3Fh` file read and stop immediately **before**
`12C6:029F INT 21h`. This bounded C stage does not perform the read,
model DOS side effects, or claim a complete enclosing loader/function.

Before debugging, `deja "Poolrad saved loader 12C6 0291 029F DOS read
gate DS ES CX DX AX native CPU RAM"` timed out during local index
refresh (exit 124). The merged [seek boundary](RESIDENT_LOADER_SEEK_028F.md)
supplied the reached post-service address. This work independently
hash-locked the next original/loaded span and captured both native stops.

## Instruction and native state

| Input or span | Evidence |
| --- | --- |
| Ignored `START.EXE` | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| Ignored slot-A `SAVGAMA.DAT` | 13,137 bytes; SHA-256 `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original `0AA2:0291..029E`, loaded `12C6:0291..029E` | Exact 14-byte, four-instruction match; SHA-256 `e024fb0005bc417e084a74e2dd9650d5d5c52f80d43993f80a03dbde9788695c` |
| Numeric CPU transcript | 232 bytes; SHA-256 `7b9a2e01beb538a367feccb309d40eb9c1f216ea6e17e1e7b7ac949bf75b5f90` |
| Entry and pre-read 655,360-byte RAM images | Identical; SHA-256 `9cd2d5fc44bff02bb14103265d8270c67558b55f27a291820c88866bf16d0ea8` |

Direct ignored run `slot-a-loader-read-gate-0291-009` stopped at both
addresses after selecting saved slot A:

| CPU field | Entry `+0291` | Before read `+029F` |
| --- | --- | --- |
| `CS:IP` | `12C6:0291` | `12C6:029F` |
| `AX,BX,CX,DX` | `AA87,0005,0002,0002` | `3F87,0005,0463,0000` |
| `DS,ES,FLAGS` | `14A0,08D0,7202` | `2CCD,08D0,7246` |
| `SS:SP,BP,SI,DI` | `1D16:3770,3794,44BA,0824` | unchanged |

`FS=GS=0000` at both stops. The instructions load DS from
`ES:[0010]=2CCD`, clear DX with XOR, load CX from `ES:[0008]=0463`,
and set AH to `3F` while retaining AL=`87`. The saved XOR result sets
ZF and PF, clears CF/OF/SF, and leaves observed AF clear. AF is
architecturally undefined for XOR; the C model uses the observed
DOSBox convention for this bounded stage. No captured RAM byte changed.

At `+029F`, the pending external read has handle `BX=0005`, buffer
`DS:DX=2CCD:0000`, and requested byte count `CX=0463`. The DOS read
result, guest-RAM writes, later conditional branches, and return remain
unmeasured by this stage. The C function returns `AT_DOS_READ` at that
boundary and has no implicit service callback or saved result value.

Compiled C matches all 14 modeled CPU fields and every byte of the
captured 640 KiB RAM image. The upper 384 KiB of the model's 1 MiB RAM
space was not captured. Synthetic tests cover saved input, alternate
ES table words and AL, segment-bus wrap, and invalid entry/arguments.
Only the saved values have direct native before/after evidence.

Reproduce locally with ignored inputs:

```sh
PYTHONPATH=/tmp/poolrad-capstone:tools/overlay python3 \
  tools/overlay/resident_loader_read_gate_0291_audit.py
cmake -S . -B /tmp/poolrad-loader-read-gate-0291-build -DCMAKE_C_FLAGS=-Werror
cmake --build /tmp/poolrad-loader-read-gate-0291-build
ctest --test-dir /tmp/poolrad-loader-read-gate-0291-build --output-on-failure
```

Original executable/save, screen, CPU transcript, and RAM captures
remain under ignored `resources/`. The audit prints only addresses,
hashes, and counts.
