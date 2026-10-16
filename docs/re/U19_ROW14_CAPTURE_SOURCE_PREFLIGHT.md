# U19 row-14 selected-store capture source preflight

This local-only source check prepares the next bounded same-run selected
`A000:118F` store capture. Heavy native is held. No new CPU, RAM, EGA-plane,
or RGB capture exists, and no first writer, page selection, C parity, or
full-frame credit follows.

Before this check, bounded `deja "Poolrad U19 row14 selected store same-run
capture host RGB page return source join"` found no indexed matching session.
The merged [candidate order](COLD_START_CODEWORD_ROW14_STORE_CANDIDATES.md),
[target filter](U19_ROW14_TARGET_FILTER_PREFLIGHT.md),
[eight-stop route](U19_ROW14_SELECTED_STOP_ROUTE.md), and
[offline join](U19_ROW14_SELECTED_STORE_JOIN.md) supply the DOS addresses,
128-hit cap, and private manifest boundaries. The new check independently
pins the previously cited debugger implementation, now present under
ignored `resources/` in this isolated checkout.

## Pinned debugger behavior

The private source revision is `5a9c7c53481c2e933948039c33256776653f4faf`.
`src/debug/debug.cpp` hashes to
`2762251108e54aeec3474b9f495222ce09bec9f246dd0af53a4d10fa7bb2a38d`;
the heavy executable hashes to
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.
The audit checks six exact source windows:

| Source lines | Bytes | SHA-256 | Capture implication |
| --- | ---: | --- | --- |
| 682–701 | 599 | `669b8a12166f92d6be2164c1dd980a041a3bae2741a3b6fac651220c77abe4b8` | Memory watch seeds from one guest-readable byte when armed. |
| 729–793 | 2166 | `c9cba40ea45854a6d3323e4c63c0261042d1f570fe2fa6446cf99133de4e9484` | Later checks compare that one byte; a BPM stop is not a four-plane write record. |
| 1994–2000 | 276 | `234edc0cdd3ce35aa7512598bef4dbd5d6f2080d68bbdbe4ae396c9874504351` | `MEMDUMPBIN` parses segment, offset, and length. |
| 2505–2522 | 606 | `acd91f4150f20159f134b787ab8751e1e43d25b2ffc833c2ce3fbfebc48e285e` | `BP` and heavy-only `BPM` are distinct debugger commands. |
| 3833–3839 | 220 | `8f9b1f26da533494a7514509bb7b206fe1467c06860957e179596de8cc6391ed` | `OUTP` writes an I/O byte, so plane-read map and selector restoration are required. |
| 5794–5811 | 453 | `636ec61547ed2f23811922e8c8bc50005d4eebd09df050d4a08088b1deb36f52` | Binary dumps overwrite `MEMDUMP.BIN` and substitute zero for a failed guest read. |

The dump loop writes the requested byte count even when individual reads
fail. Therefore a 640-KiB file, its SHA-256, or the debugger's success
message alone cannot certify every byte in it. The merged route join checks
the loaded resident/stub/U19/printer code in a supplied pre-call RAM image;
that check does not validate unrelated RAM bytes. Any failed or unknown
read coverage is a **typed dump-integrity blocker** for claims that require
those bytes. Preserve the raw debugger log, and independently inspect the
specific code, page words, and caller-frame locations used by each later
claim.

## Bounded next capture, only after a native slot is authorized

1. Pin this executable and source check and rerun the merged original-byte
   [candidate audit](COLD_START_CODEWORD_ROW14_STORE_CANDIDATES.md). Start
   from the verified credits frame with one Return and derive loaded
   segments from the actual resident call and patched stub. A changed U19
   or printer segment stops the current fixed-segment CPU filter.
2. Record the first U19 `+02D8` CPU/stack and full RAM before entering the
   printer; move each `MEMDUMP.BIN` to a unique ignored name before another
   dump. Verify the return frame at the printer entry `+0626`. Keep the
   complete bounded candidate-hit log, including no-change stores.
3. Treat `BPM A000:118F` as a one-byte hint. Use the four pinned drawer
   instruction stops `+0412/+0438/+0445/+0451` and filter for effective
   `ES:DI=A000:118F`. At the selected instruction, preserve full CPU/RAM,
   Graphics Controller and Sequencer state and four independent plane
   windows; single-step exactly one instruction to its pinned successor
   and repeat. Restore and verify the original controller selector and
   read map after every plane read. Abort if restoration or the direct
   successor is not measured.
4. Continue to the first U19 `+02DD` return and keep its own CPU/RAM. Run
   the merged CPU/EGA event and eight-stop route joins on this one private
   run. These are structural checks; the raw stop transcript and debugger
   breakpoint coverage still require independent review before a first
   value-changing guest writer can be claimed.

The host RGB/page correlation remains a disjoint follow-on. Even an
observed four-plane change does not identify the host-presented page or
the time the changed pixel reached the screen.

Read-only local preflight, without launching DOSBox-X:

```sh
python3 tools/replay/cold_start_codeword_row14_capture_source_audit.py \
  --source resources/dosbox-x-16a1-private-src \
  --heavy resources/dosbox-x-heavy-src/src/dosbox-x
```

All private source, debugger binaries, game media, dumps, and pixels stay
under ignored `resources/`. This note publishes addresses, hashes, counts,
and capture limits only.
