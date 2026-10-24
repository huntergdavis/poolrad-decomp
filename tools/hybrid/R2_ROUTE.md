# R2 camp route

`WALK_ROUTE=r2` extends the accepted R1 harness with a separate JSON route for Camp Magic,
Rest adjustment, and Alter/Order. All input discovery and reference runs use
original guest code, a disposable copy of the pinned slot-A save, and the same
explicit seed pin. C dispatch remains unset. Submit each native job once
through the shared heavy-job FIFO.

The retained static design identifies 22 candidate bodies: six rest/order
bodies and 16 list/selector bodies shared with R1. Candidates are possibilities,
not measured arrivals. The complete pinned original pair is still pending.

The DOS source evidence is in
[the Rest preflight](../../docs/re/U7D_UPDATE_03A3_ERID_PREFLIGHT.md),
[party-before](../../docs/re/U6A_PARTY_BEFORE_14A9.md), and
[party-after](../../docs/re/U6A_PARTY_AFTER_1601.md). Order has two modes:
character selection, then selected-character movement. The control bitmap at
U6A `+174E` accepts Return/P/S to switch modes. Extended Home/End reach
`+14A9/+1601` only in movement mode. The native preflight confirmed this mode
distinction and the reverse movement.

Rest adjustment is separate from completed rest. The U7D `+0721` handler starts
on minutes; I adds five minutes and D subtracts five. Return maps to R and
starts resting. R2 uses I, D, then Exit to exercise adjustment and return to
the menus. It does not claim completed rest or newly readied spells.

## Native preflight findings

The first probe completed the canonical startup and Camp/Magic entry. The
interactive second probe verified that End selects the next party member in
Magic; Down selects the head through the
[active-selection handler](../../docs/re/U77_ACTIVE_SELECT_27DF.md). The
selected second member opened a two-page Memorize list. Home, End, Page Down
and Page Up were exercised. The captured screen reported zero free
memorization slots, so this route browses the list without claiming a newly
memorized or readied spell.

Display reached its effects screen with a standalone Exit control. The normal
660-by-26 OCR crop returned empty text, including with an alternate page mode.
A 110-by-26 crop at the same origin, scaled fourfold, read the control as
`EXT`. The second probe stopped at its bounded blank-prompt limit and cleaned
up with SIGTERM; no attempted manual recovery input was sent.

The R2 driver change retries only empty prompt results with that narrow crop,
accepts only `EXIT`, `EXT`, or `EXT.`, and records the fallback in observations.
It rejects `NEXT` and other strings. The optional outer route field
`exact_prompt_states` requires a whole-prompt match for specified states,
ignoring surrounding spaces and periods. R2 uses it for the standalone Display
exit so an earlier menu containing the word Exit cannot satisfy that gate.
The six fields within each state record remain unchanged. Existing R1 states
retain substring matching.

The third probe confirmed Rest duration changing from zero to five minutes and
back to zero, then returning to Camp through Exit. It entered Alter/Order,
used Return to change Select to Place mode, moved the selected first member
to the tail with Home, restored the original order with End, and used Return
to return to Select mode. Exit then returned through Alter and Camp to the
area screen. Cleanup completed successfully with SIGTERM.

Sixteen driver tests, four R2 route tests, and seven attribution tests pass.
The narrow Exit crop was measured on the retained frame; its live use and the
complete assembled route await the original reference pair. Probe runs are
excluded from that pair.

The full route selects the second member with End, browses the Memorize list
with Home/End/Page Down/Page Up, leaves it, opens and exits Display, adjusts
Rest through Magic with I/D/Exit, then leaves Magic. It enters Alter/Order,
selects that member with Return, applies Home/End, releases selection with
Return, and exits Order/Alter/Camp. The terminal frame is the area screen.
It neither selects a spell for memorization nor starts timed rest.

```sh
WALK_ROUTE=r2 R1_SEED_PIN=12345678 /path/to/heavy-wrapper tools/hybrid/run_r1.sh r2-original-a original
WALK_ROUTE=r2 R1_SEED_PIN=12345678 /path/to/heavy-wrapper tools/hybrid/run_r1.sh r2-original-b original
# Submit only after the pinned original pair completes and is compared:
WALK_ROUTE=r2 R1_SEED_PIN=12345678 /path/to/heavy-wrapper tools/hybrid/run_r1.sh r2-observer-a observer resources/hybrid/registry_r2_pin.tsv
```

The accepted launcher and run-artifact names retain their `r1` prefix; the
selected JSON and immutable snapshots identify R2. No second launcher is needed.

## Arrival attribution

The observer registry contains the unchanged unique rows for all 22 candidates,
the draw pin, and four menu-entry observation markers:

| Phase | Observation marker |
| --- | --- |
| Camp | `OVL_006A_1E6F` |
| Magic | `OVL_006A_13F1` |
| Rest | `OVL_006A_077A` |
| Order | `OVL_006A_1780` |

Markers are excluded from candidate counts. Original references use only the
draw-pin registry. Keep registry signatures, input media, captures and raw logs
under ignored `resources/`.

`r2_report.py` requires a successful route, no forced-kill report, complete
per-ID counter/sequence agreement, monotonic instruction order, all candidates
and markers present, and exactly one measured Camp entry. It separates candidate
arrivals before and after that entry. Only arrivals after Camp entry establish
that a shared candidate was reached on this new route. Zero means not observed.

The sequence records guest instruction counts, not wall-clock timestamps.
Buffered sequence-file size and UI timestamps therefore do not establish phase
boundaries. The report uses actual guest marker order and does not reconstruct
caller stacks or assert a precise parent call for each arrival. Its scope is
after Camp entry through the end of the run.

```sh
python3 tools/hybrid/test_r2_report.py
python3 tools/hybrid/test_r2_route.py
python3 tools/hybrid/r2_report.py resources/oracle-runs/r2-observer-a
```

Complete native measurements must include the two original final hashes, the
observer final hash, immutable input/harness/emulator hashes, actual seed-pin
events, gate timings, teardown, complete counters, and any differing input
sequences. Par-b owns review, merge, and subsequent V1/route validation. These
arrival measurements do not award replacement credit.
