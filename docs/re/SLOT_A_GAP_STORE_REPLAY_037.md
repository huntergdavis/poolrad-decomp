# Slot-A Up: store replay of the `10D2:0165 → 10D2:13C6` window (run 037)

This note traces the one window of the saved-slot-A Up chain `00F4 → 0165 → 13C6 → 0747 → 3492` that had no trace of its own. The chain's windows come from different native runs (host hours 8, 14 and 15), so this is a statement about this window and its run-030 counterpart, not a single-run chain.
[SLOT_A_MEASURED_INPUTS_030.md](SLOT_A_MEASURED_INPUTS_030.md) explained the 54 native bytes of
`0165 → 13C6` from the C models without a trace; this run traces the window (`LOGL`, first log of a fresh
native run) and replays its stores over the native `0165` image with the same method as the
[0747 → 3492 replay](SLOT_A_WINDOW_STORE_REPLAY_034.md). It is bounded evidence for that window only. It
claims no complete one-Up frame, no same-invocation C route, and no function count or credit. Media, RAM
images and traces stay in ignored `resources/`; this note holds hashes, addresses and numbers only.

## Run and pins (ignored `resources/oracle-runs/t4492-slot8-gap-037/`)

Fresh single invocation: reviewed controller under `poolrad-heavy.sh`, `TERM=xterm-256color`, the pinned
debugger, screenshot-gated keys, breakpoints only at `10D2:0165` and `10D2:13C6`, one Up key, `LOGL 5000`
(hex) issued at the `0165` stop as the first log of the run and continued with `RUN` to the `13C6` stop.
Capture manifest `MANIFEST-CAPTURE.sha256` (17 entries) `8da214c5…`; cleanup by an identity-checked `TERM` of
the recorded controller (outer and observer rc 0, inner 143, `post-audit.txt` present, nothing left running).

| File | SHA-256 |
| --- | --- |
| `LOGL-W2-0165-to-13C6.TXT` (766 lines, 246,652 B, 0.258 ms emulated) | `9a186281…` |
| `RAM-10D2-0165.BIN` / `RAM-10D2-13C6.BIN` | `0914b16f…` / `7f255423…` |
| `CPU-10D2-0165.TXT` / `CPU-10D2-13C6.TXT` | `60cc866a…` / `65366307…` |

Run 030 files used for the cross-run control: `RAM-10D2-0165.BIN` `b227e2c1…`, `RAM-10D2-13C6.BIN`
`34288251…`, `CPU-10D2-0165.TXT` `be674fba…`. The audit
(`tests/replay/audit_slot_a_gap_store_replay_037.py`) locks the full digests.

## Result

The first line repeats the `0165` breakpoint instruction and the last is the `13C6` stop state (an `int 21h`
not yet executed), so 764 lines execute. 254 stores are decoded from the logged instruction bytes with capstone
and addressed with the logged registers.

| Per-byte account of the 54 native delta bytes | Bytes |
| --- | --- |
| replayed writer exists and the replayed value equals native | 54 |
| writer exists, value differs | 0 |
| native changed, no replayed writer (**unattributed**) | **0** |
| replay changes a byte native left unchanged (spurious) | 0 |
| written by a replayed store but equal in both images (benign) | 356 |

The predicted image equals native `13C6` in all 655,360 bytes. **Who writes what** (last writer):

| Writer | Bytes | Where |
| --- | ---: | --- |
| `1742` table entry | 34 | 26 resident data in `10F00..110FF`, 8 stack |
| `17AE` | 12 | 3 in `10F00..110FF`, 9 in `11100..111FF` (three 3-byte groups at a 0x30 stride) |
| `13B1` | 4 | stack |
| `0165` body | 4 | stack pushes |

By opcode: `mov` 38, `push` 14, `call` 2 (stores: `mov` 224, `push` 19, `dec` 8, `call` 3). The stack bytes span
`SS:3DA4–3DB5`. The delay writes nothing here: it ended at `0165`.

## Declared writers: none needed (fail-closed)

Unlike the 030 and 034 windows this one needs no declared host writer, and the audit **fails** if the trace
contains an IRQ entry, a `callback` line, an `IN`/`OUT`, a line outside segment `10D2`, a store at or above
`0xA0000`, a replayed read of display memory, a string or `xchg` store form, or a BIOS tick change. None occurs:
no `13EA` hook entry, no DOS/BIOS/idle callback, no port access, no display store; the tick word is the same in
both images.

## Controls

* **Trace read annotations.** 260 lines carry a `seg:[off]=value` memory value measured by the emulator; all agree
  with the replayed shadow memory (0 mismatches).
* **Log vs the manual CPU transcriptions.** All 14 fields (flags included) of `CPU-10D2-0165.TXT` and
  `CPU-10D2-13C6.TXT` equal the trace's first and last lines.
* **Omission controls** (bytes left unexplained when a routine or opcode class is dropped, by last writer): by
  routine `1742` 34, `17AE` 12, `13B1` 4, `0165` body 4; by opcode `mov` 38, `push` 14, `call` 2. They are close
  to definitional (each equals its class's last-writer count); the mutants carry the weight.
* **Cross-run control against run 030.** The two native deltas have the identical address set (54 and 54). 53 of
  the 54 final values are equal. The one difference, `SS:3DA6`, is written by a replayed `push si` (rows 10 and 387)
  and `SI` is the host hour: `000E` in this run, `0008` in run 030, and run 030's byte equals its own `SI`. The tick
  word is host time of day too: 921,770 is hour 14 and 545,920 is hour 8, matching `SI` in both runs. The two `0165`
  images differ in 126 bytes (time, tick and stack noise).

## Mutation probes

Ten mutants of the audit's branches (`tests/replay/check_slot_a_gap_store_replay_037.sh`): eight are killed
(stop row applied, `push` skipped, `call` return address off by one, `mov` stores dropped, `dec` by two, cross-run
differing set expected empty, cross-run register wrong, host hour off by one). Two survive and are equivalent in this
window: re-applying the duplicated first line (its `xor ax,ax` stores nothing) and ignoring the SS default of
BP-based operands (the window's only two BP-based operands are register loads).

## Limits

CPU files are manual `EV` transcriptions; stop receipts are the debugger's breakpoint display only; the debugger
halts the guest at both stops and `LOGL`'s effect on timing is not excluded; the upper 384 KiB was never observed;
trace quirks (first line at a stop logged twice, lazily evaluated `FLG`, 1/3 µs timestamps); host-time noise
(`SI`, `AX`, `DX`, the tick word) differs between runs by construction; the baseline files were copied from run 034
after byte equality with this run's `0165` image. Cleanup state is above. No frame, route or credit is claimed.

## Missing inputs and next step

None is missing for the byte account of this window. What remains open for the chain is composition, not evidence:
the C models `1742`, `17AE` and `13B1` have not been verified span by span against this trace the way
the 0747 → 3492 composition (PR #1968, not merged when this note was written) verifies its models (exit registers, RAM). The next step is to
run them over this `0165` image from this trace's entry states, check each exit state and RAM against the
replayed shadow, and diff the composed image with native `13C6`; the `INT 21h AH=25` that follows (`13C6 → 0747`)
is already traced in run 030 (`LOGL-W3`).
