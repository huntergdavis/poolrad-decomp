# SYSTEM 0919 and 084A: Write(string) and Writeln to a Text file (raw CPU)

`pr_system_write_str_0919` and `pr_system_writeln_084a` are raw-CPU, full-state models of the Turbo Pascal SYSTEM text
output routines in START.EXE's SYSTEM segment (`0AF8`). Both run three near helpers inside their models:

- `07B8`, the output-mode check;
- `07DC`, the put-character routine;
- `087B`, the flush.

They are on the RES `00BA:16EC` path. `OVL_040C_015D` (the callee of the held `RES_03F1_0000`; see
[RES_00BA_16EC_PORT_HARNESS.md](RES_00BA_16EC_PORT_HARNESS.md)) calls `Write(string)` and `Writeln` on its error branch.
Elsewhere, `Write` has 13 call sites and `Writeln` has 12. This is readiness work; identity and credit are the verifying
lane's calls.

## Exact original

| Routine | Span | Size | SHA-256 |
| --- | --- | --- | --- |
| `0919` Write(string) | `0AF8:[0919,0957)` | 62 bytes, 32 instructions, `RETF 6` | `f359cb6a90d2ff7b8537977c7275b17dc18424ec318c1bf4b5a5b63d48bfea36` |
| `084A` Writeln | `0AF8:[084A,086E)` | 36 bytes, 17 instructions, `RETF 4` | `2dc2aedff9017bf614e03ad0f721385a496189d1fed39c3264f792c591151a79` |
| `07B8` mode check | `0AF8:[07B8,07DC)` | 36 bytes, 10 instructions | `b917c13e546b29278c76a82f2c43356b442235f7ba6c5a95b56997734350ade6` |
| `07DC` put character | `0AF8:[07DC,081C)` | 64 bytes, 31 instructions | `40fcc0bb4628b90895a2f2dbec804a0ca025f1d625bcdb011fdc4259ecb54ad8` |
| `087B` flush | `0AF8:[087B,0898)` | 29 bytes, 12 instructions | `752bedb6a93c16358992f1ff56a0e1aab4e1021cca52433ff5a1c974d4bbb118` |

All five come from the loader-expanded START.EXE (image SHA-256 `a42a57e6…`). The put-character routine loads DS from
the word at `CS:07EF`, which is a loader relocation, so the model reads it live.

## Behaviour

**Mode check (`07B8`).** If InOutRes `DS:[4426]` is 0 and the TextRec Mode (`+2`) is not `D7B2h` (fmOutput), it sets
InOutRes to `69h`. It then loads three TextRec fields:

- BX = BufPos (`+8`);
- DX = BufSize (`+4`);
- ES:DI = BufPtr (`+0Ch`).

The callers proceed only when InOutRes is 0.

**Put character (`07DC`).** It stores AL at `ES:[BX+DI]`, then `INC BX`. When BX reaches BufSize it:

1. saves CX, SI and DS;
2. reloads DS from `CS:07EF`;
3. stores BufPos;
4. far-calls InOutFunc (`+14h`) with the TextRec pointer;
5. moves a nonzero result AX into InOutRes;
6. reloads AX = BufEnd, BX, DX and ES:DI.

**Flush (`087B`).** It far-calls FlushFunc (`+18h`) only when its segment word (`+1Ah`) is nonzero and InOutRes is 0. A
nonzero AX becomes InOutRes.

**`Write(f, s:width)`.** The stack holds `[SP+4]` width, `[SP+6]` the string pointer and `[SP+0Ah]` the TextRec pointer.
It runs the mode check, then pads with `width - length` spaces when that is positive (`LOOP`). It writes the string's
bytes (`CLD`, `LODSB`, `LOOP`) and stores BufPos back. It ends with `RETF 6`, which leaves the TextRec pointer for the
caller's next call.

**`Writeln(f)`.** It runs the mode check, puts CR and LF, stores BufPos and flushes, then `RETF 4`.

**On return.** Every near-call return word (`0855`, `085C`, `0861`, `086A`, `0925`, `093A`, `094A`) is written to the stack
as on the CPU. Flags follow every instruction:

- XOR AH,AH and OR AX,AX clear AF;
- INC keeps CF;
- LOOP and the moves leave flags unchanged;
- CLD clears DF on Write's string path only.

**Statuses.** `RETURNED`, `ARGUMENT` (nothing changed) and `SERVICE`. `SERVICE` means a driver refused or did not return
to its continuation. The CPU is left at the driver entry, with the far return frame on the stack.

**Declared environment.** The driver functions are a `pr_system_text_driver`. The service receives RAM and the CPU at the
driver's entry (CS:IP from the TextRec). It must return at the SYSTEM CS, at IP `07FF` (InOutFunc) or `0890`
(FlushFunc), with the same SS and SP + 8 (`RETF 4`). The audits install a real-byte stub at InOutFunc and at FlushFunc:
`PUSH BP; MOV BP,SP; LES DI,[BP+6]; MOV ES:[DI+8],0; MOV AX,imm; POP BP; RETF 4`. Unicorn executes it, and the C side
mirrors it.

## Validation

**Focused C test.** `test_system_text_write_cpu.c` runs under CTest with ASan/UBSan, and the sources compile with
`-Wconversion -Wsign-conversion -Wshadow -Werror`. It passes **17** hand-predicted checks:

- right-aligned padding;
- a buffer that fills mid-string and is emptied by InOutFunc;
- Writeln's CR LF, flush and InOutRes from FlushFunc;
- an input-mode file (InOutRes `69h`);
- a pending InOutRes;
- a refused driver (`SERVICE` at the FlushFunc entry);
- the argument checks.

**Unicorn differential.** `tools/overlay/system_text_write_cpu_audit.py --unicorn` loads the whole relocated loader image
and runs every byte of the closure. It compares all 14 CPU words and the whole 1 MiB with **no mask**.

| Routine | Cases | Stops | Driver calls |
| --- | ---: | --- | ---: |
| `0919` | 135 | 134 `RETURNED`, 1 `SERVICE` | 7,918 |
| `084A` | 107 | 105 `RETURNED`, 2 `SERVICE` | 79 |

Together the cases reach all 102 instruction starts and all 26 branch outcomes. They cover:

- buffer sizes 1 to 64 at six fill positions, with both driver results;
- FlushFunc present and absent;
- four Mode words and a pending InOutRes;
- widths 0, 1, 3, 7, 25, `8000h` and `FFFFh`, with strings of 0 to 20 bytes;
- the seven flag seeds;
- SS across A20, a low SP and a buffer that wraps its offset;
- a stub in high memory;
- refused drivers;
- random fixtures.

**DOSBox-X.** `--native` reads the relocated image from `IMAGE.BIN`, loads each case's text segment (the TextRec,
buffer, string and both stubs) and far-calls the routine. The routine returns to a capture label. **25** cases compare
all 14 CPU words, a 1 KiB stack window, the 64 KiB text segment and `DS:4420..442F`, with no mask. The result SHA-256
is `d729c78a8f546ea00e7e30b9269d26d11d01d50c7d983fb1cecf5395b2fa04e2`.

```sh
ctest --test-dir <build> -R poolrad_system_text_write_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/system_text_write_cpu_audit.py --unicorn --native --output resources/system-text/report.json
```

`CHECKER_ROWS = ('RES_0AF8_0919', 'RES_0AF8_084A')`, and `--compile-only` builds the checker with no media. No classifier,
queue or count ledger is changed.
