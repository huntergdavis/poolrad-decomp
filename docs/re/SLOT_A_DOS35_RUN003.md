# Run003 DOS vector-get hypothesis

The retained same-invocation run003 `1386`/`1388` pair shows `AX=3508`
returning the INT 08 vector in `ES:BX`, with transcribed flags unchanged.
Its conventional RAM delta is 25 bytes. Six bytes correspond to the INT
return frame; the 18 bytes immediately below it equal the input AX, BX, CX,
DX, SI, DI, BP, DS and ES words in that order. The remaining byte is in a
four-byte pointer at physical `0816E`: its resulting value is `1D16:3D96`,
the start of that register-save region. The candidate PSP at segment `0814`
contains a `CD 20` signature and has its saved-stack field at offset `2E`.
These are observations and a candidate identity, not independent proof of
the active PSP or write causality.

A local DOSBox-X source tree at commit `b6abbd5980a885f5f310a4088c59a8688d1b116c`
(`src/dos/dos.cpp`, `DOS_21Handler`, and `include/dos_inc.h`, `DOS_PSP`)
contains a corresponding PSP stack-pointer update, register save, and AH=35
IVT lookup. Its association with the binary used for run003 has not been
established. It supports a hypothesis, not an independently verified build
identity or a general DOS contract.

The hash-locked test below models only that exact transcribed CPU state with
an explicitly supplied **hypothetical** PSP `0814`. It models the INT frame,
register save and PSP pointer, reads the vector from the captured IVT, and
returns the pre-INT flags. It compares all 655,360 conventional RAM bytes
and CPU14 with the same-run `1388` capture; both match. An unsupported PSP is
refused without state mutation. The model lives in the test, is not a DOS
provider, and does not feed run003 output into run002.

```sh
bash tests/replay/check_slot_a_dos35_run003.sh PRIVATE_RUN003_DIR
```

CPU records are manual transcriptions and the segment checks are same-source
integrity evidence. The active PSP, exact emulator build, service causality,
interrupt timing, upper memory and devices remain unverified. In particular,
this observation does not justify replacing the run002 service refusal or
claiming `3492` or complete Up parity.
