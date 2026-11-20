# Slot-A Up: C composition of the `2E60:0747 → 3582:3492` window against native 3492 (run 034)

This note takes the per-byte account of [SLOT_A_WINDOW_STORE_REPLAY_034.md](SLOT_A_WINDOW_STORE_REPLAY_034.md)
and replaces instruction replay by the existing raw-CPU C models wherever one exists, run over the
composed RAM, then diffs the composed 640 KiB image with the native `3492` image. It is bounded evidence
for that window only. It claims no complete one-Up frame, no same-invocation C route, and no function count
or credit. Media, RAM images and traces stay in ignored `resources/`; this note holds hashes, addresses and
numbers only.

**What is composed and what is declared.** Control flow and every model's entry state come from the
hash-locked W1 `LOGL` trace (declared: this is trace-driven composition, not a C-driven route). Each
modeled span is run through its C model over the composed RAM and checked against the trace; everything
else (spans without a model, the host DOS/BIOS callbacks, the IRQ0 landing inside a modeled span) is
applied from the store-replay journal, which is log-derived and therefore declared, not modeled.

## Pins

The audit (`tests/replay/audit_slot_a_window_compose_034.py`) locks the run-034 files of the store-replay
note (log `0cbc4716…`, `RAM-2E60-0747.BIN` `f9b539f0…`, `RAM-3582-3492.BIN` `ed613cd5…`, CPU files
`44c10f70…` / `ccbac0c0…`), `GAME.OVR` `872964a3…`, and the Run B manifests (`c7fedbac…`, `82ed962a…`).
The models are built from the sources the per-function audits already use (no new model code).

## Verified spans

Nine spans, 21,146 of the 25,587 executed lines (83%), run through the C models from the trace's entry
state; each reproduced the trace: exit registers equal, OUT sequence equal, in-order display (A000) write
sequence equal, and RAM below `0xA0000` equal to the log shadow.

| Span (loaded address) | Model | Trace rows |
| --- | --- | ---: |
| `2F6B:0000` unit 0x7D list/timer | `pr_u7d_list_timer_0000_cpu` | 876 |
| `08DE:1073` ×2 word string | `pr_res_00ba_1073_cpu` | 166, 186 |
| `08DE:1037` ×2 byte string | `pr_res_00ba_1037_cpu` | 183, 171 |
| `0C15:0137` clear (two rect fills inside) | `pr_r03f1_clear_0137_cpu` | 871 |
| `0F2D:0626` printer (twelve glyph calls inside) | `pr_r0709_printer_0626_cpu` | 18,671 |
| `0C88:2595` ×2 bitmap release (null-pointer early return here) | `pr_r0464_bitmap_release_2595_cpu` | 11, 11 |

Display: 352 + 768 = 1,120 `A000` writes, compared in order and by value with the models' write callbacks
(previously only counted); the 452 display OUTs (44 + 408) match the models' OUT sequences. The planar plane
contents are **not** claimed: the initial EGA planes were never captured, so only the CPU-level writes are
compared. The single EOI `OUT 20h` lies in the IRQ0 interval and stays a journal effect.

## Per-byte account

| Composed 640 KiB image vs | Result |
| --- | --- |
| native 3492 (6,164 delta bytes) | explained 6,164; **unattributed 0**; extra (composed changes an unchanged byte) 0 |
| store-replay prediction | 0 differing bytes (same image) |

Delta bytes by last-writing source: C models 172 (`u7d` 84, printer 46, clear 37, `1037` 5); journal 5,992
(DOS read data 5,503, game code in unmodeled spans 459, DOS callback 20, ROM stub 9, BIOS tick 1). The
printer span's 10 RAM differences from the log shadow are exactly the IRQ0 interval's stores (tick, frame
and BIOS-handler pushes), which the model cannot contain; the audit merges them in where no later in-span
store overwrote them (declared rule).

## Declared exclusions (not claims)

* **20 Run B delta bytes.** Against Run B's final `3492` image 6,144 of the 6,164 bytes are independently
  supported. The other 20: 1 BIOS-tick, 2 DOS-callback and 10 game-code bytes that lie in the run-to-run
  noise set (Run B's own `0747` image already differs there) and 7 ROM-stub bytes Run B contradicts (its IRQ0
  landed on a different instruction). Of the 13 delta bytes in the noise set only the tick also differs in
  Run B's final image; the other 12 match Run B's final value but are not independent support, because their
  inputs already differ between the runs. Eight bytes (the tick and the 7 ROM-stub bytes) have a different
  final value in Run B. The audit pins these groups exactly.
* **Stack bound.** The Run B stop control now uses the measured SP range of the window, `SS:3A42–3EF6`
  (1,204 bytes), not the whole `1D16` segment: all 20 intermediate stops have 0 mismatch bytes outside noise
  and outside that range; 19 of the 20 exclusions sit inside it (the tick is not a stack byte). The bound is
  checked against the trace (`SS` constant, SP min/max over 25,589 lines) and a mutant that tightens it is killed.
* **Display memory** is unmodeled: 1,120 `A000` stores compared at the CPU-write level only; the trace's 768
  old-value annotations on display stores are not reads. The upper 384 KiB was never observed.
* **Host callbacks as hypotheses supported by byte equality:** DOS INT 21h (register save, PSP pointer, seek and
  read of `GAME.OVR`), INT 1Ch, 164 idle callbacks, BIOS IRQ0 tick. Not instruction-traced, not modeled in C here.
* **Run-specific IRQ0 landing** (Run A only), manual CPU transcriptions, breakpoint-only receipts, halted guest.
* There is no `13EA` hook entry in this window: IRQ0 enters the BIOS handler at `F000:FEA5` directly.

## Mutation probes

Seven mutants of the new branches (`tests/replay/check_slot_a_window_compose_034.sh`): six are killed
(entry SP perturbed, IRQ0 merge dropped, stack bound tightened, model RAM not applied, models run over a stale
image, fallback journal dropped for a row range). One survives and is equivalent: perturbing AX at every span
entry (AX is dead input for all nine models in this window).

## Exact missing inputs (what stops the composition from being model-only)

4,441 executed lines have no C model in this composition: host `F000` DOS/BIOS callback code 2,150; resident
loader `12C6` 1,286; unit 0x7D rest (`2F6B:02F2` carry and others) 369; system string/runtime `131C` 263
(`16A8` ×3 and others); `08DE` 157 (the `290B` dispatcher's own lines); unit 0x3BD `2D14` 112; unit 0x68 entry
1 body `2E60` 73; `3052` 25; stub lines 6. Specifically:

1. **Loader relocation path.** Models exist only to boundaries (`+0268` to the seek INT 21h, the `028F`/`029F`
   service-driven stages, `+0553`, `+0538`, `+0338` to its first call). Nothing models `12C6:0435`, `04E6`,
   `046B`, `03E9`, `0506`, `0512` or `02E5`, which carry the `0x0824` fixup application and most of the loader's
   459 game-class delta bytes; the loader's DOS read services need a C-side `GAME.OVR` binding.
2. **State-struct models not wired to RAM:** unit 0x68 entry-1 body, `2F6B:02F2`, unit 0x3BD in-grid `031F`, and
   the `08DE:290B` I/O model (only its inner raw-CPU spans are verified).
3. **Host services** for DOS/BIOS in C, and an EGA initial-plane capture, if display contents are ever to be claimed.
4. A model-driven route (control flow from the C entry, not the trace) needs all of the above; none of that exists yet.

## Next step

Close the loader: add raw-CPU models for the unmodeled `12C6` relocation pieces, then run the loader's
service-driven stages with a `GAME.OVR` read binding so the 0x0824 fixup stores and the 5,503 DOS-read bytes
come from C, leaving only the host callbacks as declared hypotheses.
