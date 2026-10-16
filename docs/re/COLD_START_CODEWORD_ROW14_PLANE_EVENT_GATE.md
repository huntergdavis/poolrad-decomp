# U19 row-14 paired four-plane capture gate

This is a **preflight** for the next bounded native slot. No new DOS run or
EGA plane capture exists here. The merged
[four-plane watch](COLD_START_CODEWORD_ROW14_PLANE_WATCH.md),
[candidate order](COLD_START_CODEWORD_ROW14_STORE_CANDIDATES.md), and
[page-selector boundary](COLD_START_CODEWORD_ROW14_PAGE_SELECTOR.md) give the
loaded caller, witness, and four store PCs. The new offline checker enforces
paired instruction stops and four distinct plane windows before it reports
the first **observed** change. It never promotes that report to a first
actual guest writer without native breakpoint coverage.

Before this preflight, bounded deja recall for the U19 A000:118F synchronized
four-plane watch timed out after 20 seconds. The cited merged notes were
reused as leads; their source addresses and captured-state limits remain
separate from the future measured claim.

## Serial native slot contract

Use the already retained cold-start route, with no overlapping heavy run.
At live U19 38D3:02D8, capture CPU, full caller stack and RAM, display
words DS:4A16..4A1E, Graphics Controller selector/read map, Sequencer map
mask, canonical host RGB, and four A000:[1180,12C0) plane windows. Keep
the debugger output and all payloads under ignored resources/.

After the caller stop, arm memory-change BPM A000:118F as a route hint.
Filter source instruction stops 0F2D:0412, :0438, :0445, and :0451 to
effective ES:DI = A000:118F; do not stop through the whole string. The
pinned instruction successors are respectively :0415, :043B, :0449, and
:0455. At a reached witness store, preserve a **before** sample, single-step
that one instruction, and preserve an **after** sample at its successor.
Capture all four read-map planes, not only the selected map. Record
registers, sequence number, active/draw page words, plane-loop counter,
Sequencer map mask, and return/caller frame. Cap the serial slot at the
first paired value change or a small agreed stop budget; a missed or
unmapped writer is a typed blocker.

For each four-plane sample, read and record the original Graphics Controller
selector/read map. Select register 4 with OUTP 3CE 04, use OUTP 3CF 00
through 03, and copy each MEMDUMPBIN A000 1180 140 result to a distinct
private file. Restore the original read map **and selector** before
resuming. Verify these two controller values after restoration; abort the
route if they differ.

The ignored heavy DOSBox-X source pins the debugger's BPM initialization
to the guest byte at arm time and its later comparison to one guest-readable
byte (debug.cpp SHA-256
2762251108e54aeec3474b9f495222ce09bec9f246dd0af53a4d10fa7bb2a38d,
AddMemBreakpoint and CheckBreakpoint). That guest byte follows the selected
Graphics Controller read map, so BPM cannot establish the first change in
*all four* planes. Its older README labels BPM differently; the source
implementation is the contract used here.

The private EGA write path's unchained
VGA_Generic_Write_Handler in vga_memory.cpp (SHA-256
f506017948503fa22ce40715205293c987473a938247cbd09ba409ffbb652c14)
computes pixels.d from the prior planar word and writes the four-plane
word immediately afterward. A bounded private watch inserted around that
assignment, conditional on the *resolved* plane address 0x118F and the
saved EGA mode, could record all four before/after bytes and CPU PC without
changing the guest's Graphics Controller read map. This is a source
candidate, not a measured hook: first verify the saved route selects this
unchained handler and that the resolved plane address denotes A000:118F.
No emulator source was changed or launched for this preflight.

The private JSON manifest for the offline checker has version 1,
window_start 4480, window_bytes 320, witness_offset 4495, and an ordered
events array. Each event records a 1-based sequence, candidate pc and
next_pc, plus before/after objects. Each object names one 640-KiB RAM file,
four distinct 320-byte plane files in plane order, and two [selector,
read-map] pairs gc_before/gc_after. All paths are relative to one ignored
run directory. The checker rejects truncated planes, missing or reordered
pairs, wrong instruction successor, changed page-segment route, un-restored
controller state, and an unaccounted A000:118F byte change between pairs.
It reports byte changes separately from the 0x20 witness pixel-bit change.
It cannot certify that the debugger observed *every* write; retain the
ordered debugger log and before/after CPU frames for PM review.

After the printer returns at U19 +02DD, repeat the four-plane/host snapshot
at +02E2, +02FD, and +0302. A same-run page-helper call/return and host
presentation correlation is the separate B gate. The existing C
printer-page model can be instantiated for [1180,12C0) once a private
pre-printer plane page is captured; that C output is a prediction until it
is compared with the immediate native post-printer planes.

Reproduce the synthetic gate without original media:

    python3 -m unittest tests.replay.test_cold_start_codeword_row14_plane_event_audit

After an approved native slot, analyze only private files:

    python3 tools/replay/cold_start_codeword_row14_plane_event_audit.py \
      --run resources/oracle-runs/PRIVATE-RUN \
      --manifest resources/oracle-runs/PRIVATE-RUN/plane-events.json

No first actual writer, page flip, host timing, C parity, full-frame or
identity credit is asserted by this preflight.
