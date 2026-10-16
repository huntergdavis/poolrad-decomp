# U19 row-14 selected-plane capture integrity

This source-only gate sharpens the merged [same-run capture preflight](U19_ROW14_CAPTURE_SOURCE_PREFLIGHT.md) for selected `A000:118F`. It does not supply a new native stop or plane byte. The [eight-stop join](U19_ROW14_SELECTED_STORE_JOIN.md) and [paired-plane audit](COLD_START_CODEWORD_ROW14_PLANE_EVENT_GATE.md) remain separate checks. Heavy native is held.

Before this work, bounded `deja "Poolrad U19 row14 selected A000 118F four-plane GC Seq selector restoration page CRTC retrace RGB same-run capture"` found no matching indexed session. The merged PR #709 preflight supplied the debugger SHA, its `MEMDUMPBIN` failed-read caveat, and the selected store route; this note adds the controller and read-path source gate. The ignored DOSBox-X source revision is `5a9c7c53481c2e933948039c33256776653f4faf`, and its heavy executable SHA-256 is `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

## Source-backed capture conditions

The new read-only audit pins five complete private source files and twelve line windows. These are the decisive windows:

| Private source | Lines | SHA-256 | Consequence |
| --- | ---: | --- | --- |
| `debug/debug.cpp` | 3327–3336, 3375–3384 | `4a42b9a847ec186d71443ea530daf6a1f0d8287a8c8a17e875e8ada21c8b25f8`, `153e4a23edb61b14d841d5cc03731f4619d9371f842e48da91310fb0d0e684c6` | `GC` prints the host's live index, read map, mode and miscellaneous register; `SEQ` prints its live index, map mask and memory mode. |
| `debug/debug.cpp` | 3812–3841, 5794–5811 | `ae2eee9af3e0f384feb141792e79c53ab8058d9af94b97d91df6e50960183182`, `636ec61547ed2f23811922e8c8bc50005d4eebd09df050d4a08088b1deb36f52` | Debugger `OUTP` writes the emulated port. `MEMDUMPBIN` replaces any failed byte read with zero while still writing the requested length. |
| `hardware/vga_gfx.cpp` | 98–115, 218–258 | `652932b3092b8f732445fb14c9d548814a0c9694a85c8951d67631627f17d33f`, `1d48c0c3e302fce19f9e1cea6ac4d7ce76f4b2a6868d81d20dc1116088706ba9` | GC index 4 controls read-map selection; port read handlers are registered only for `IS_VGA_ARCH`. |
| `hardware/vga_seq.cpp` | 172–181, 240–277 | `ef3fe3d90eacc0940cbaf783e92789fb5ea585215fcf90ea618f78523410a13e`, `38878099b7a9c110cf448d88f6fa893318310be48cc1c01ba72676974c558ceb` | Sequencer index 2 controls the write map mask; port reads are likewise registered only for `IS_VGA_ARCH`. |
| `hardware/vga_memory.cpp` | 145–183, 282–330, 580–594 | `e8afbb45e2e12a44b0b7c29d9ee6d7ce6349f24a9ffbe0f0241011471fdece35`, `96f25917754d8fa9ceaf3a994c89f7eb565e69c6ee3d62473747faa5310856eb`, `a5d1c8b980152a9381add18663001f28ce6f72cf23c7aaa82bfbf87ce0281795` | The unchained read handler passes the selected map to the generic read. Read mode 1 returns a color comparison instead of that plane's byte; odd/even configuration can remap the selected plane/address. Reads load `vga.latch`; write operations can consume that latch. |

The pinned `include/dosbox.h:200–207` window defines `IS_VGA_ARCH` as `MCH_VGA`, distinct from the EGA/VGA predicate. A BIOS mode-0D word does not establish the emulator's machine architecture. Therefore debugger `INP 3CE/3CF/3C4/3C5` is **not** a source-backed controller readback method when the machine could be `MCH_EGA`; the `GC` and `SEQ` commands read host state directly. The direct-plane interpretation also needs GC mode bit 3 clear (read mode 0), Sequencer memory-mode bit 2 set (odd/even addressing disabled), GC miscellaneous bit 1 clear, and confirmation of the unchained A000 handler. If any condition is absent or unmeasured, a map-selected binary dump is a typed **plane-interpretation blocker**.

## Restoration and integrity sequence for an authorized slot

At the reached `38D3:02D8` pause and at the first selected drawer instruction (`0F2D:0412/:0438/:0445/:0451`, effective `ES:DI=A000:118F`), save the CPU/stack, complete RAM, raw debugger log, and the unmodified `GC` and `SEQ` command outputs. The latter supply the original GC index/read map and Sequencer index/map mask/memory mode without changing either selector. Check the direct-plane conditions above before labeling a file as a plane.

For a diagnostic debugger dump, select GC index 4 with `OUTP 3CE 04`; for each read map `00` through `03`, issue `OUTP 3CF nn`, record a `GC` state showing index 4 and the requested map, then issue `MEMDUMPBIN A000 1180 140`. Move the fixed `MEMDUMP.BIN` to a unique ignored filename before the next dump and record its length and digest. Restore the original map while GC index 4 is selected, then restore the original GC index. Record `GC` and `SEQ` again and require every reported field to match the baseline. Do not alter the Sequencer index or map mask merely to read `SEQ`; if either changed, stop and explain it.

**Do not resume the selected guest store on the strength of that restoration alone.** The generic A000 read loads `vga.latch.d`, and `ModeOperation` can use that latch on a subsequent write. The four 320-byte debugger dumps therefore perturb state that `GC` and `SEQ` do not report. For a before/after single-instruction parity gate, prefer a separately verified private host-internal four-plane snapshot at the resolved address, which does not invoke guest memory reads or port writes. Otherwise privately preserve and restore the latch too and prove the active write mode does not consume any unaccounted state. A missing latch proof is a **replay-perturbation blocker**. Keep raw CPU/controller/source logs and four separate plane windows at the exact before and successor stops in one ignored run.

Matching 320-byte lengths, digests, and `GC`/`SEQ` fields do **not** certify that `MEMDUMPBIN` successfully read every byte. The native slot needs a separately verified private read-failure counter or an internal four-plane snapshot at the resolved address. A failed or unknown read count is a **dump-integrity blocker**. The merged plane-event checker validates supplied filenames, page words, declared GC before/after equality, and pair order; those assertions cannot authenticate the debugger log or prove native breakpoint coverage. Its current declared GC index range ends at 8; if the live baseline index is 9–15, adjust that checker from evidence before using it, rather than mislabeling the valid host state.

## Deterministic inputs available now

The read-only audit verifies ignored `START.EXE` and `GAME.OVR`, the heavy executable/source, and the four retained 640-KiB post-call U19 RAM files (`+02DD/+02E2/+02FD/+0302`). They are earlier endpoints, not a same-run `+02D8` pre-call, selected store, or successor. This checkout has no `route.json`, `cpu-frames.json`, or `plane-events.json` in the retained row-14 directory. Its existing screenshots do not replace those files. No emulator was launched.

```sh
python3 tools/replay/cold_start_codeword_row14_plane_integrity_audit.py \
  --source resources/dosbox-x-16a1-private-src \
  --heavy resources/dosbox-x-heavy-src/src/dosbox-x \
  --game resources/DOS/POOLRAD \
  --run resources/oracle-runs/codeword-row14-p5-001
```

Only addresses, hashes, counts, and source conditions are public. Original media, full RAM, controller logs, EGA planes, and RGB remain under ignored `resources/`. No live writer, page/RGB, C, strict, or full-frame credit follows.
