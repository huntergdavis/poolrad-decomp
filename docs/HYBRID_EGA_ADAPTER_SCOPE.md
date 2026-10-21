# Hybrid executor: the EGA/bus adapter, priced (VP i20594)

**Status:** desk scope; no code. Facts name their source: a file in this tree, or the pinned DOSBox-X source
(`5a9c7c53`, `src/...`).

**Why it gates stage (a).** Stage 0 measured that 5 of the 12 credited bodies on the cold-start path are `pr_bus` models
with a private EGA. One of them, `RES_0709_01DF`, accounts for 824 of the 976 arrivals
([HYBRID_STAGE0_OBSERVER.md](HYBRID_STAGE0_OBSERVER.md)).

**The answer, priced in detail at the end:**

| Scope | Lane-days |
|---|---|
| Adapter plus stream-equivalence validation | about 4.5–5.5 |
| Adding the full-EGA-state check (the 0294-style standard) | about 7.5–10.5 |

**Stage (a)** would therefore be roughly the scope's week plus one to two weeks for the adapter. The largest unknown is
whether the full-state check can snapshot and restore DOSBox-X's VGA state inside a run.

## 1. What the adapter has to do

**The interface it replaces is small.** A `pr_bus` model touches the device only through three functions
(`include/poolrad/recovered/memory_bus.h`, `src/recovered/memory_bus.c`):

- `pr_bus_read8`: physical A0000h–AFFFFh goes to `pr_ega_read`, which loads the latches; everything else reads RAM.
- `pr_bus_write8`: VRAM goes to `pr_ega_write`, after the `on_vram_write` observer; everything else writes RAM.
- `pr_bus_out`: the `ports->out` observer first, then `pr_ega_out`.

IN goes through `pr_port_env.in` (`include/poolrad/recovered/port_env.h`). Measured: **no recovered model calls it**
(`/usr/bin/grep -rln "ports->in\|->in(" src/recovered` is empty). The two "3DA" hits in `src/recovered` are code offsets,
not the status port, so no current model polls a status port.

**The design: route, don't translate.** A hybrid build links its own `memory_bus.c`, with the same three functions:

- RAM goes to DOSBox-X's guest memory (`MemBase`, `src/hardware/memory.cpp:261`).
- VRAM reads and writes go to DOSBox-X's own memory path, `mem_readb_inline`/`mem_writeb_inline` (`include/paging.h:441`).
  That path dispatches to the current VGA page handler. In planar modes this is `VGA_UnchainedVGA_Handler`
  (`src/hardware/vga_memory.cpp:579`), whose byte write runs `VGA_Generic_Write_Handler` and whose byte read runs
  `VGA_Generic_Read_Handler`, which loads the latches (`:586-592`).
- OUTs go to `IO_WriteB`, or `IO_WriteW` for 16-bit.
- `on_vram_write` and `ports->out` stay as observers, for counters.

**The model then never touches `pr_ega`.** Its private EGA is bypassed, and the device state is DOSBox-X's own, the same
state the original bytes see. That includes registers `pr_ega` does not model: CRT, attribute, DAC, odd/even, chain-4 and
write mode 3 (`include/poolrad/recovered/ega_hw.h:7-10`). "Our `pr_ega` is more abstract than the real device" therefore
stops being a hybrid problem.

What remains is whether the model issues **the same ordered stream of device accesses** as the original.

**Read-back.** When a model reads VRAM, it gets what the real device returns, with the real latches loaded, so it reads
back what it wrote on the same terms the original does.

**Word and byte granularity.** Our models issue byte accesses. DOSBox-X splits a word or dword VRAM write into byte
writes, low byte first (`do_write`, `vga_memory.cpp:602-608`), and reads the same way (`do_read`, `:594-600`).

**16-bit OUTs.** The sequencer, graphics-controller and attribute ports are registered byte-only (`IO_MB`):
`vga_seq.cpp:272-273`, `vga_gfx.cpp:253-254`, `vga_attr.cpp:448-450`. A 16-bit OUT to them therefore falls back to
`IO_WriteDefault`, which writes `port` then `port+1` (`src/hardware/iohandler.cpp:92-94`). That is the index/data
behaviour `pr_ega_out` models (`ega_hw.h`, "a word OUT to an index port writes the index, then that register's data").

**Raw-RAM models need a guard.** The 401 prototypes that take raw `uint8_t *ram` assumed A000 is flat RAM in their Unicorn
audits. In DOSBox-X, `MemBase[A0000h..]` is host memory that the VGA handlers shadow. A raw model writing there would
change nothing on screen and would not fail. The guard:

1. Snapshot `MemBase` A0000h–BFFFFh (128 KiB) and the rest of conventional RAM.
2. Run the model.
3. If the VGA window changed, restore everything and hand off to the original bytes.

At stage 0's 976 arrivals, a 1 MiB snapshot per call costs about 1 GB of copying per cold start, which is affordable for
stage (a).

## 2. The hard cases

1. **Reads were never checked by the differential.** In the Unicorn audits, A000 is flat RAM on the original's side. So a
   VRAM read there returns the last byte written, not latch-mode data. The differential compares the ordered OUT stream and
   the VRAM write operands (e.g. `tools/overlay/res0709_printer_0768_cpu_audit.py` docstring), **not read values or read
   order**. Any credited bus body whose control flow or written values depend on a VRAM read was checked for that only by
   its native DOSBox-X run. Through the adapter, its reads come from the real device. Validation (section 3) must cover
   reads; which of the 5 reached bodies branch on a VRAM read is UNKNOWN until measured.
2. **The 1 MiB mask against A20.** `pr_bus` masks every physical address to 1 MiB (`memory_bus.c`, `MASK`). If DOSBox-X has
   A20 enabled, an access the model computes at or above 100000h would differ. The adapter must refuse those accesses and
   hand off rather than mask them. Whether any reached body forms such an address is UNKNOWN; the guard makes it measured.
3. **Timing.**
   - DOSBox-X charges a delay per VRAM access (`VGAMEM_USEC_read_delay`/`write_delay` in the planar handler's `readb`/`writeb`, `vga_memory.cpp:612-625`). Through
     the adapter that delay is charged from inside the C call. Whether that is harmless while the core is between
     instructions is UNKNOWN; it is an emulated-time effect only.
   - No current model polls a status port (section 1), so a zero-time C call cannot spin waiting for a retrace that never
     comes. Any future model with an IN loop needs a budget and a handoff.
4. **Mode.** The adapter does not assume mode 0Dh: it uses whatever page handler is current, which is the one the original
   would use. Only `pr_ega` assumed mode 0Dh, and it is bypassed.
5. **Access order.** The Unicorn differential proves the order of OUTs and of VRAM writes. It does not prove the order of
   reads relative to writes, and latch loads make that order matter. Section 3 covers it.

## 3. Validating the adapter independently

Two levels. The first is cheap and strong; the second is the 0294-style full-state standard.

**V1: equal streams in the live game.**

- **Record the original.** At a hooked bus body's arrival, the observer records the original's ordered device stream from
  entry to return: VRAM byte reads with the value returned, VRAM byte writes with address and value, and port writes. The
  hook point has precedent: the copy of the pinned core this build came from already intercepts guest memory reads and
  writes through the core's load/store macros, the `pr_delay_readwatch` patch.
- **Run the model against a replay bus.** The replay bus returns the recorded read values in order, records the model's
  own stream, and changes no device.
- **Pass condition:** the two streams are equal, element by element.
- **Why that suffices:** the DOSBox-X device is deterministic in its start state and its input stream, so equal streams
  from the same start state give equal device state. This does not test the device itself, because the device is the
  thing being trusted. It tests what the hybrid adds: the model plus the routing.
- **Cost:** 2–3 lane-days.

**V2: equal full EGA state (the 0294 standard).**

1. At the arrival, snapshot guest RAM and the VGA state: the four planes in `vga.mem.linear`, the latches, and the
   sequencer, graphics, attribute, CRT and DAC registers in the `vga` struct.
2. Run the C body through the adapter and capture the full state.
3. Restore the snapshot, let the original run to its return, and capture again.
4. Compare unmasked, then continue from the original's state, so the game itself never depends on the model.

This is shadow mode for bus bodies. Cost: 3–5 lane-days. **UNKNOWN:**

- whether restoring the `vga` struct by copy is sound (it holds derived fields and handler caches, though no mode change
  happens inside these bodies);
- whether re-running from a restored state is safe inside the instruction loop.

DOSBox-X's in-memory save-state slots are an alternative restore mechanism; their use from inside the loop is also
UNKNOWN.

## 4. Price, in lane-days

| Item | Days | Unknowns |
|---|---|---|
| Hybrid `memory_bus.c` (RAM, VRAM and ports routed; IN routed for completeness; observers kept; the 1 MiB/A20 refusal) | 1 | low |
| Glue for the 5 reached bus bodies: confirmed symbols, CPU copy-in/out, declared budgets, a `pr_bus` with the EGA flag set so VRAM routes | 1 | the registry symbol column is a candidate only, and 1 of 12 was wrong (stage 0) |
| Raw-RAM guard (VGA-window check, snapshot, restore and handoff) | 0.5 | none known |
| V1 stream-equivalence recorder and replay bus | 2–3 | the recording hook inside the VGA handlers or core macros |
| V2 full-state shadow check | 3–5 | VGA state restore soundness; re-running inside the loop |
| **Adapter plus V1** | **4.5–5.5**, about **1 lane-week** | |
| **Adapter plus V1 plus V2** | **7.5–10.5**, about **2 lane-weeks** | |

**Stage (a)** is the scope's estimate (about 1 lane-week for stage 0, done, plus a narrow (a)) plus this adapter. So:

- with V1 only: **about 2 lane-weeks**;
- with the full-state standard: **about 3**.

These numbers assume no surprise in the UNKNOWN rows.

**The decision this leaves for you.** Is V1 (equal device-input streams on live entry states) enough for stage (a), with V2
run on a sample of arrivals? Or is V2 required at every hooked arrival? V1 tests everything the hybrid adds. V2 adds a
direct check of the device end state, at the cost of the riskiest piece of DOSBox-X surgery in the plan.
