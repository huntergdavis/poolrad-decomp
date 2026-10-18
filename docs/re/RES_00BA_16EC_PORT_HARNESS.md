# RES 00BA:16EC closure: port-bound share and port-harness spec

This is a specification, written on 2026-10-18. The owner decided, via the
VP (i18076), to build a port harness by extending the DOSBox-X oracle as an
external tool. So far only the closure measurement below and the first pure
callee ([delay path](RES_0709_DELAY_0DBE_CPU.md)) exist; [the plan](#plan)
orders the rest.

## Why 16EC

`RES 00BA:16EC` is 247 bytes and 116 instructions, ending in `RETF 0Ch`.
**19** of the 75 handlers in issue #1019 call it directly:

- U38 `+017F`, `+0233`, `+070F`, `+07E0`, `+09C0`, `+0A65`, `+0C4B`,
  `+10DE`, `+1399`, `+176F`, `+1935`, `+1CAD`, `+1E77`, `+23EC`, `+2556`,
  `+2C1C` and `+2F57`;
- U5E `+39D2` and U86 `+30D9`.

Seven more reach it through U38 `+0058` or `+1545`: `+0775`, `+0B1B`,
`+165D`, `+1671`, `+1685`, `+170B` and `+2610`. No handler that depends on
16EC can become strict until 16EC itself is.

## The closure, measured

**Method.** A static recursive descent over the expanded START.EXE, starting
at `00BA:16EC`. It follows conditional and near jumps, and near and far
calls, including SYSTEM `0AF8`. It stops at `RET`, `RETF`, `IRET`, far or
indirect jumps, and `INT 3Fh`. Each unique instruction byte is counted once.

**Result.** 65 routines and **6,312** code bytes. The overlay routine behind
the `INT 3Fh` stub `0428:002A` is in GAME.OVR and is not counted.

| Share | Bytes | % |
| --- | ---: | ---: |
| Direct port, BIOS or DOS code | 1,936 | 30.7 |
| Transitively port-bound (calls such code) | 3,548 | 56.2 |
| Pure (no I/O anywhere below) | 828 | 13.1 |

About **87%** of the closure is port-bound. Direct I/O by class (a routine
can be in two classes):

| Class | Bytes | Routines |
| --- | ---: | --- |
| EGA ports (`OUT DX,AL`) | 1,335 | `0709:0000`, `0709:00B7`, `0709:01DF` (the 1,036-byte blitter) |
| DOS `INT 21h` | 461 | SYSTEM `00E9`, `01CD..01E7`, `16BC`; `0A32:00A2`; `08AE:137C`, `08AE:13B1` |
| BIOS `INT 10h`/`16h` | 138 | CRT `0A40:0143`, `02FD`, `030F`, `04F8`, `04FF`, `060B` |
| PIT, speaker and sound ports (`40h`, `43h`, `61h`, `C0h`) | 110 | `08AE:137C`, `08AE:13B1` |
| Ctrl-Break `INT 23h` | 61 | `0A40:0143` |
| `INT 3Fh` overlay trap | 2 | `0428:002A` |

16EC has seven direct callees:

| Callee | Status |
| --- | --- |
| SYSTEM `0B25` | pure; a covered runtime model |
| `0709:0DBE` | pure; raw-CPU form added by the delay-path PR |
| `03F1:0137` | port-bound: EGA through `0709:00B7` |
| `00BA:17E3` | port-bound: calls `03F1:0137` |
| `00BA:1816` | port-bound: EGA through `0709:0626` and `01DF` |
| `0709:0768` | port-bound: EGA, plus keyboard, PIT, speaker and DOS through `0709:0BF5` and `0899:0059` |
| `03F1:0000` | overlay trap through `0428:002A` |

The pure 13% is mostly modelled already: the SYSTEM string leaves, LongMul,
the `08AE` table writers and the delay path. What remains pure is about six
small SYSTEM leaves (`05C1`, `0D12`, `0DC0`, `16A8`, `1712`, `182A`) and
three tiny `0709`/`0899` leaves. SYSTEM `05FD` ends in an indirect `LCALL
ES:[BX+DI]`, the text-file driver dispatch, so its "pure" label is
provisional.

## Licence boundary

The Poolrad repository is public and has no licence. DOSBox-X is GPLv2.
**The emulator stays an external tool, and no GPL source enters the
repository:**

- **Invocation.** DOSBox-X runs as a separate process from the system
  install (`/usr/bin/dosbox-x`), as `tools/oracle/launch.sh` and every
  synthetic native audit already run it. No DOSBox-X source, header, patch
  or binary is copied, vendored, linked or committed, and nothing in
  `poolrad_recovered` links against it.
- **Other external tools.** The same boundary already covers Unicorn
  (GPLv2) and GNU `as`/`ld` (GPLv3). They are used at audit time from
  outside the repository. Their outputs (COM images, traces, reports) are
  generated artifacts under ignored `resources/`.
- **The tracer.** It is original 16-bit guest code emitted by our own audit
  scripts. It observes the emulated machine through architectural means
  only: the trap flag, the interrupt vector table, and port and memory
  reads.
- **C hardware models.** C models of EGA, BIOS or PIT behaviour are written
  from public hardware documentation and checked against traces. They are
  never derived from emulator source.
- **A future emulator build.** If a later interaction cannot be observed
  from the guest, any instrumented emulator build is made and kept outside
  the repository. Only its outputs, in the trace format below, are
  consumed.

## Extending the oracle in place

**Yes, the oracle can be extended in place.** The installed DOSBox-X
2026.01.02 exposes no debugger or I/O-logging options (`-help` lists none),
so the harness instruments the **guest**, not the emulator. The synthetic
COM program that every native audit already builds gains a trace layer.

No emulator rebuild is needed, and none of the 16EC closure needs one. The
only fallback, if a future interaction is invisible from the guest, is the
out-of-repository build described above.

## Harness design

1. **Two runs per case.**
   - The **plain run** is today's oracle, with interrupts off and no
     tracing. It gives the full-state comparison: all 14 CPU words, the
     stack window and the data segment.
   - The **traced run** repeats the case with tracing. It yields the
     interaction sequence and the live state.
   - The CPU pushes FLAGS, CS and IP below SP on every trap, so the traced
     run's dead stack below SP is not compared. The plain run covers it.
2. **Single-step tracer.**
   - The harness installs its own `INT 1` handler and enters the routine
     with TF set (`PUSHF`; set bit 8; `POPF` just before the far call).
   - At each step the handler decodes the instruction at the interrupted
     CS:IP. `IN` and `OUT` are `E4`–`E7` and `EC`–`EF`; `INS` and `OUTS`
     are `6C`–`6F`.
   - It records the port (DX or imm8) and, for `OUT`, the value. An `IN`
     result is recorded at the next step, from the destination register.
   - The handler runs with TF clear and returns with `IRET`. An address
     filter can skip hot pure loops such as `0A40:02BD`.
3. **Interrupts.**
   - A `CD xx` at the stepped CS:IP records an INT-enter event with all
     registers. The next step after the service's `IRET` records the
     INT-leave event with registers and flags.
   - Services run natively but deterministically:
     - **Keyboard.** The BIOS keyboard ring (`0040:001A..003D`) is filled
       with scripted keys.
     - **Files.** DOS file calls act on scripted files in the mounted
       directory.
     - **Termination.** A harness `INT 21h` hook catches `AH=4Ch` (the SYSTEM
       termination path `00E9`), records a terminate event and returns to the harness
       on its own stack. `INT 23h` and `24h` point at harness stubs.
     - **Passthrough.** The hook is transparent when no case is running, so
       the harness's own file writes are untouched.
4. **EGA capture.**
   - The DOSBox-X default VGA machine keeps the EGA modes, and its
     registers can be read back.
   - After each case the harness dumps the sequencer and graphics-
     controller register files and all four planes. It reads them through
     Read Map Select, `3CEh` index 4, with read mode 0.
   - The port log gives the exact order of register writes. The plane dump
     gives the effect of every `A000` memory write under the active write
     mode.
5. **PIT, speaker and sound.** Writes to `40h`, `42h`, `43h`, `61h` and
   `C0h` are logged. `IN 61h` and PIT latch reads are timing-dependent, so
   the C model replays their recorded values instead of predicting them.

## Trace format

The file starts with an 8-byte header: `"PRTR"`, a version word and a case
count. Each case then has:

- a 16-byte preamble: the case index, the event count, the live BIOS
  `0040:0000` byte, and scripted-input digests;
- fixed **32-byte** event records, little-endian.

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 2 | sequence number |
| 2 | 1 | kind: 1 `OUT8`, 2 `OUT16`, 3 `IN8`, 4 `IN16`, 5 `INS`, 6 `OUTS`, 7 INT-enter, 8 INT-leave, 9 terminate, 10 overlay trap (`INT 3Fh`) |
| 3 | 1 | interrupt vector, or the operand size for string I/O |
| 4 | 4 | CS:IP of the instruction (IP, then CS) |
| 8 | 2 | port (DX or imm8) |
| 10 | 2 | value: written for `OUT`, read for `IN` |
| 12 | 20 | INT events only: AX, BX, CX, DX, SI, DI, BP, DS, ES, FLAGS; zero otherwise |

After the events come the four plane dumps and the register files, when
the case asks for EGA capture.

## Comparing a C model

Port-bound raw-CPU models take an environment with three callbacks:
`out(port, value, size, cs, ip)`, `in(port, size, cs, ip) -> value` and
`interrupt(vector, regs, ram)`. The audit's replay environment walks the
native trace in order:

- each `out` must equal the next `OUT` record: port, value, size and CS:IP;
- each `in` must match the next `IN` record's port and CS:IP, and returns
  its recorded value;
- each `interrupt` must match the next INT-enter record's registers. It then
  applies the INT-leave record's registers and flags, plus any memory the
  service wrote, taken from the plain run's dump.

A model must consume exactly the recorded events. The first mismatch is
reported with its sequence number.

Memory writes to `A000` go through a C EGA model with write modes 0–2,
latches, Map Mask, Set/Reset, Enable Set/Reset, Data Rotate and function,
and Bit Mask. Its planes and registers are compared with the dump. CPU
state, the stack and the data segment are compared with the plain run as
today.

**The Unicorn half.** It can use the same replay environment, since it
hooks `IN`/`OUT` with `UC_HOOK_INSN` and interrupts with `UC_HOOK_INTR`. It
cannot emulate planar EGA memory, so it compares everything except `A000`,
and planes are checked only natively.

## Scope

With the harness, **5,752 of the 6,312 bytes (91.1%)** become testable:
every routine except those that reach the overlay trap.

**What remains outside the harness:**

- **The `INT 3Fh` overlay-trap class** (560 bytes): the stub `0428:002A`,
  `03F1:0000` and 16EC itself.
  - The stub loads a GAME.OVR routine through the Turbo Pascal overlay
    manager. Testing that path needs a model of overlay-manager state, or
    the real game image.
  - Until then, `03F1:0000` and 16EC can be strict partials that stop with
    `PENDING` at the stub call, as U7D `+0000` does at `+01FE`. The VP
    accepts such a partial once the other callees are done.
- **SYSTEM `05FD`'s indirect text-driver call.** It is testable natively
  once synthetic text-file records are set up, and stays a guarded stop
  until then.
- **Timing-dependent reads** (`IN 61h`, PIT latches). These are replayed,
  not predicted.

## Plan

Work in dependency order. Each step is one PR with Unicorn, native and
focused-test evidence, as now.

1. **Tracer prototype.** Add `INT 1` stepping and the two-run scheme to the
   synthetic COM. Prove it on `0709:0000`, 113 bytes of EGA register
   writes, with the port log only. **Done:** see
   [RES_0709_EGA_RESET_0000_CPU.md](RES_0709_EGA_RESET_0000_CPU.md), where
   576 traced OUT events equal the C model's port environment.
2. **Shared environment and EGA.**
   - Add the C `out`/`in`/`interrupt` environment and a C EGA model.
   - Model `0709:00A0` (pure, 23 B) and `0709:0000`.
   - Then model `0709:00B7` (rect fill), then `03F1:0137` (188
     instructions) with SYSTEM `16A8`, then `00BA:17E3`.
3. **Blitter chain.** `0709:01DF` (1,036 B), then `0709:0626`, then
   `00BA:1816`.
4. **CRT, timer and sound.**
   - Model the keyboard and video BIOS routines in CRT `0A40`, using the
     scripted keyboard ring.
   - Then the PIT, speaker and sound routines in `08AE`, then `0899:0000`
     and `0899:0059`.
   - Then `0709:0BF5`, then `0709:0768`.
5. **Overlay-trap partials.** Model `03F1:0000` with `PENDING` at
   `0428:002A`, then 16EC as a partial that inherits it. Then compose the
   19 direct handlers.
