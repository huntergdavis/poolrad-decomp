# EGA planar-mode model (`pr_ega`)

`pr_ega` (`include/poolrad/recovered/ega_hw.h`) is a C model of the EGA
graphics pipeline in planar mode, as used by video mode `0Dh`. It is
step 2's foundation in
[the RES 00BA:16EC port-harness plan](RES_00BA_16EC_PORT_HARNESS.md#plan).
Raw-CPU models of the EGA routines (`0709:00B7`, `01DF`, `0DD3` and
`03F1:0137`) will route their `A000` memory accesses and their EGA `OUT`s
through it.

It is written from the public EGA/VGA register documentation, and no
emulator code is involved (see the spec's licence boundary).

## Scope

- **Ports.** The sequencer index/data pair (`3C4h`/`3C5h`) and the
  graphics-controller pair (`3CEh`/`3CFh`). A word `OUT` to an index port
  writes the index, then that register's data. Other ports, and indexes
  beyond the register files, are ignored.
- **Memory.** Four 64 KiB planes behind `A000:0000..FFFF`, and four latches.
- **Reads.** A CPU read loads all four latches.
  - Read mode 0 returns the plane named by Read Map Select.
  - Read mode 1 returns the bits whose colour, in the planes enabled by
    Colour Don't Care, equals Colour Compare.
- **Writes.**
  - Write mode 0 rotates the CPU byte, then substitutes Set/Reset on the
    planes enabled by Enable Set/Reset.
  - Write mode 2 replicates CPU bit *p* to plane *p*.
  - Both then apply the ALU function (replace, AND, OR or XOR with the
    latch) and the Bit Mask, which chooses between that result and the
    latch.
  - Write mode 1 copies the latches.
  - All write modes write only the planes enabled by Map Mask.
- **Outside the model:**
  - odd/even and chain-4 addressing;
  - write mode 3 (VGA only), which returns `UNSUPPORTED`;
  - the CRT, attribute and DAC registers.
- **Reset state.** `pr_ega_reset` sets planes and latches to zero, Map Mask
  to `0Fh`, Colour Don't Care to `0Fh`, Bit Mask to `FFh`, and every other
  register to zero.

## Validation

**Focused C test.** `test_ega_hw.c` runs under CTest with ASan/UBSan. The
source compiles with `-Wconversion -Wsign-conversion -Wshadow -Werror`. It
passes **33** hand-predicted checks:

- byte and word `OUT`s, and ignored ports and indexes;
- Map Mask and Bit Mask against the latches;
- rotate with XOR;
- Enable Set/Reset with AND;
- the write mode 1 latch copy;
- write mode 2 under a Bit Mask;
- read mode 1 with and without Colour Don't Care;
- the write mode 3 refusal.

**DOSBox-X oracle.** `tools/overlay/ega_hw_native_audit.py` builds a
synthetic COM. On `machine=ega` it sets mode `0Dh` and runs a random script
of fixed 4-byte operations:

1. it sets every modelled register explicitly and clears a 256-byte
   window;
2. it primes the latches with one read;
3. it mixes byte `OUT`s, writes and reads.

It records every read and finally dumps the window of all four planes
through Read Map Select. `pr_ega` replays the same script, and every read
and plane byte must be equal.

| Seed | Operations | `OUT`s | Writes | Reads | Plane bytes | Result SHA-256 |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `0xE6A0` | 3,000 | 1,096 | 1,246 | 658 | 1,024 | `5f47c0bc40c7dedfca1d1c90e323c4eb7230b0523cd958542f8bba6bfa5112dc` |
| `0x7A11` | 3,000 | 1,170 | 1,203 | 627 | 1,024 | `943cd040f42605823a82dd52e5d8fbd5eb77b1cb7cdfef04478c1f07099ecf95` |

All reads and plane bytes were equal in both runs.

```sh
ctest --test-dir <build> -R poolrad_ega_hw --output-on-failure
PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/ega_hw_native_audit.py --seed 0xE6A0 --output resources/ega-hw/report.json
```

`pr_ega` is a shared hardware model, not a game routine, so it carries no
strict credit. It lets the EGA routines be modelled next.
