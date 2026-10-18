# Resident 0709:00A0: raw-CPU EGA page select

`pr_r0709_page_select_00a0_cpu` is a raw-CPU, full-state model of the
START.EXE resident routine `0709:00A0..00B6`. It is a pure, call-free leaf
of the RES `00BA:16EC` closure, called by the rect fill `0709:00B7`, and
belongs to step 2 of [the port-harness plan](RES_00BA_16EC_PORT_HARNESS.md#plan).
The bounded `resident_0709_rect_fill_00b7.h` covers its effect only inline,
so this is its first standalone model.

## Exact original

| Item | Value |
| --- | --- |
| Span | `0709:00A0..00B6` of the expanded START.EXE |
| Size | 23 bytes, 10 instructions, `RETF 2` |
| SHA-256 | `6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d` |

## Behaviour

The routine runs `PUSH BP; MOV BP,SP; MOV AL,[BP+6]; XOR AH,AH; MOV CX,9;
SHL AX,CL; ADD AX,0A000h; MOV DS:[4A18h],AX; POP BP; RETF 2`.

- DS:[4A18h] becomes `A000h + 200h × page`, using the page byte at
  `[BP+6]`. This selects an 8 KiB EGA page segment.
- The result is 16 bits, so page `80h` wraps to `A000h` and page `7Fh`
  carries to `9E00h`.
- After a return AX is that word and CX is 9. The flags are those of
  `ADD`, which overwrites `SHL`'s undefined flags.

The store comes before the `POP` and the `RETF`, which read RAM physically
contiguous afterwards. So a store that aliases the frame changes them as on
the CPU.

## Validation

**Focused C test.** `test_resident_0709_page_select_00a0_cpu.c` runs under
CTest with ASan/UBSan, and the source compiles strictly. It passes **10**
hand-predicted checks:

- pages 1, `7Fh` and `80h`, with the exact `ADD` flags;
- a store onto the return IP;
- the guards.

**Unicorn differential.** `tools/overlay/res0709_page_select_00a0_cpu_audit.py`
runs **126** original-byte cases. It compares the whole 1 MiB RAM and all
14 CPU words with **no mask**, over 10/10 instruction starts. The cases
include:

- random page words, segments, SPs and flags;
- stores that alias the return IP, the pushed BP and the argument;
- SS across A20, DF set, and a random background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **32**
cases with no mask. Each compares all 14 words, the stack window and the
whole data segment. The result SHA-256 is
`7d61d6cf5eeab6a793b55d919baaea7a2d32356d65193005feb28a4a1b93c053`.

```sh
ctest --test-dir <build> -R poolrad_resident_0709_page_select_00a0_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 1500s \
  python3 tools/overlay/res0709_page_select_00a0_cpu_audit.py --unicorn --native --output resources/res0709-page/report.json
```

No classifier, queue or count ledger is changed. Crediting `RES_0709_00A0`
is the verifying lane's call.
