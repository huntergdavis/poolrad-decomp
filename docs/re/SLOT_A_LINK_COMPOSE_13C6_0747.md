# Slot-A Up: C composition of the `10D2:13C6 → 2E60:0747` window (run 030) and the four models' own audits

This note applies the trace-free composition method to the last link of the `0165 → 13C6 → 0747` chain: the
`INT 21h` that restores the IRQ0 vector (`AH=25h`), `IN 61h = 0x30`, the ordered port writes, and the return to
`2E60:0747`. The [030 measured-inputs note](SLOT_A_MEASURED_INPUTS_030.md) already composes this link in a C test
against run 030's native images; here the composition is repeated in the audit harness, accounted byte by byte, and
verified against the 26-line run-030 `LOGL-W3` trace. The four C models' own parity audits were re-run. It is bounded
evidence for that window only. It claims no complete one-Up frame, no same-invocation C route, and no function count or
credit, and **nothing about parity, function status or credit follows from a same-trace re-check**. Media, RAM images and
traces stay in ignored `resources/`; this note holds hashes, addresses and numbers only. No `src/` or `include/` file
changes: the audit builds the models from the recovered sources with `ctypes`.

## Pins (ignored `resources/oracle-runs/t4492-slot4-logc-030/`)

`LOGL-W3-13C6-to-0747.TXT` `4960c5b9…` (26 lines), `RAM-10D2-0165.BIN` `b227e2c1…`, `RAM-10D2-13C6.BIN` `34288251…`,
`RAM-2E60-0747.BIN` `99d632bb…`, `CPU-10D2-0165.TXT` `be674fba…`, `CPU-10D2-13C6.TXT` `c5403b94…`,
`CPU-2E60-0747.TXT` `c4b27985…`. The audit (`tests/replay/audit_slot_a_link_compose_13c6_0747.py`) locks the full digests.

## What the models cover without the trace (A)

The `13B1` model cannot be resumed at `13C6` (arbitrary-PC resume is excluded), so the window is reached through the
whole function: from the **native 0165 RAM image and CPU transcription only**, the chain `dispatch → 17AE → dispatch →
1742 → dispatch → 13B1` runs under a declared environment (below) and ends with the dispatcher's `RETF 2`.

| Account | Result |
| --- | --- |
| composed image at the `INT` (model's frame restored) vs native `13C6` | equal in all 655,360 bytes (4 pre-INT pushes belong to the previous window) |
| composed RAM at `2E60:0747` vs native `0747` | **0 differing bytes** |
| native `13C6 → 0747` delta | 26 bytes, **explained 26, unexplained 0** |
| final CPU vs `CPU-2E60-0747.TXT` | equal in all 14 fields (flags included, `7206`) |

The 26 delta bytes by writer: the **C model's INT frame push** 6 (`SS:3DA8–3DAD`: FLAGS, CS, next IP), and the **declared
DOS binding** 20 (nine saved registers below the frame 15 changed bytes, the PSP stack-pointer words 1, the INT 08h
vector 4). The binding stores 26 bytes in all; 20 change value. Omission controls: dropping the model's frame leaves 6
bytes unexplained, dropping the declared binding leaves 20.

## The declared environment is a hypothesis, not a measurement

The `AH=25h` service (vector write at the IVT entry for INT 08h, PSP stack pointer `0816E/08170`, the nine saved
registers at `SP-18`, returned FLAGS `7246`) is the run003/030 binding, **declared**: the trace shows the callback
(`callback 0038`) returning straight to the stub's `iret`, and its internals are not instruction-traced. It is
supported only by byte equality with native `0747` (26 bytes) and by the hook's precondition checks (AX/DX/DS, IVT
`10D2:13EA`, the PSP bytes, the frame words). The `IN 61h` value (`0x30`) is a **measured input** read from the trace (the
`AL` after the `IN`); the old synthetic `03` is refuted (a mutant using it is killed: `OUT 61h` would be `00`, the trace
shows `30`).

## Against the W3 trace (B) and the transcriptions (C)

* Registers the model hands to the DOS hook equal trace row 3 (`callback 0038`) in all fields except CS/IP/FLAGS.
* The model's INT frame words (`13C8`, `10D2`, `7246`) equal the trace (next IP and CS from row 5, FLAGS from row 0).
* The OUT sequence recorded by the model's environment (`40h←FF`, `40h←FF`, `61h←30`, `C0h←9F,BF,DF,FF`) equals the
  seven `OUT` rows of the trace in order and value; one `IN 61h`.
* The model's return state at `10D2:0177` equals trace row 23 in all 14 fields.
* The dispatcher's return span, from the trace's own entry state (row 23) to `2E60:0747`, returns with exit registers equal
  to trace row 25 and RAM unchanged. This is the only span the trace can enter independently.
* All 14 fields (flags included) of `CPU-10D2-13C6.TXT` and `CPU-2E60-0747.TXT` equal the trace's first and last lines.

## Boundary differences (not disagreements)

1. The `13B1` model cannot start at `13C6`, so its span is verified through its events, not from a trace entry state.
2. The model stops after the INT frame push where the trace shows the INT executing; with the frame restored, the image at
   the INT equals native `13C6` (previous link).
3. The W3 trace has no store rows for the callback: the 20 binding bytes are verified by native-image equality only.

## The four models' own audits, re-run

Each audit executes the original START.EXE bytes in Unicorn and compares **all 14 CPU words and the whole 1 MiB RAM, with
no mask**, against the C model. `--compile-only` and `--unicorn` were run for all four, **rc 0 each**; the `--native` modes
(synthetic native DOSBox-X runs) were **not** run here.

| Model (body SHA-256) | Original-byte starts | Cases | Reference-checked / other |
| --- | --- | --- | --- |
| `1742` `6128c512…` | 43 of 43 | 607 (603 return, 4 argument) | 275; conditional outcomes 6 of 6 |
| `17AE` `8bb73768…` | 43 of 43 | 1,023 (1,019 return, 4 argument) | 275; conditional outcomes 6 of 6 |
| `13B1` `0fa5df2c…` | 27 of 27 | 480 (all return) | 4,320 environment events, all 256 `IN` values, 64-case `137C → 13B1` round trip with the vector restored |
| dispatcher `00DC` `aae16730…` | 77 of 77 | 1,788 + 5 argument | 1,050; conditional outcomes 16 of 16 |

Masked RAM bytes 0 in all four; the reports' `ownership` records say `credit_claimed: false`, `identity: unresolved`.
**Status stated plainly:** the four models pass their own original-byte Unicorn differentials on synthetic RAM and this
window composes against run 030's native images only; the identity of these routines is recorded as unresolved, native corroboration was
not re-run, and the repository's credit rules are untouched.

## Mutation probes

Nine mutants (`tests/replay/check_slot_a_link_compose_13c6_0747.sh`): eight are killed (the old synthetic `IN 61h = 03`,
saved-register order, PSP stack-pointer value, vector segment, returned IF bit, the binding's stores skipped, a perturbed
dispatcher entry `SP`, a dispatcher resume at the wrong `IP`). One survives and is equivalent: the ZF/PF bits of the
FLAGS the service returns are overwritten by the following `AND AL,FCh`, as the 030 note already records.

## Limits

CPU files are manual `EV` transcriptions; stop receipts are the debugger's breakpoint display only; the debugger halts the
guest at the stops and `LOGL`'s effect on timing is not excluded; the upper 384 KiB was never observed; host-time noise
(`SI`, `AX`, `DX`, the tick word) differs between runs by construction, and this window is from run 030 alone (the
`0165 → 13C6` window of run 037 and the `0747 → 3492` window of run 034 are different runs); the DOS binding is a
hypothesis; the `OUT`/`IN` values are the model environment's, compared with the trace, not read from hardware.

## Next step

The whole chain `0165 → 3492` is now traced and composed window by window from three runs. The open step is a single-run
chain: capture `0165 → 13C6 → 0747 → 3492` in one invocation (or accept the documented cross-run bound), and, separately, the
`--native` corroboration of the four models, which this slice did not run.
