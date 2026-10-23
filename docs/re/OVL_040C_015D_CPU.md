# OVL_040C_015D: tile placement by band (raw CPU, composed)

`pr_ovl040c_015d_cpu` is a raw-CPU, full-state model of GAME.OVR unit `40Ch`'s public entry `+015D`. Every one of its
eight far calls runs through a merged (or stacked) C model.

`RES_03F1_0000`'s overlay call binds this entry, and `RES_03F1_0000` holds the RES `00BA:16EC` keystone (see
[RES_00BA_16EC_PORT_HARNESS.md](RES_00BA_16EC_PORT_HARNESS.md)). With this model, every callee on that path is modelled.
This is readiness work; identity and credit are the verifying lane's calls.

The branch stacks on #1413 (`0464:08D4`) and #1414 (SYSTEM `0919`/`084A`); it will be retargeted once they merge. It
also adds `CBW` to the shared emitter (`emit_raw_cpu_c.py`), and the other three emitted files regenerate byte-identically.

## Exact original

| Item | Value |
| --- | --- |
| Span | unit `40Ch` `+[015D,02AD)`, 336 bytes, 127 instructions, `RETF 0Ah` |
| SHA-256 | `2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00` |
| Unit | 690 code bytes, entries `02AD`, `003B`, `015D`, 25 segment fixups |

The unit is found by `overlay_inventory.find_units`. The eight far-call segment words (`CS:01C8`, `01DA`, `01DF`,
`01E4`, `01EC`, `024C`, `028E`, `02A5`) are all fixups, and the model reads them live. `emit_ovl040c_015d.py`
generates the C from the bytes, one case per instruction; `--check` regenerates it byte-identically.

## Behaviour

**Band selection.** The tile number `[BP+0Ah]` selects a band, stored in `[BP-1]`:

| Number | Band |
| --- | --- |
| `1..2Dh` | 0 |
| `2Eh..73h` | 1 |
| `74h..B9h` | 2 |
| `BAh..FFh` | 3 |
| `100h..11Eh` | 4 |

**Error branch.** A number of 0, or `11Fh..7FFFh`, takes the error branch:

1. `0464:00FF` restores the video mode.
2. `Write(Output, CS:013A, 0)` writes the message in the unit's code segment. The message is game text and stays in
   `resources/`.
3. `Writeln(Output)`.
4. `0899:0059` ReadKey, with the key stored in `[BP-2]`.
5. `0899:0000` QUIT, which never returns.

**Stale band.** `8000h..FFFFh` passes no test and falls through with `[BP-1]` as it was on the stack. The model carries
that stale byte faithfully.

**Placement.** If the band's far pointer `DS:5E3A[band]` is null, the routine returns. Otherwise `[BP+0Ah] -=
DS:2722[band]`, then:

- **`[BP+8]` byte non-zero:** `0464:08D4(DS:4A20 object, tile set, flag [BP+6] in AL, item, sign-extended [BP+0Ch],
  [BP+0Eh])`. The pushed item word keeps AH from the preceding `CBW`.
- **`[BP+8]` byte zero:** `Move(tile set + 17h + item * row size (+11h), DS:5E4E object + 17h, row size)`. The `MUL`
  leaves DX = the high product. Then `0464:1951(DS:5E4E object, sign-extended [BP+0Ch], [BP+0Eh])`.

**Statuses.**

- `OK`: `RETF 0Ah` executed.
- `ARGUMENT`: nothing changed.
- `CALLEE`: a callee stopped; the diagnostic gives the site and the callee's own status.
- `TRANSFER`.
- `CONTRACT`: an EGA is attached and either an error-branch callee or a Move meets `A0000..AFFFF`. The flat-RAM
  callees would bypass the EGA, so the model stops at the callee's entry.
- `EXIT`: QUIT reached Halt, or ReadKey exited through its own QUIT (key 03 with `STING`) or the Crt Ctrl-Break path.

**Undefined flags.** `SHL` leaves AF undefined at `0235`/`0237`, and it is live into `08D4`. `08D4`'s prologue `SUB`
overwrites it, so it is never observed. Every other undefined flag is overwritten on every path.

**Declared environment.** The video handler is the one `00FF` uses. The Text driver serves `Write`/`Writeln`. ReadKey's
environment (INT 16h, the 08AE driver, INT 21h `25h`/`35h`/`2Ch`, ports, QUIT's INT 10h handler) comes from the 0899
models. `max_loopbacks` is passed through to `08D4` and `1951`.

## Validation

**Focused C test.** `test_overlay_u40c_tile_015d_cpu.c` runs under CTest with ASan/UBSan, and the sources compile with
`-Wconversion -Wsign-conversion -Wshadow -Werror`. It passes **15** hand-predicted checks:

- band 2's rebase and the Move row copy;
- a null band;
- the stale `[BP-1]` path with a wrapped row offset;
- `CONTRACT` at `00FF` with an EGA attached;
- `CONTRACT` at a Move into `A000`;
- the argument checks.

**Unicorn differential.** `tools/overlay/ovl040c_015d_cpu_audit.py --unicorn` loads the whole relocated loader image and
the unit's code with its fixups, then runs the original closure. That covers the body, `00FF` with `Dos.Intr`, SYSTEM
`Write`/`Writeln` with real-byte driver stubs, ReadKey with Crt/08AE/SYSTEM members, QUIT, `08D4`, Move and `1951`. It
compares all 14 CPU words, the whole 1 MiB and the ordered environment event log (INT 16h/21h, ports including the
`3C4h`/`3C5h` sequencer OUTs with no EGA attached), with **no mask**.

| Measure | Result |
| --- | --- |
| Cases | **105** |
| Stops | 83 `RETURN`, 21 `EXIT`, 1 Ctrl-Break (reported as `EXIT` through ReadKey) |
| Body starts | 127/127 |
| Branch outcomes | 29/30, with one declared unreachable |
| Call sites reached | `01C5`/`01D7`/`01DC`/`01E1` 22 each, `01E9` 20, `0249` 39, `028B` 32, `02A2` 32 |

The unreachable outcome is `01BE`'s `JB` taken: a non-zero number below `11Fh` is always inside a band already taken.

**DOSBox-X.** `--native` reads the relocated image at `1000h`, loads each case's regions and sets all fourteen
registers. It far-jumps to `015D` and returns to a capture label. **63** cases (all `RETURN`) compare CPU14 and the
regions `1C7C0+10000`, `30000+14000`, `44000+18000`, `7E000+2000` and `80000+C000`, with no mask. The result SHA-256 is
`23cb9ec487545901d842950f218c1aa2dce151d406a9d9aeaeab27e527b0fff1`.

Two parts are Unicorn-only:

- The error branch: QUIT ends the process.
- Video modes other than 0 and 2: their sequencer OUTs and `A000` stores would drive the real adapter.

The 0899 and SYSTEM text audits already corroborate those callees natively.

```sh
ctest --test-dir <build> -R poolrad_overlay_u40c_tile_015d_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/ovl040c_015d_cpu_audit.py --unicorn --native --output resources/ovl040c-015d/report.json
python3 tools/overlay/emit_ovl040c_015d.py --check
```

`CHECKER_ROWS = ('OVL_040C_015D',)`, and `--compile-only` builds the checker with no media. No classifier, queue or count
ledger is changed.
