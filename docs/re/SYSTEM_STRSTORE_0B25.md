# SYSTEM 0AF8:0B25 string store: raw-CPU full state

`pr_system_strstore_0b25` is a raw-CPU, full-state model of the Turbo
Pascal SYSTEM string store at loaded `0AF8:0B25`. The VP approved it on
2026-10-17 as a runtime model with **no strict credit**, the same
precedent as the [SYSTEM `+11C0` random model](SYSTEM_RANDOM_FULLSTATE_11C0.md).
It exists so overlay bodies that call it can compose it as a real far call.

In the census-1937 candidate queue, 83 rows call `runtime:2853` (this
routine). For four of them it is the only callee that is not strict or
modeled:

| Row | Size | Prior translation |
| --- | --- | --- |
| U3B0 `+08D7` | 76 bytes, 31 instructions | none |
| U3B0 `+0923` | 70 bytes, 29 instructions | none |
| U25 `+0F53` | 512 bytes, 181 instructions | provisional hook model on main |
| U1C `+0126` | 125 bytes, 50 instructions | inline-covered by #102/#108; not claimed |

The timeboxed `deja` recall for this routine returned nothing within 90
seconds, so nothing is reused.

## Exact original

| Artifact | Verified bound |
| --- | --- |
| START.EXE | 65,231 bytes; SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`; expanded with `resident_loader_map.unpack` |
| String store | loaded `0AF8:0B25..0B48`, expanded image `0xBAA5..0xBAC8`, file offset `0xB79B` (unique); **36 bytes, 16 instructions, one conditional branch, no calls**; `RETF 0Ah`; SHA-256 `009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118` |

```text
0B25 CLD                    0B37 CMP AL,CL
0B26 MOV BX,SP              0B39 JBE 0B3D
0B28 MOV DX,DS              0B3B MOV AL,CL
0B2A LDS SI,SS:[BX+0Ah]     0B3D STOSB
0B2E LES DI,SS:[BX+6]       0B3E MOV CL,AL
0B32 MOV CX,SS:[BX+4]       0B40 XOR CH,CH
0B36 LODSB                  0B42 REP MOVSB
                            0B44 MOV DS,DX
                            0B46 RETF 0Ah
```

## ABI and ordered effects

The routine is entered after the far CALL with `SS:[SP]` the return IP,
`[SP+2]` the return CS, `[SP+4]` the maximum length word m,
`[SP+6]/[SP+8]` the destination far pointer D and `[SP+0Ah]/[SP+0Ch]` the
source far pointer S. Pascal callers push S, then D, then m.

1. `CLD` clears DF. BX := SP and DX := DS.
2. SI:DS := S, DI:ES := D and CX := m, all read through SS.
3. `LODSB` reads the source length byte. AL := min(that byte, CL), so
   only m's low byte limits the copy. `STOSB` stores AL as the new length.
4. CX := AL. `REP MOVSB` then copies AL bytes forward, one at a time. An
   overlapping destination reads back bytes it has just written: with
   D = S + 1 the stored length byte fills the whole copy.
5. DS := DX, and `RETF 0Ah` pops the return address and the three
   arguments, so SP rises by 14.

On return AH is unchanged, BX is the entry SP, CX is 0, DX and DS hold the
entry DS, ES is D's segment, and SI and DI point one past the last bytes
read and written. SI and DI wrap at 16 bits inside their segments.

**Flags.** The final flags are `XOR CH,CH`'s: ZF and PF set, CF, OF and
SF clear, and DF clear from `CLD`. `CMP AL,CL`'s flags are overwritten
before any read and are folded. AF is architecturally undefined after
`XOR`; both oracles clear it, and the model clears it.

**Addressing.** Offsets wrap at 16 bits and physical addresses at 1 MiB.
The `LDS`/`LES` pairs and the `RETF` frame are read physically contiguous,
as Unicorn reads them, so a frame at a segment end takes its second word
from the bytes past the segment. Null RAM or CPU, or RAM that is not
exactly 1 MiB, returns `ARGUMENT` before any change.

## Validation

**Focused C test.** `test_system_strstore_fullstate_0b25.c` runs under
CTest with ASan/UBSan and compiles with `-Wconversion -Werror`. It uses
synthetic data only, compares the whole 1 MiB buffer with an expected
copy in every case, and passes 28,217 checks:

- every source length 0 to 255 against eleven caps, including caps with
  a nonzero high byte and caps just below, at and above the length;
- literal final FLAGS for two entry values;
- destinations one above, one below and equal to the source;
- SI and DI wrapping inside their segments, and S and D at `FFFF:xxxx`;
- frames at `SP = FFF4` and `FFFE` and at physical `FFFFEh`;
- the argument guards.

**Unicorn differential.** `tools/overlay/system_strstore_0b25_audit.py`
places the expanded START.EXE image at load segment 0 and runs the
original at `0AF8:0B25`. It compares the entire 1 MiB RAM and all 14 CPU
words with no mask. **368 cases** pass with 16/16 instruction starts and
2/2 conditional outcomes. They cover:

- every source length, each with a cap near it and a random high byte;
- seven fixed caps, including `0100h`, `01FFh` and `FFFFh`;
- 80 random worlds over four FLAGS values;
- nine overlap distances from -16 to +255 bytes, and D equal to S;
- SI and DI wrap;
- S, D and SS across A20, and stacks at segment ends;
- a random 1 MiB background.

**DOSBox-X oracle.** The normal-core/386 synthetic oracle passes **96
cases**, a third of them with an overlapping destination in the source
segment. Each case compares all 14 CPU words, 64 stack bytes and both
`0x300`-byte string regions, with no mask. The COM runs the original
bytes at its own paragraph. The result SHA-256 is
`04f373afa3725e35410d3d4789d6ba89775d25d2f617e61ff5e89e3c943097d0` with
COM segment `0814`. Original bytes and generated images stay under
ignored `resources/`.

```sh
ctest --test-dir <build> -R poolrad_system_strstore_fullstate_0b25 --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn PYTHONDONTWRITEBYTECODE=1 timeout 3000s \
  python3 tools/overlay/system_strstore_0b25_audit.py --unicorn --native --output resources/s0b25-parity/report.json
```

| Parity report field | Finding |
| --- | --- |
| `target_function` | SYSTEM string store, loaded `0AF8:0B25..0B48` |
| `source_candidates` | `src/recovered/system_strstore_fullstate_0b25.c` |
| `signals` | Exact hash and CFG, whole-RAM/CPU differential with full instruction and branch-outcome coverage, independent C test, native synthetic execution |
| `score` | 4/4 evidence checks passed; checklist rather than probability |
| `verdict` | `match` under the documented RAM/CPU contract |
| `review_notes` | Runtime library code: no strict or identity credit. Next are U3B0 `+08D7` and `+0923`, then U25 `+0F53`, which compose it. |

No classifier, queue or count ledger is changed.
