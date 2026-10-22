# RES_0464 00CF and 00FF: video-mode set and restore over Dos.Intr (raw CPU)

`pr_r0464_set_mode_00cf_cpu` and `pr_r0464_restore_mode_00ff_cpu` are raw-CPU, full-state models of two START.EXE resident
routines. Both compose the covered `Dos.Intr` at `0A32:000B` (`dos_intr_000b_cpu`, #1259).

They sit on the RES `00BA:16EC` path. 16EC is blocked by `RES_03F1_0000`, whose overlay call binds `OVL_040C_015D`, and
`RES_0464_00FF` is one of that routine's unready callees (see the `RES_03F1_0000` hold in
[STRICT_CANDIDATE_QUEUE.md](STRICT_CANDIDATE_QUEUE.md)). This is readiness work; identity and credit are the verifying
lane's calls.

## Exact original

| Routine | Span | Size | SHA-256 |
| --- | --- | --- | --- |
| `00CF` (set) | `0464:[00CF,00FF)` | 48 bytes, 19 instructions, `RETF 2` | `1291e63d47f0d18e3c3708159546250c155647778751c8e8a2123f8bcc44afb3` |
| `00FF` (restore) | `0464:[00FF,010C)` | 13 bytes, 8 instructions, `RETF` | `cec981142e63bb53b622852e0f77455255fae44bc55ea80b0e47105e05e18ef6` |

Both come from the loader-expanded START.EXE (image SHA-256 `a42a57e6…`). The `LCALL 0A32:000B` segment word at
`0464:00F1` is a loader relocation, and the models read it live.

## Behaviour

**`00CF(mode)`**, with the mode word at `[SP+4]`:

1. `PUSH BP; MOV BP,SP; SUB SP,2`.
2. `[4A1C] := [4A1D]` saves the previous mode.
3. `[4A03] := 0` and `[4A02] := mode` set the Regs record's AX to `00mm`, the BIOS set-mode call.
4. `PUSH 10h; PUSH DS; PUSH 4A02h; LCALL 0A32:000B` calls `Dos.Intr(10h, Regs)`.
5. `[4A1D] := mode` records the new mode. The mode is reread from `[BP+6]`.
6. `MOV SP,BP; POP BP; RETF 2`.

**`00FF`**, with no argument: `PUSH BP; MOV BP,SP; MOV AL,[4A1C]; PUSH AX; PUSH CS; CALL 00CF; POP BP; RETF`. It sets
the saved mode again, and `00CF` saves the mode it replaces.

**On return.** The flags are those `Intr` returns with, and AX/DI are as `Intr` leaves them. DS is restored by `Intr`.
The pushed AX keeps the caller's AH.

**Guards.** `00CF` needs its entry SP in `[0022h,FFFAh]`. It pushes 14 bytes before `Intr`, whose own guard needs
`[0014h,FFF6h]`, and its `RETF 2` must not wrap. `00FF` needs `[002Ah,FFFCh]`.

**Statuses.** `OK`, `ARGUMENT` (nothing changed), `INTR` (`Intr` did not return; its status is reported, `SERVICE`
meaning the `INT 10h` handler refused) and `TRANSFER`.

**Declared environment.** The `INT 10h` handler is a `pr_dos_intr_service`. The audits use the same real-byte stub as
`RES_0899_0000`'s audit: it stores AL in the BIOS mode byte `0040:0049`, then `IRET`s.

## Validation

**Focused C test.** `test_resident_0464_video_mode_cpu.c` runs under CTest with ASan/UBSan, and the sources compile with
`-Wconversion -Wsign-conversion -Wshadow -Werror`. It passes **12** hand-predicted checks:

- `00CF`'s mode bookkeeping, Regs record, BIOS byte, return and pushed words;
- `00FF`'s restore through `00CF`;
- a refused `INT 10h` (`INTR` with `Intr`'s `SERVICE`);
- the guards.

**Unicorn differential.** `tools/overlay/res0464_video_mode_cpu_audit.py --unicorn` loads the whole relocated loader image,
so every byte of the closure runs, including all 69 of `Intr`'s instruction starts. It compares all 14 CPU words and the
whole 1 MiB with **no mask**.

| Routine | Cases | Body starts |
| --- | ---: | --- |
| `00CF` | 59 | 19/19 |
| `00FF` | 59 | 8/8 |

The cases cover every mode class, flag seeds, the SP guard edges, an odd SP, SS equal to DS, SS across A20, a stub in high
memory, random registers and Regs records, and random fill bytes. Each case also checks the mode bookkeeping and the BIOS
byte against expectations derived from the bytes themselves.

**DOSBox-X.** `--native` reads the relocated image from `IMAGE.BIN` and points `IVT[10h]` at the stub. It far-calls each
routine and returns to a capture label. **16** cases (8 per routine) compare all 14 words except the capture CS:IP
(checked separately), the stack window, `DS:4A00..4A1F` and `0040:0049`, with no mask. The result SHA-256 is
`df86c795bf0f1f174260399d535d6623865b0d8bb1a1213bce81918e16c9786f`.

```sh
ctest --test-dir <build> -R poolrad_resident_0464_video_mode_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/res0464_video_mode_cpu_audit.py --unicorn --native --output resources/0464-video/report.json
```

`CHECKER_ROWS = ('RES_0464_00CF', 'RES_0464_00FF')`, and `--compile-only` builds the checker with no media. No classifier,
queue or count ledger is changed.
