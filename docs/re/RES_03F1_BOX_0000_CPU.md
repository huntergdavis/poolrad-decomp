# RES_03F1_0000: tile box through the overlay stub (raw CPU, composed)

`pr_r03f1_box_0000_cpu` is a raw-CPU, full-state model of START.EXE resident `03F1:0000`, the routine that draws a box of
tiles. It is the held one-hop callee of the RES `00BA:16EC` keystone (see
[RES_00BA_16EC_PORT_HARNESS.md](RES_00BA_16EC_PORT_HARNESS.md)). All eight of its calls go through overlay stub
`0428:002A`, which is unit `40Ch` entry 2, `+015D`, and they compose `OVL_040C_015D`'s model.

This branch stacks on #1415 (and therefore on #1413 and #1414). This is readiness work; identity and credit are the
verifying lane's calls.

## Exact original

| Item | Value |
| --- | --- |
| Span | `03F1:[0000,0137)`, 311 bytes, 145 instructions, `RETF 0Ah` |
| SHA-256 | `fae6cb41423bcd48237e75a08f5eda70642254eac8449373291d57f34120a8a4` |
| Stub entry | `0428:002A` = `INT 3Fh` with `015Dh` (unit `40Ch` entry 2) |

The image comes from the loader-expanded START.EXE (SHA-256 `a42a57e6…`). The eight stub-call segment words are loader
relocations, and the model reads them live. `emit_res03f1_box_0000.py` generates the C from the bytes, and `--check`
regenerates it byte-identically.

## Behaviour

The arguments are `[BP+6]` style, `[BP+8]` bottom, `[BP+0Ah]` right, `[BP+0Ch]` top and `[BP+0Eh]` left, all bytes.

1. The coordinates are first adjusted in place: left and top `DEC`, right and bottom `INC`.
2. `[BP-3]` := style * 3 + `114h`. The corner tile is `+0`, the vertical edge `+1` and the horizontal edge `+2`.
3. The four corners are placed.
4. For each column from left + 1 to right − 1 (signed byte compare, held in `[BP-1]` and `[BP-4]`), the top and bottom
   horizontal edges are placed.
5. For each row from top + 1 to bottom − 1, the left and right vertical edges are placed.

**Tile calls.** Every tile call pushes the frame (column, row, number, 0, 0). AH of each byte word is whatever the
preceding instruction left. The number's high byte (`01h`) is in the flag word, so `015D`'s byte test sees 0 and always
takes the Move-and-`1951` path.

**Error-branch styles.** Style 3 makes the horizontal edge `11Fh`, which is `015D`'s error branch. Larger styles do the
same, so they end in `EXIT`.

**The stub.** It is executed as on the CPU. A resident unit's entry holds `JMP FAR` (`EAh`) to the unit's
segment:`015D`, which is what the overlay manager writes. The model follows it into `015D`, whose `RETF 0Ah` returns
straight to this body. An unloaded entry (`INT 3Fh`) stops with `OVERLAY` before the overlay manager runs. The CPU is
then at the stub with the call frame pushed.

**Statuses.** `OK`, `ARGUMENT`, `CALLEE`, `TRANSFER`, `LIMIT` (this body's own backward jumps), `OVERLAY`, `CONTRACT` and
`EXIT`. The last two propagate from `015D`. The diagnostic gives the call site, `015D`'s status and its diagnostic.
The only undefined flag, `SHL`'s AF at `0019`, is overwritten on every path.

## Validation

**Focused C test.** `test_resident_03f1_box_0000_cpu.c` runs under CTest with ASan/UBSan. It passes **11**
hand-predicted checks:

- a 12-call box: argument adjustment, locals, the last tile in the object, the return;
- `OVERLAY` at an unloaded stub;
- `LIMIT` after the first column;
- `015D`'s `CONTRACT` propagated;
- the argument checks.

**Unicorn differential.** `tools/overlay/res03f1_box_0000_cpu_audit.py --unicorn` runs the original closure. It reuses
`ovl040c_015d_cpu_audit.py`'s image, unit, fixture and declared environment unchanged, and adds the stub entry in its
loaded state. It compares all 14 CPU words, the whole 1 MiB and the ordered environment event log, with **no mask**.

| Measure | Result |
| --- | --- |
| Cases | **56** |
| Stops | 48 `RETURN`, 6 `EXIT`, 1 Ctrl-Break, 1 `OVERLAY` |
| Body starts | 145/145 |
| Branch outcomes | 8/8 |
| Composed `015D` calls | 1,124 |

The cases cover:

- styles 0–3 and `FFh`;
- full-screen, large, single-cell, adjacent and inverted boxes;
- signed wrap at `7Fh`/`80h`;
- garbage high bytes;
- 1951 modes 0–3;
- a null band;
- a relocated unit segment;
- the flag seeds and random boxes.

**DOSBox-X.** `--native` uses `ovl040c_015d_cpu_native.py`'s oracle. It adds the stub segment's first 40h bytes,
which hold the loaded entry, to the compared regions. **16** cases (all `RETURN`, including the 124-call large box)
compare CPU14 and every region with no mask. The result SHA-256 is
`0b57f9c894a636c9816395c41a1386ca2e34c51ab7cfbddbdbdc5745f8f8b2aa`.

Three groups are Unicorn-only:

- **Error styles and the unloaded stub:** they leave through QUIT or the overlay manager.
- **Boxes with an edge of 0 or at the signed wrap:** a −1 edge indexes `1951`'s row table at −1, so its Move goes to an
  arbitrary segment.
- **The random cases:** a Unicorn write trace found eight of them storing outside the oracle's loaded regions, into the
  interrupt table or high conventional memory. Under DOSBox that hangs or reboots the machine, while under Unicorn it is
  plain RAM, compared in full.

**Shared harness change.** This branch generalises the 015D audit's Unicorn runner: a body-segment parameter, extra
known segments, and a stop before an `INT 3Fh` opcode. It also lets the native oracle take selectable regions and
cases. The 015D audit was rerun after the change and reproduces exactly: 105 Unicorn cases and the same native result
SHA, `23cb9ec4…`.

```sh
ctest --test-dir <build> -R poolrad_resident_03f1_box_0000_cpu --output-on-failure
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/res03f1_box_0000_cpu_audit.py --unicorn --native --output resources/res03f1-box/report.json
python3 tools/overlay/emit_res03f1_box_0000.py --check
```

`CHECKER_ROWS = ('RES_03F1_0000',)`, and `--compile-only` builds the checker with no media. No classifier, queue or count
ledger is changed.
