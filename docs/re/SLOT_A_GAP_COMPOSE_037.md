# Slot-A Up: C composition of the `10D2:0165 → 10D2:13C6` window against the run-037 trace

This note composes the existing C models of the window, `pr_r08ae_dispatch_00dc_cpu` (the `00DC` dispatcher
that drives the `0165` return point), `pr_r08ae_table_17ae_cpu`, `pr_r08ae_table_1742_cpu` and
`pr_r08ae_vector_13b1_cpu`, in the order the [030 composition test](SLOT_A_MEASURED_INPUTS_030.md) uses, and
checks them against the run-037 `LOGL` trace of the same window (the trace and its store replay are described in
the run-037 replay note of PR #1969, which was not merged when this note was written). It is bounded evidence
for that window only. It claims no complete one-Up frame, no same-invocation C route, and no function count or
credit. Media, RAM images and traces stay in ignored `resources/`; this note holds hashes, addresses and numbers
only. No `src/` or `include/` file changes: the audit builds the models from the recovered sources with `ctypes`.

## Pins

Run 037 (`resources/oracle-runs/t4492-slot8-gap-037/`): `LOGL-W2-0165-to-13C6.TXT` `9a186281…` (766 lines),
`RAM-10D2-0165.BIN` `0914b16f…`, `RAM-10D2-13C6.BIN` `7f255423…`, `CPU-10D2-0165.TXT` `60cc866a…`,
`CPU-10D2-13C6.TXT` `65366307…`. Run 030 (second, independent instance of the same window, host hour 8 instead of
14): `RAM-10D2-0165.BIN` `b227e2c1…`, `RAM-10D2-13C6.BIN` `34288251…`, `CPU-10D2-0165.TXT` `be674fba…`,
`CPU-10D2-13C6.TXT` `c5403b94…`, and the later `RAM-2E60-0747.BIN` `99d632bb…`. The audit
(`tests/replay/audit_slot_a_gap_compose_037.py`) locks the full digests.

## What the models cover without the trace (A)

Starting only from the **native 0165 RAM image and the manual CPU transcription** (not the trace), the chain
`dispatch → 17AE → dispatch → 1742 → dispatch → 13B1` (13B1 stopped at its `INT 21h` by a NULL environment) was run
and its RAM diffed with native `13C6` byte by byte. "Without the trace" means no trace entry state and no replayed
store; the native 0165 data tables and CPU values are still inputs.

| Per-byte account of the 54 native delta bytes | Run 037 | Run 030 |
| --- | ---: | ---: |
| explained by the composed C image (value equals native) | 54 | 54 |
| unexplained | **0** | **0** |
| composed pre-INT image equals native `13C6` | all 655,360 bytes | all 655,360 bytes |

Last-changing component of the 54 delta bytes (identical in both runs): `17AE` 21, `1742` 25, `13B1` 4,
dispatcher glue 4. Bytes changed by each step: dispatcher 2, `17AE` 21, dispatcher 1, `1742` 25, dispatcher 4,
`13B1` 10 (4 + the 6 frame bytes below). The omission controls (revert the bytes a component last changed to their
0165 value) leave exactly those counts unexplained; they are close to definitional, so the mutants carry the weight.
The two composed images differ from each other at 121 bytes, which is the host-time noise in the two runs' inputs
(`SI`/`AX`/`DX`, the tick word and the stack bytes derived from them): the models read those values from RAM and CPU
and need no time-of-day input of their own.

## Where the models and the trace differ: one boundary, no disagreement

The `13B1` model stops **after** pushing the `INT 21h` frame (FLAGS, CS, next IP: six bytes at `SS:3DA8–3DAD`, `SP`
down by 6, IF cleared) and the trace stops **before** the `INT` executes. This is a window-boundary difference,
not a disagreement: the final CPU equals the transcription in every field except exactly `SP` (−6) and the IF bit
of FLAGS. Four of the six frame bytes lie on native delta bytes (stale stack bytes earlier code left there), so the
raw model image differs from native `13C6` at those six addresses. The audit therefore restores the six bytes to
their values before the `13B1` model ran, which is exact only because `13B1`'s own pre-INT pushes are disjoint from
the frame (checked, and a mutant that forces an overlap is killed). The frame values themselves equal run 030's
**later native `0747` image** at the same addresses (run 030 holds the frame below its `SP` after the `INT` returns),
so the six model-only bytes are independently corroborated for both runs.

## Per-span verification against the trace (B)

Each span is run from the trace's own entry state over `base + store-replay shadow` and its exit compared with the
trace row at the span's exit (all fourteen CPU fields, flags rebuilt from the per-flag columns) and the shadow RAM:

| Span (trace rows) | Model | Result |
| --- | --- | --- |
| 1 → 5 | dispatcher to `CALL_17AE` | exit regs equal, RAM 0 diffs |
| 5 → 378 | `17AE` (`RETF 2`) | exit regs equal, RAM 0 diffs |
| 378 → 382 | dispatcher to `CALL_1742` | exit regs equal, RAM 0 diffs |
| 382 → 755 | `1742` (`RETF 2`) | exit regs equal, RAM 0 diffs |
| 755 → 757 | dispatcher to `CALL_13B1` | exit regs equal, RAM 0 diffs |
| 757 → 765 | `13B1` to the `INT 21h` | exit regs equal except the INT effects above; RAM 0 diffs once the frame is undone |

So every model reproduces the trace from the trace's own entry state, and the chain without the trace reproduces the
same window: the trace adds no information the models lacked for these 54 bytes.

## Mutation probes

Twelve mutants of the audit's branches (`tests/replay/check_slot_a_gap_compose_037.sh`), **all killed**: DS perturbed
at the chain entry, a glue error (IP off by one before a dispatcher resume), the RAM effects of each of the four
model stages dropped (dispatcher 0, `17AE`, dispatcher 1, `1742`, dispatcher 2, `13B1`: six mutants), the INT frame
left in place, a perturbed span entry `SP`, a forced overlap between `13B1`'s pushes and the frame, and a shifted
frame reference. No equivalent survivor exists in this window.

## Limits and the C models' own status

* **The C models are unverified beyond this window.** Their own per-function audits (`res08ae_table_1742_cpu_audit.py`,
  `res08ae_table_17ae_cpu_audit.py`, `res08ae_vector_13b1_cpu_audit.py` and `res08ae_dispatch_00dc_cpu_audit.py`) were **not re-run** in
  this slice, and no parity, function count or credit is claimed. This note shows only that, on two native instances of
  this window, their composition reproduces the native delta and the trace.
* CPU files are manual `EV` transcriptions (flags included); stop receipts are the debugger's breakpoint display
  only; the debugger halts the guest at both stops and `LOGL`'s effect on timing is not excluded; the upper 384 KiB was
  never observed; host-time noise differs between the runs by construction; the baseline files of run 037 were copied
  from run 034 after byte equality with its `0165` image.
* The `13B1` model's environment (the `INT 21h AH=25` vector service and the port writes `40h`, `61h`, `C0h`) is not
  exercised here: the window ends at the `INT`.

## Next step

Extend the same method to the next link, `13C6 → 0747`: the `INT 21h AH=25` binding, the `IN 61h` value and the ordered
port writes, which the 030 composition test already covers against run 030's native images, verified span by span
against the 26-line run-030 `LOGL-W3` trace the way this note verifies `0165 → 13C6`, and re-run the three models'
own audits so their unverified status can be stated with a result.
