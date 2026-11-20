# Slot-A Up: store replay of the `2E60:0747 → 3582:3492` window (run 034)

This note extends the run-030 store replay ([SLOT_A_MEASURED_INPUTS_030.md](SLOT_A_MEASURED_INPUTS_030.md))
from the 131-byte `00F4 → 0165` window to the 6,164-byte `0747 → 3492` window of one fresh native run,
and gives a per-byte account of the native delta. It is bounded evidence for that window only. It claims
no complete one-Up frame, no same-invocation C route, and no function count or credit. Media, RAM images
and traces stay in ignored `resources/`; this note holds hashes, addresses and numbers only.

## Pins (ignored `resources/oracle-runs/t4492-slot6-window-034/`)

| File | SHA-256 |
| --- | --- |
| `LOGL-WIN-0747-to-3492.TXT` (25,589 lines, 8,214,069 B) | `0cbc4716…` |
| `RAM-2E60-0747.BIN` | `f9b539f0…` |
| `RAM-3582-3492.BIN` | `ed613cd5…` |
| `CPU-2E60-0747.TXT` / `CPU-3582-3492.TXT` | `44c10f70…` / `ccbac0c0…` |
| `GAME.OVR` (206,105 B) | `872964a3…` |

The audit (`tests/replay/audit_slot_a_window_store_replay_034.py`) locks the full digests of these files.
The optional Run B control also locks the capture manifests of `t4492-slot7-images-035` (`c7fedbac…`)
and `t4492-slot7-fill-036` (`82ed962a…`) and checks each image it uses against its manifest entry.

## Result

The first log line repeats the `0747` breakpoint instruction and the last is the `3492` stop state, so the
replay executes lines 2 to 25,588 (25,587 lines) over the native `0747` image. Stores are decoded from the
logged instruction bytes with capstone and addressed with the logged registers: 9,863 store events, of which
1,120 target display memory (`A000`) and are treated by the display rule below.

| Per-byte account of the 6,164 native delta bytes | Bytes |
| --- | --- |
| replayed writer exists and the replayed value equals native | 6,164 |
| writer exists, value differs from native | 0 |
| native changed, no replayed writer (**unattributed**) | **0** |
| replay changes a byte native left unchanged (spurious) | 0 |
| written by a replayed store but equal in both images (benign) | 200 |

The predicted image equals native `3492` in all 655,360 bytes. By last writer class the 6,164 delta bytes
are: DOS read data 5,503, game code 632, ROM stubs 9, DOS callback register save and PSP pointer 19 (18 + 1)
and BIOS tick 1. By writing opcode, the 641 game-code and ROM-stub bytes are `add` 193, `stos` 180, `push` 79,
`rep stos` 66, `lcall` 36, `mov` 35, `rep movs` 25, `int` 11, `inc` 9, `call` 6 and `xchg` 1.

## Bindings: measured or hypothesis

| Binding | Status | Support / limit |
| --- | --- | --- |
| rep string ops: one trace line covers a chunk of iterations, count = CX change to the next line | measured | all 165 `repe` lines (133 `stosb`, 32 `movsb`, 569 iterations); 161 follow `min(CX,4)`, 4 are shorter (2, 2, 2 and 3 iterations); DI (and SI) progress by exactly the counted iterations on every line, so DF = 0 is checked, not assumed |
| non-rep `stosb`/`stosw` 59 + 72, `xchg` 16 (12 register-register, 4 with memory) | measured | decoded from logged bytes and registers |
| hardware IRQ0 frame at `F000:FEA5` | derived | return CS:IP from the line after the matching `iret`; flags as logged with IF set. All its bytes are overwritten before `3492` except 5 that game code also writes, so the frame is **not observable** in this window (omitting it changes nothing) |
| BIOS IRQ0 clock callback: tick dword + 1 | measured | the two anchors' tick words differ by exactly 1; one `callback 0008`, one `F000:FEA5` entry |
| DOS INT 21h callback: nine saved registers below the interrupt frame, PSP stack pointer | hypothesis | the run003/030 binding; six calls, DI = `0824` (the relocation constant) measured in the log at every call; supported only by byte equality |
| DOS seek (AH 42h) and read (AH 3Fh): read copies CX bytes of `GAME.OVR` from the seeked position to DS:DX | hypothesis | positions `0x23DCC` and `0x2E0CE` and lengths 3,685 / 88 / 1,989 / 100 are read from the log registers; destinations `2F6B:0000`, `3052:0000` (twice) and `30CF:0000`; handle 5 is assumed to be `GAME.OVR`; supported by byte equality of 5,503 delta bytes |
| INT 1Ch callback (1) and idle callback (164): no stores | hypothesis | registers unchanged across 161 of them; the other 4 idle callbacks set AX to the byte count of the preceding DOS read (the delayed read completion, a register effect); their internals are not instruction-traced |
| 452 display OUTs (ports 3C4: 14, 3C5: 398, 3CE: 20, 3CF: 20) and one EOI (port 20) | hypothesis | no effect on the 640 KiB image |
| stores at or above `0xA0000` | rule | 1,120 stores into `A000` are counted, **not compared, and given no bytes**; 768 trace reads of display memory are unobserved and the audit fails if any such read feeds a store into the 640 KiB image (0 do: all feed display stores) |

## Controls

* **Trace read annotations.** 8,816 lines carry a `seg:[off]=value` memory-operand value measured by the
  emulator. The 8,048 outside display memory all agree with the replayed shadow memory (0 mismatches); this
  checks the replay at every instant of the window, not only at the end, including the DOS-loaded overlay
  bytes and the stack.
* **Omission controls** (bytes left unexplained when a writer class or kind is dropped, by last writer):
  DOS read 5,503, game 632, DOS callback 19, ROM stub 9, BIOS tick 1; by kind `rep-stos` 66, `rep-movs` 25,
  `stos` 180, `xchg` 1, `dos-regsave` 18, `dos-psp` 1 and the others in the audit output. The hardware frame
  leaves 0 (dead stores, stated above).
* **Run B intermediate stops.** 20 stops of a different native run (images only) lie inside the window; the
  replayed prefix (Run A `0747` plus the stores up to the stop row) is compared with each Run B image. Bytes
  where Run B's own `0747` image already differs from Run A's (75 to 87, the run-to-run noise) are excluded.
  Outside that noise the mismatch is 0 at 6 stops, 1 to 2 bytes at `028F#1`–`03D5` (a stack byte written by
  one `push` whose pushed register differs by one between runs, and its copy in the first DOS register save) and
  20 bytes from `0BF4:002F#1` on (the IRQ0 landed on a different instruction in Run B, so its stack frame and the
  BIOS handler pushes are elsewhere). The audit requires every such byte to lie in the stack segment `1D16`
  (0 elsewhere); this is a loose bound and it does not prove the stack bytes right. In Run A the replay
  matches the native `3492` stack bytes exactly.
* **Reuse of earlier controls.** The tick difference (1) and the single `F000:FEA5` / IRQ0 callback / INT 1Ch
  callback match the run-027/030 structure (tick +1 per BIOS chain), without the 26-entry hook of the earlier window.

## Mutation probes

Sixteen mutants of the new branches (`tests/replay/check_slot_a_window_store_replay_034.sh`): 15 are killed
(applying the stop row, register-save order, PSP pointer, seek position (high word), read destination segment,
read position, short read, skipped read, tick + 2, skipped tick, string-move source segment, `stos` width,
skipped `xchg`, display stores wrapped into low memory, and the `min(CX,4)` chunk count). The `rep-chunk4` mutant
is killed by the DI-progression check rather than by an image difference. One survives and is equivalent in this
window: dropping the hardware IRQ0 frame stores (all its bytes are dead, see Bindings).

## Limits and missing inputs

* Display memory (`0xA0000` and above) was never captured; the 640 KiB dumps cannot show it. The planar EGA
  state behind 452 port writes is not modelled, so nothing is claimed about what the 1,120 display stores did.
* The DOS read and the INT 21h register-save bindings are hypotheses supported by equality, not read from
  callback code; the idle and INT 1Ch callbacks are declared store-free from register evidence only.
* Timing: the IRQ0 landing point is run-specific (Run B differs); this replay is anchored to the Run A log only.
* CPU files are manual `EV` transcriptions; the `FLG` column is lazy (flags rebuilt from the per-flag columns) and
  DF is not logged; `LOGL`'s effect on timing is not excluded.

## Next integration step

Turn the account into an executable oracle for the C composition: model the DOS seek/read service (reads from the
original `GAME.OVR`), the BIOS tick and the display sink as declared services of the hybrid/replay harness, run the
existing unit models for the window (unit `0x3BD` entries, unit `0x7D` entry 1 loader, `0xBA:0x290B`, the two
`0x464:0x2595` releases), and diff the model's RAM against this predicted image with the same writer-class
tables, so each remaining difference is a named writer, not a count.
