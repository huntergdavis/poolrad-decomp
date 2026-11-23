# Slot-A Up: the whole chain `0165 → 13C6 → 0747 → 3492` in one native invocation (run 038)

Until now the saved-slot-A Up chain between the delay's end and the caller's `3492` return was traced and composed window
by window from three different native runs (`0165 → 13C6` in run 037, `13C6 → 0747` in run 030, `0747 → 3492` in run 034),
so every cross-window statement carried a three-run bound. This note records one fresh native invocation that covers all
three windows: one reviewed controller session, one Up key, breakpoints only at the four stops, **one armed `LOGL` log of
26,378 lines**. Per-window byte accounts, a C-model composition and a comparison with the filed windows follow. It is
bounded evidence for that run only. It claims no complete one-Up frame, no same-invocation C route, and no function count
or credit; **no parity, function identity or credit follows from any re-check here**. Media, RAM images and traces stay in
ignored `resources/`; this note holds hashes, addresses and numbers only. No `src/` or `include/` file changes.

## The single invocation was feasible

Recipe of the earlier runs (reviewed controller under `poolrad-heavy.sh`, `TERM=xterm-256color`, screenshot-gated keys, the
pinned debugger build `d85a8563…`), breakpoints only at `10D2:0165`, `10D2:13C6`, `2E60:0747` and `3582:3492`, one Up key.
At the `0165` stop the ten-segment RAM image and CPU14 were captured and `LOGL 10000` (hex, up to 65,536 lines) was issued,
then `RUN`. The point that decides feasibility: **the armed log survives breakpoint stops.** At the `13C6` stop the file
held 766 lines; after one more `RUN` it held 791 lines whose first 766 were byte-identical; at `3492` it held 26,378.
Every stop appears as a duplicated pair of lines (rows 0/1, 765/766, 790/791; the second is the executed instruction), and
the last row is the `3492` stop state. Cleanup: an identity-checked `TERM` of the recorded controller (outer and observer
rc 0, inner 143, `post-audit.txt` present, nothing left running). Capture manifest `6e83c309…` (25 entries, all verify).

| Run-038 file (ignored `resources/oracle-runs/t4492-slot9-chain-038/`) | SHA-256 |
| --- | --- |
| `LOGL-CHAIN-0165-to-3492.TXT` (26,378 lines, 8,467,338 B, 9.460 ms) | `82fa7c72…` |
| `RAM-10D2-0165.BIN` / `RAM-10D2-13C6.BIN` | `a2c94c58…` / `66c64af4…` |
| `RAM-2E60-0747.BIN` / `RAM-3582-3492.BIN` | `ad337c93…` / `59ac71b7…` |
| `CPU-10D2-0165.TXT` / `CPU-10D2-13C6.TXT` | `bb164902…` / `baef3d82…` |
| `CPU-2E60-0747.TXT` / `CPU-3582-3492.TXT` | `9a27b7ec…` / `ccbac0c0…` (identical to the run-025/034 file) |

Host time of day this run: hour 0 (`SI=0000`), against hour 14 (run 037), 8 (run 030) and 15 (run 034).

## Per-window account against this run's own images (`tests/replay/audit_slot_a_chain_single_038.py`)

The audit replays every store of the whole log (capstone, 11,238 byte stores) over the native `0165` image and takes a
shadow snapshot at each stop. Declared host writers, as in the earlier notes: the DOS `INT 21h` callback (nine saved
registers, PSP stack pointer, and for `AH=25h` the vector write, `AH=42h`/`3Fh` the seek and read of `GAME.OVR`), the BIOS
tick, and the one hardware IRQ0 frame; `INT 1Ch` and the idle callbacks store-free. 8,308 trace read annotations match the
shadow memory (0 mismatches); 1,120 display stores are counted, not compared (the display rule as before).

| Window | Executed lines | Native delta | Explained | Unattributed | Replayed image vs native at the stop |
| --- | ---: | ---: | ---: | ---: | --- |
| `0165 → 13C6` | 764 | 52 | **52** (game 52) | **0** | 0 mismatches |
| `13C6 → 0747` | 24 | 25 | **25** (DOS callback 19, game 6) | **0** | 0 mismatches |
| `0747 → 3492` | 25,586 | 6,163 | **6,163** (DOS read data 5,503, game 631, DOS callback 19, ROM stub 9, BIOS tick 1) | **0** | 0 mismatches |

The end-to-end `0165 → 3492` delta is 6,203 bytes (the windows overlap). The declared DOS events in the chain are the vector
write `INT 08h <- F000:FEA5`, then the seek to `0x23DCC` with reads of 3,685, 88 bytes, and the seek to `0x2E0CE` with reads of
1,989, 100 bytes, as in run 034.

## C-model composition from this run's own `0165` image

From **this run's** native `0165` RAM image and CPU transcription (no trace entry state), `dispatch → 17AE → dispatch → 1742
→ dispatch → 13B1` runs under the declared environment (the `AH=25h` vector service, `IN 61h = 0x30` read from this run's
trace, the ordered OUTs) and the dispatcher returns to `2E60:0747`: the composed image at the `INT` (frame restored) equals this
run's native `13C6` (**0 differing bytes**), the composed RAM at `0747` equals this run's native `0747` (**0 differing bytes**),
the final CPU equals the transcription in all 14 fields, and the OUT sequence equals the trace. The three-run bound is
therefore gone for the first two windows: they are composed and checked inside one invocation. The DOS binding remains a
declared hypothesis supported by byte equality; the C models are unverified beyond this chain (their own audits are separate
and were not re-run here). The third window is accounted by store replay only: the C-model span composition of `0747 → 3492`
(PR #1968) is not merged and was not repeated.

## Each window against the separate-window run it repeats

| Window vs filed run | Delta here / filed | Changed only in the filed run | Changed only here | Differing final values |
| --- | --- | --- | --- | --- |
| `0165 → 13C6` vs run 037 | 52 / 54 | 2: `push ax`, `push di`, same final value (the stale starting value coincides here) | 0 | 1: `push si` (host hour `0000` here, `000E` there) |
| `13C6 → 0747` vs run 030 | 25 / 26 | 1: the saved `SI` of the DOS callback (hour `0000` here, `0008` there) | 0 | 0 |
| `0747 → 3492` vs run 034 | 6,163 / 6,164 | 2: IRQ0 handler `push ax` (ROM stub) and a game `push ax` with the same final value | 1: IRQ0 handler `push ax` (ROM stub) | 3: the BIOS tick, the handler's `INT 1Ch` frame, its `push dx` |

Every byte changed in only one of the runs has a replayed writer here (0 without), the causes are pinned exactly in the audit,
and each is host-time or interrupt-landing noise: the hour in `SI`, the tick word, the BIOS IRQ0 handler's own pushes and frame,
or a stale starting value that happens to equal the stored one. The remaining changed bytes (52, 25 and 6,162 respectively) are at
identical addresses in both runs, with identical final values except the differing bytes named in the table.

## Mutation probes

Twelve mutants of the audit's branches (`tests/replay/check_slot_a_chain_single_038.sh`): eleven are killed (the stop rows executed,
the `AH=25h` vector write, the saved-register order, the DOS read position and a skipped read, the tick, a composition entry
`DS`, the old synthetic `IN 61h = 03`, the composition's binding skipped, and two cross-run pins). One survives and is
equivalent: dropping the hardware IRQ0 frame stores (all its bytes are overwritten before `3492`, as in the earlier note).

## Limits

CPU files are manual `EV` transcriptions; stop receipts are the debugger's breakpoint display only; the debugger halts the guest
at **every** stop (four in this chain) and `LOGL`'s effect on timing is not excluded, so the IRQ0 landing and the tick in this
run include those pauses; the upper 384 KiB was never observed; host-time noise differs between runs by construction (four
distinct hours across the four runs); the DOS binding is a hypothesis; the window `0747 → 3492` has one executed line fewer than
run 034 (25,586 against 25,587), not analysed beyond the exact byte account; this is a fourth native run touching these windows, not
a statement about other saves or other keys.

## Next step

Compose the `0747 → 3492` spans with the C models inside this same invocation once the span composition (PR #1968) is merged,
which would make the whole chain model-composed in one run; and the open identity question for the four `08AE` models, which
no re-check here resolves.
