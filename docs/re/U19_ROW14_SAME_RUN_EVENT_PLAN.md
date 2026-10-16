# U19 row-14 same-run CPU/EGA event plan

This source/tooling gate connects the merged
[strict CPU target filter](U19_ROW14_TARGET_FILTER_PREFLIGHT.md) to the
[paired four-plane event audit](COLD_START_CODEWORD_ROW14_PLANE_EVENT_GATE.md).
No emulator was launched and no new native frame or EGA plane capture exists.
The host native slot remains held. The result from synthetic input is an
offline consistency check, never a first-writer or page/RGB claim.

Before implementation, bounded deja queries for the U19 same-run CPU/EGA
capture and page-return witness found no indexed sessions. The merged
[candidate order](COLD_START_CODEWORD_ROW14_STORE_CANDIDATES.md),
[plane watch](COLD_START_CODEWORD_ROW14_PLANE_WATCH.md), and
[page-return handoff](COLD_START_CODEWORD_ROW14_PAGE_RETURN_RGB_HANDOFF.md)
supplied the hash-locked caller and store addresses and the capture limits.

## Ordered private capture contract

When a serial native slot is authorized, use a single run token and
monotonically numbered debugger stops:

1. Pin the ignored heavy debugger by SHA-256, then stop at live U19
   `38D3:02D8`. Retain CPU, full caller/stack frame, DS page words,
   Graphics Controller and Sequencer state, and a full RAM image before
   running the printer. The existing target-filter preflight rejects the
   system debugger binary. A `BPM A000:118F` event is only a route hint.
2. Bound the interval by U19 `38D3:02DD`. Within it, watch drawer
   `0F2D:0412/0438/0445/0451`; retain only an instruction stop whose
   effective ES:DI is `A000:118F`. Cap candidate hits at 128. The
   `38D3:02DD` stop before a witness is a typed blocker, not a silent
   no-change result.
3. Before stepping the selected store, capture the full CPU/stack and
   640-KiB RAM, four distinct `A000:[1180,12C0)` read-map plane windows
   (320 bytes each), and the original Graphics Controller selector/read
   map. After each debugger plane read, restore both controller values
   and verify restoration. Save Graphics Controller write mode, set/reset,
   rotate, bit mask, and Sequencer map mask and memory mode alongside the
   private stop record.
4. Single-step **one** store to its pinned successor:
   `0412→0415`, `0438→043B`, `0445→0449`, or `0451→0455`.
   Repeat the RAM, four-plane, and controller-state capture immediately.
   Missing or wrong successor, unrestored controller state, altered
   page-segment route, or incomplete planes is a typed blocker.

Both samples and the CPU frame stream belong in one ignored private run
directory. The frame JSON has `version:1`, one `run_token`, and ordered
`frames`, each with four one-to-four-digit ASCII hexadecimal WORDs
`cs/ip/es/di`. The paired plane manifest uses the existing version-1
`window_start=0x1180`, `window_bytes=0x140`,
`witness_offset=0x118F` schema and carries the same `run_token`.
It has exactly one event with the selected `pc/next_pc`, 1-based sequence,
two 640-KiB RAM files, eight distinct 320-byte plane files, and
`gc_before/gc_after` selector/read-map pairs for each sample. All paths
are relative to that run directory.

`tools/replay/cold_start_codeword_row14_event_plan.py` checks the pinned
debugger, private path containment, matching run tokens, strict CPU frame
selection, selected PC/successor equality with the single plane event,
four-plane lengths, saved EGA page segments, and read-map restoration.
The plane audit reports **observed** byte and `0x20` witness-bit changes
for the supplied pair only. Matching labels and file placement cannot
authenticate a debugger transcript, establish breakpoint completeness,
or prove that no earlier guest store changed the witness.

The distinct page-return and host-RGB gate still needs same-invocation
resident `+0074/+0087/+008C/+0099/+009C` or
`+00A0/+00B0/+00B3` helper frames tied to U19, then immediate
`+02DD/+02E2/+02FD/+0302` plane and RGB endpoints. Equal retained
DS:4A16/4A18 values do not prove absence of an intervening same-value
page selection.

Run the synthetic bundle gate without private media or an emulator:

```sh
python3 -m unittest tests.replay.test_cold_start_codeword_row14_event_plan
```

After a separately authorized native capture, analyze only ignored files:

```sh
python3 tools/replay/cold_start_codeword_row14_event_plan.py \
  --debugger resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/PRIVATE-RUN \
  --frames resources/oracle-runs/PRIVATE-RUN/cpu-frames.json \
  --manifest resources/oracle-runs/PRIVATE-RUN/plane-events.json
```

The original game media, debugger binary, CPU/RAM records, EGA planes and
pixels remain under ignored `resources/`. This plan earns no native,
first-writer, page/RGB, full-frame, C, or identity credit.
