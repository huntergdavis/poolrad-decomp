# First-page `RES_0464:+16A1` natural capture recipe

This is a **source-only plan**. No new emulator run or native byte parity is
claimed. The one-byte C model and its synthetic bridge remain bounded at the
first `REP MOVSB` iteration; they do not establish page, RGB, or full-frame
parity. The bounded `deja` query for this DEBUGBOX stop found no matching
session. This plan reuses the hash-locked first-byte operand audit and the
synthetic capture bridge in this tree.

## Stop and time boundary

The original-image `0AF8:16A1` notation names the routine in the existing
ledger. The reviewed loaded caller at `0C88:1DA0` invokes **`131C:1685`**;
the natural breakpoint is therefore **`BP 131C:16A1`**. The corrected bridge
takes its expected loaded segment from the
[hash-locked caller audit](../../tools/replay/res464_16a1_loaded_pc_audit.py),
which checks the ignored flush-entry RAM and extracts the relocated far-call
operand. The segment is not fixed in the C bridge. This repairs the parser's
address contract; it does not establish a natural native stop.

Use a fresh ignored `resources/` run directory with the pinned saved-slot-A
inputs and a private DEBUGBOX command transcript. When PM grants the sole
native slot and CPU PSI ten-minute average is below 20, run the established
network-isolated `xvfb-run`/`bwrap --unshare-net` launcher with DOSBox-X
`-time-limit 85` under an outer `timeout -k 2s 88s`. These are total elapsed
limits, not 85 seconds after the breakpoint. If startup cannot reliably hit
the breakpoint within that budget, stop and report a timeout; do not extend
the slot or substitute a stale capture. Earlier 170/220-second launch scripts
are not this recipe.

At the first loaded `131C:16A1` stop, **before any `OUTP`**:

```text
EV CS IP DS ES SS SP BP AX BX CX DX SI DI FLAGS
VGA GC
VGA SEQ
VGA MODE
```

Record the exact output and run provenance privately. Check `CS:IP` before
interpreting any value. The expected `DS=A200`, `SI=03C3`, `ES=A000`,
`DI=03C3`, `CX=000B`, and clear direction flag are hypotheses from the
previous flush-entry evidence, **not** substitutes for this stop's CPU state.
`VGA GC` exposes GC4 read-map, GC5 mode and GC6 miscellaneous, and its
current index. `VGA SEQ` exposes Seq2 map mask and Seq4 memory mode. `VGA
MODE` exposes compatible-chain4 and read/write modes. Preserve the raw
transcript so these fields are not reconstructed from earlier register writes.

If the measured read mode is zero and the mapping permits selecting a plane
with GC4, perform the following four iterations while still stopped. Use
`P=00,01,02,03` in order. `MEMDUMPBIN` overwrites `MEMDUMP.BIN`, so move each
private dump to a distinct ignored filename immediately after its command.

```text
OUTP 3CE 04
OUTP 3CF P
MEMDUMPBIN A000:23C2 2
# Move MEMDUMP.BIN to an ignored source-P.bin immediately.
MEMDUMPBIN A000:03C2 2
# Move MEMDUMP.BIN to an ignored page-P.bin immediately.
```

Each two-byte dump covers both possible GC6 mappings. For `GC6=04`, the
source and old-page bytes are at planar offsets `23C3` and `03C3`; for
`GC6=06`, they are at `23C2` and `03C2`. Thus the selected four source plus
four old-page plane bytes can be extracted **after** GC6 is measured. The
other adjacent bytes are diagnostic only. Record the original GC index and
GC4 value from `VGA GC`; restore GC4 and then the original GC index before
resuming. If read mode, mapping, or restoration is uncertain, stop without
claiming eight valid plane bytes or continuing execution.

## Typed boundary before projection

The synthetic bridge additionally requires the actual handler selection,
`ignore_odd_even`, `memmask`, `bank_read`, and `bank_write`. The checked
DEBUGBOX `VGA GC`/`VGA SEQ`/`VGA MODE` source does **not** expose all of those
fields. In particular, do not infer `memmask=FFFF` or zero banks from the
visible register output, and do not label CPU-window dumps as physical planes
until the handler is confirmed. Capture these missing values through a
separately reviewed host-side instrumentation step, or report
`STOP_UNMEASURED_EGA_MAPPING`. The earlier `STOP_LOADED_PC_CONTRACT` is
resolved in source by the caller-derived expected segment, but no native
record has been supplied to exercise it.

Only after those stops are resolved may a private native record be converted
to the bridge's 29-field format and compared with the bounded C projection.
Keep screenshots, memory/plane dumps, decoded media, and the transcript under
ignored `resources/`. This plan supplies neither a record nor a native match.

## Source checks

`tools/replay/res464_16a1_explicit_byte_audit.py` hash-locks the ignored
START.EXE and the first-flush RAM/GC inputs, checks the original `+16A1`
`REP MOVSB`, and derives the conditional first host byte. The DOSBox-X heavy
debugger source was read only for `BP`, `EV`, `VGA GC`, `VGA SEQ`, `VGA MODE`,
`OUTP`, and `MEMDUMPBIN` syntax. `MEMDUMPBIN` writes `MEMDUMP.BIN` in its
working directory. None of these source checks proves a natural stop.
