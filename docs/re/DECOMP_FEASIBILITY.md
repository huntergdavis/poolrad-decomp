# 16-bit automated decompilation: feasibility spike

**Date:** 2026-10-10. **Lane:** A (DOS code recovery), issue #1. **Inputs:** the owner's DOS
v1.3 build, `START.EXE` sha256 `d825df2b…6a07bd` (65,231 bytes) and `GAME.OVR` sha256
`872964a3…928614` (206,105 bytes); full hashes in [overlay_inventory.md](overlay_inventory.md).
**Tooling:** Ghidra 12.1.4 headless (Temurin JDK 21), language `x86:LE:16:Real Mode`,
`BinaryLoader`, with the script and runner in [`tools/overlay/ghidra/`](../../tools/overlay/ghidra/).
Timebox: one afternoon. No decompiler output is committed; it is derived from the game's code.

## 1. What was tried

| Decompiler | Result |
| --- | --- |
| Ghidra 12.1.4 | Used. Installed into a scratch directory from the public release zip; needs a JDK 21+ (the system JRE lacks a compiler). Headless import plus a Java post-script took about 90 seconds per run. |
| Reko 0.12.5 | Not run. Only Windows builds are published; no `dotnet` runtime is installed here. |
| RetDec 5.0 | Not run. It supports 32- and 64-bit x86 only; no 16-bit real-mode support. |
| IDA, radare2, rizin | Not installed; radare2 is in apt but installing system packages was out of scope. |

## 2. Setup with overlay metadata applied

The target unit is overlay unit `0x1c` (file offset 3398, 13,581 code bytes, 56 Pascal
prologues, 2 public entries), which contains the ECL interpreter. Its fixup table has 402
entries; 308 are the segment words of far calls and 94 are other segment references.

Program layout created by `Spike.java`:

- resident `START.EXE` image (file offset 0x200 onward) at `0000:0000`;
- unit `0x1c` code at `2000:0000`;
- an uninitialized 64 KB block at `9000:0000` with the `DS` context register set to `0x9000`
  over the unit, so that `[imm16]` memory operands become named globals;
- one label and function per overlay entry stub (404 labels) named
  `ovl_u<unit>_e<entry>__ovr_<offset>`, resolved from the inventory's stub map;
- calling convention `__stdcall16far` on the four target functions (Turbo Pascal: callee cleans
  the stack; argument order is reversed relative to Pascal source).

A "bare" run skips the `DS` context and the stub labels so that the metadata's contribution can
be measured.

## 3. Results on four functions of the interpreter

| function (unit offset) | bytes | insns | far calls | C lines | calls resolved to overlay stubs | runtime calls still anonymous | artifact registers (`unaff_`/`extraout_`) meta / bare |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `ecl_run` (0x3171) | 66 | 22 | 0 | 14 | n/a (1 near call, resolved) | 0 | 0 / 1 |
| `ecl_dispatch` (0x2F2D) | 580 | 273 | 0 | 157 | n/a (49 near calls, all resolved to unit functions) | 0 | 0 / 1 |
| opcode 0x00 handler (0x0001) | 243 | 91 | 6 | 40 | 0 of 6 | 6 | 4 / 5 |
| opcode 0x20 handler (0x0BD1) | 163 | 66 | 10 | 28 | 5 of 10 | 5 | 2 / 3 |

Observations, per function:

- **`ecl_run`** decompiles to a faithful loop: store the parameter to the PC global, clear the
  stop byte, loop while both stop bytes are zero, latch previous opcode, fetch the byte at
  buffer pointer plus PC plus the 0x6700 bias, dispatch. One defect: the buffer far pointer at
  `0x49DE` is read as a near pointer (only its offset is added), because the decompiler does
  not model the `les di` segment load into the memory reference.
- **`ecl_dispatch`** becomes a 61-way `if / else if` chain on the opcode global. Ghidra does
  not recognise it as a `switch`, but every `push cs; call` pair resolves to a unit function,
  so the complete opcode-to-handler table falls out mechanically (see
  [ECL_CONTRACTS.md](ECL_CONTRACTS.md)).
- **Opcode 0x00 handler.** Control flow, the two conditional blocks, the stop-flag writes,
  the PC increment, and the loop that walks a linked list and calls a routine with argument 6
  are all recovered. All six far calls go into the resident runtime and stay `FUN_0000_xxxx`;
  knowing that one of them is Turbo Pascal `FreeMem` (so the loop frees 6-byte call-stack
  nodes) is not something the decompiler provides. The `push cs` idiom leaves `unaff_CS`
  artifacts and far pointers appear as two 16-bit halves.
- **Opcode 0x20 handler.** With the stub labels, five of its ten far calls read as calls into
  unit `0x25` entries 20 to 23 (the operand helpers) and unit `0x7f` entry 1 (the overlay
  loader). The other five are runtime string routines. Segment immediates such as `0xAF8` and
  `0x2B` show up as integer arguments because Turbo Pascal pushes far pointers to constants;
  patching the 94 non-call fixups to the unit's assigned segment would turn those into real
  pointers to the unit's constant strings.

## 4. Mechanical recovery versus behavioral understanding

Mechanical recovery is good: every instruction of the four functions decoded, no
`halt_baddata`, no `goto`, stack frames and local sizes right, every near call resolved, and,
with metadata, every far call into the overlay resolved to a named entry. The 16-bit real-mode
model costs little once `DS` is pinned.

Behavioral understanding is still almost entirely external:

- The names that make the C readable (PC, stop flags, call-stack head, opcode latch) came from
  prior static analysis, not from the tool.
- Runtime calls (1,836 sites across the overlay, segment `0xAF8` and others) are anonymous.
  Naming them needs a Turbo Pascal 5.x system-unit signature set or hand labeling of the
  resident image; neither exists yet in this repository.
- Pascal strings, far pointers, and `push cs` constant references need the fixup table applied
  as relocations rather than as labels.
- The decompiler cannot tell which helper reads operands or how. Those helpers are in unit
  `0x25` (verified by the call graph), so they can be decompiled the same way; the kit's claim
  that they are resident and need an emulator is wrong.

## 5. Recommendation for the roadmap

Use Ghidra headless as the reading aid for stage 2, with a project generator that: maps all
34 units at distinct segments with `DS` pinned; applies the 4,221 fixups (4,012 call-segment
words and 209 constant-segment words) as true relocations; labels the 404 stubs and the globals
from the ECL contract; and names runtime routines as they are identified. Treat every
decompiled function as a hypothesis for the address ledger, and confirm behavior against the
DOS oracle. Do not expect switch recovery or Pascal-order parameters. Keep all decompiler
output under ignored paths.

## 6. Reproduce

```
export GHIDRA_HOME=/path/to/ghidra_12.1.4_PUBLIC JAVA_HOME=/path/to/jdk-21
tools/overlay/ghidra/run_spike.sh resources/DOS/POOLRAD/START.EXE resources/DOS/POOLRAD/GAME.OVR /scratch/spike
tools/overlay/ghidra/run_spike.sh ... /scratch/spike bare
```

The runner prints one metrics line per function and writes the C into the scratch directory.
