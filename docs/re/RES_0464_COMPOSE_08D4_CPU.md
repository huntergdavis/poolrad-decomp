# RES_0464 08D4: the tile compositor (raw CPU, composing SYSTEM Move)

`pr_r0464_compose_08d4_cpu` is a raw-CPU, full-state model of the START.EXE resident routine `0464:[08D4,0D71)`, the
tile compositor that slot-A rendering reaches. Bounded models already cover parts of it: the plane loop and the dirty
metadata ([SLOT_A_U40C_FIRST_TILE_PLANE.md](SLOT_A_U40C_FIRST_TILE_PLANE.md),
[SLOT_A_U40C_GROUP3_DIRTY_METADATA.md](SLOT_A_U40C_GROUP3_DIRTY_METADATA.md)). This is the whole routine.

It is also the largest unready callee of `OVL_040C_015D`, on the RES `00BA:16EC` → `RES_03F1_0000` path (the
`RES_03F1_0000` hold in [STRICT_CANDIDATE_QUEUE.md](STRICT_CANDIDATE_QUEUE.md)). It is readiness work; identity and
credit are the verifying lane's calls.

## Exact original

| Item | Value |
| --- | --- |
| Span | `0464:[08D4,0D71)` of the loader-expanded START.EXE |
| Size | 1,181 bytes, 429 instructions, `RETF 10h` at `0D6E` |
| Calls | `LCALL 0AF8:1685` (SYSTEM Move) at `0B30` and `0D2A`; no port I/O or interrupts |
| SHA-256 | `9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe` |

## How the source was produced

`tools/overlay/emit_res0464_compose_08d4.py` generates the header and source with the shared per-instruction emitter
`emit_raw_cpu_c.py`, as for `RES_0464_1951`:

- there is one switch case per original instruction;
- operands are reread from a `pr_bus` in source order;
- the two `Move` calls compose the accepted raw `pr_system_move_1685`.

`--check` confirms that the committed files regenerate byte for byte. The emitter's undefined-flag report lists 50
producers (`SHL`/`SHR` AF, and `MUL`'s SF/ZF/AF/PF), and **none is observed** on any path: each is overwritten first.

## Behaviour

**Frame.** The routine is entered after a far call. `[BP+6]` is the destination object D, `[BP+0Ah]` the source tile set
S, `[BP+0Eh]` a flag word, `[BP+10h]` the item index, `[BP+12h]` y and `[BP+14h]` x. `RETF 10h` pops all eight words.

**Source tile set.** `S+0` is the row count; the loop runs `S[0]−1` inclusive, so 0 means 65,536 rows. `S+2` is the
width, `S+11h` the tile size (multiplied by the item index), `S+13h/15h` the aux mask far pointer, and the tile bytes
start at `S+17h`.

**Destination.** `D+2` is the row width, and the data starts at `D+17h`.

**Video state** (`DS:4A1E`):

- 0 doubles the column and row arithmetic, and 2 quadruples it;
- 1 takes the nibble-counter path (`[BP-11h]` counts 4 down, and the aux index advances every four bytes);
- any other value returns.

**Per row.** In each mode, when flag bit 0 is set and the aux pointer is non-null, a byte loop runs. It uses the
`DS:70C7` counter up to the low byte of the scaled width, so a value of `80h` or more skips it. Each destination byte
becomes `(D AND aux) OR tile`. Otherwise one `Move` copies the row.

**Metadata.** Every row sets `DS:4A28[row] := 1`. It lowers the start extent in `4C20[row]`, recording the column in
`4AD0[row]` when it does, and raises the end extent in `4D70[row]`. These arrays hold 168 entries each, packed back to
back.

**Addressing and stops.**

- Offsets wrap at 16 bits, physical addresses at 1 MiB, and word operands are physically contiguous.
- `max_loopbacks` caps the taken backward jumps and stops with `LIMIT` before the next one.
- A `Move` that returns elsewhere stops with `TRANSFER`.
- `CONTRACT` (a `Move` touching `A000` with an EGA attached) cannot arise here: the routine writes an off-screen object
  and runs with no EGA attached.
- **Unreachable code.** Each loop opens with `XOR AX,AX; CMP AX,[BP-18h]; JBE`, which is always taken. So the four
  fall-through outcomes (`09F4→09F6`, `0AD0→0AD2`, `0BDE→0BE0`, `0CCA→0CCC`) and the four `JMP`s behind them (`09F6`,
  `0AD2`, `0BE0`, `0CCC`) can never run.

## Validation

**Focused C test.** `test_resident_0464_compose_08d4_cpu.c` runs under CTest with ASan/UBSan, and the sources compile
with `-Wconversion -Wsign-conversion -Wshadow -Werror`. It passes **10** hand-predicted checks:

- a mode-0 `Move` row: its dirty byte, the extents 2 and 3, the two copied bytes at `D+19h`, and `RETF 10h`;
- an unknown video state;
- `LIMIT` on a zero row count;
- the guards.

**Unicorn differential.** `tools/overlay/res0464_compose_08d4_cpu_audit.py --unicorn` runs the original body and `Move`
bytes. It compares all 14 CPU words and the **whole 1 MiB with no mask**.

- **Cases: 172** (163 return, 9 `LIMIT`), with 149 composed `Move` calls.
- **Coverage.** 425/429 body starts and 56/60 conditional outcomes; the missing four of each are the declared
  unreachable ones above. All of `Move`'s starts ran (441/445 closure starts).
- **Inputs.**
  - Modes 0, 1, 2 and 3–FFh.
  - Flag and aux combinations, row counts 0–4, widths up to `20h` (a negative byte count).
  - Item indexes, x and y at zero and negative.
  - Low, high and mixed extent presets.
  - Forward and backward `Move` (the source offset above or below the destination's), and overlapping objects.
  - Flag seeds, SS across A20, SS equal to DS, and random frames.

**DOSBox-X.** `--native` loads four 64 KiB segments per case from `INPUT.BIN`: DS, source, destination and aux. It
places the original body at `8800:08D4` and the original `Move` at `6000:1685`, then far-calls the body. **42** cases
compare all 14 words, a 1 KiB stack window and all four segments with no mask. The result SHA-256 is
`4b51bc08531d9cd4ee3b58451426f9a551e219df3d776db53a50452a77fac80a`.

```sh
ctest --test-dir <build> -R poolrad_resident_0464_compose_08d4_cpu --output-on-failure
python3 tools/overlay/emit_res0464_compose_08d4.py --check
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn:tools/overlay PYTHONDONTWRITEBYTECODE=1 \
  python3 tools/overlay/res0464_compose_08d4_cpu_audit.py --unicorn --native --output resources/0464-08d4/report.json
```

`CHECKER_ROWS = ('RES_0464_08D4',)`, and `--compile-only` builds the checker with no media. No classifier, queue or count
ledger is changed.
