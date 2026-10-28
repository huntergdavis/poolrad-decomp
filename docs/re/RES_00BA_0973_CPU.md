# `RES_00BA_0973` raw-CPU model and composed differential

Issue: resident-first conversion of the `+0x11B` display callee. This adds a
raw-CPU model and an original-byte differential; it does not claim ownership,
strict credit, native parity or exhaustive inputs.

## Body and callers

`START.EXE` resident `00BA:[0973,09C9)` is 86 contiguous copied bytes, 32
instructions, SHA-256 `a5093db76117998bf62e1d6a3b0ca1816f6109dd7d3bca570070a09c0f73ebd0`.
The body reads node `+0x11B` and `+0x32`, selects style `0x0E` when the first
byte is below the second and `0x0A` otherwise, forces `0x0D` when the override
byte `[BP+6]` is nonzero, formats a fresh node `+0x11B` byte through the
accepted `00BA:1037` model into `SS:BP-0102h`, prints it through the accepted
`0709:0626` model, and returns via `RETF 0x0A` with the printer's AL. The
five-word frame is node offset/segment, x, y, override. Static callers are
the overlay sites in the field doc and the resident `00BA:0767`/`00BA:09E3`
near calls.

## Model

`src/recovered/resident_00ba_0973_cpu.c` (`pr_res_00ba_0973_cpu`) runs the
body on the supplied `pr_bus`; it calls the accepted `pr_res_00ba_1037_cpu`
and `pr_r0709_printer_0626_cpu` models. It checks the near-call bytes
`E8 79 06` at `09BB` and the far call `9A 26 06 09 07` at `09BE`, and returns
`ARGUMENT`, `CODE`, `CALLEE` or `TRANSFER` rather than attaching success to a
failed callee.

## Differential

`tools/overlay/resident_0973_cpu_audit.py` places the hash-checked expanded
`START.EXE` image at load segment 0, supplies the printer font/tables and a
node, and runs the original body under Unicorn against the C model. It
compares all 14 CPU words, 1 MiB RAM outside `A0000..AFFFF`, the ordered EGA
OUT events and the ordered A0000 write operands, with no flag masks. Cases
cover below/equal/above, zero and maximum byte edges, override zero/nonzero,
x/y edges and stack placement; the full run reports 32/32 body starts and 4/4
conditional outcomes.

## Acceptance window: entry SP and composed stacks

The claimed entry-SP domain is **`[0x0242, 0xFFF0]`** (16-bit). The lower bound
is the printer contract: 0973 pushes five words and a far frame, so `0709:0626`
enters at `SP-0x112`, and its header requires entry SP `>= 0x130`; `0x0242` is
the smallest entry that satisfies it (the body and `1037` need only `0x216`).
The upper bound keeps every caller-frame word at or below `0xFFFF`.

`SP > 0xFFF0` is an **excluded oracle band**: the C model performs 16-bit
address arithmetic, while the repository's Unicorn oracle reads the return
frame contiguously. This is the known Unicorn/DOSBox distinction, not a C or
native defect; `SP=0xFFFF` is the one reviewer case that lands there. The 15
author cases use `0x8000`/`0x8100`; the reviewer's adversarial set passes 30/31
with only `0xFFFF` outside the band. This is a declared acceptance window, not
an input-domain proof.

Composed callee limits: `1037` needs its `0x100`-byte frame plus the
`Str`/`StrStore` calls; `0626`'s header requires entry SP in `[0x130, 0xFFF2]`;
both are inside the window.

## Limits

The printer and formatter are their accepted models; this audit does not
re-derive their internal paths. The EGA comparison follows the printer
audit's A0000-write observation, not a full hardware model. The model is
load-zero only: it requires the resident code at `CS=00BA` and the printer at
`0709:0626`, and is not relocated-load parity. `--unicorn` is parsed for
profile symmetry but execution is unconditional. The entry FLAGS are not
varied (all cases use `0202`); the 15 cases are visited starts/edges, not an
input-domain proof. No native, full-game, ownership or credit claim is made;
ownership is unresolved.
