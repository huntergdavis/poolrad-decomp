# R2 camp route

`WALK_ROUTE=r2` extends the accepted R1 harness with a separate JSON route for Camp Magic,
Rest adjustment, and Alter/Order. All input discovery and reference runs use
original guest code, a disposable copy of the pinned slot-A save, and the same
explicit seed pin. C dispatch remains unset. Submit each native job once
through the shared heavy-job FIFO.

The retained static design identifies 22 candidate bodies: six rest/order
bodies and 16 list/selector bodies shared with R1. The completed observer
measures arrivals after Camp entry for 20 of 22 candidates, including all six
rest/order bodies and 14 of the 16 shared candidates. These are arrival
measurements, with no replacement credit.

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
Both complete original references exercised the live narrow Exit fallback,
including two matching observations before the Display exit. They also verified
the assembled Magic-to-Rest-to-Magic return. Probe runs are excluded from that pair.

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
WALK_ROUTE=r2 R1_SEED_PIN=12345678 /path/to/heavy-wrapper tools/hybrid/run_r1.sh r2-observer-b observer resources/hybrid/registry_r2_pin.tsv
```

The accepted launcher and run-artifact names retain their `r1` prefix; the
selected JSON and immutable snapshots identify R2. No second launcher is needed.

## Pinned reference and observer measurements

The 2026-10-24 original pair completed and passed comparison before the observer
was submitted. All three successful runs complete 31 gates (five canonical
startup gates and 26 R2 gates), with no error and SIGTERM cleanup. Their exact
gate/key sequences, final pixels, driver, route, input-manifest, and emulator
hashes agree. Each records the actual seed pin `12345678`. Complete sequences
from each actual pin agree: one pinned draw and no subsequent RNG draws.

| Measurement | Original A | Original B | Observer B |
| --- | ---: | ---: | ---: |
| Final gate, elapsed seconds | 434.440 | 386.510 | 328.012 |
| Shutdown, elapsed seconds | 445.003 | 397.386 | 338.633 |
| Complete sequence rows | 30533 | 30532 | 30657 |
| Post-pin draw entries, including pinned draw | 1 | 1 | 1 |

All three final RGBA SHA256 values are
`4474ef303441be51e27e3227fa5586dbef7f3d87ff0facc423e4c585c80b4b62`.
The gate/key sequence SHA256 is
`ed590d7eb5b22bd82348c346bcc17053b7a8fa8a9427dc602822db1434564b10`.
The originals have 30,533 and 30,532 draw entries; Observer B has 30,533.
The difference is before the pin. Every final counter agrees with its complete
sequence, including zero-count rows. No input-sequence exception is needed.

Selected gate times in elapsed seconds:

| Gate | Original A | Original B | Observer B |
| --- | ---: | ---: | ---: |
| `code` | 52.995 | 58.609 | 47.097 |
| `at14_4` | 129.320 | 117.370 | 95.463 |
| `camp_magic` | 154.794 | 141.586 | 113.940 |
| `display_exit` | 266.844 | 221.541 | 195.999 |
| `rest_increase` | 287.947 | 240.798 | 213.896 |
| `magic_leave` | 320.304 | 277.193 | 240.323 |
| `order_before` | 368.190 | 323.943 | 275.738 |
| `order_after` | 378.934 | 335.188 | 285.208 |
| `final` | 434.440 | 386.510 | 328.012 |

The first observer attempt, `r2-observer-a`, timed out waiting for the Code
gate at 308.545 seconds on the credits screen. It sent no route keys, never
reached the seed pin, and recorded no candidate arrivals. Its final report
contains 30,531 draw arrivals and 695,821,519 guest instructions; cleanup used
SIGTERM. It is retained and excluded from successful route comparisons. The
unchanged repeat, Observer B, passed. The startup stall cause is not established.

Shared immutable SHA256 values:

| Artifact | SHA256 |
| --- | --- |
| Driver | `8d2fbe78bfc38e19809592d2bc584936bd7b78c6aeda575b02a84156290ad84b` |
| R2 route | `339ac5013830fa2e462ac4e20f435027ecc6f0d95b79d6628aff1b90da94cdd2` |
| Input manifest | `adf69e4405ea60fe45037c018efe0b3bcdd72c357ff16927dae9a1d879735b55` |
| Private emulator | `af3000ec35c4c799d73cc2eb55e0670912b57a740343a6915bfdf8ed802286c0` |
| START.EXE | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A save | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Original draw-pin registry | `b67de8edb2c8fb68620617cfd96e74cf28a16410203eff1797ee305ff9b3155a` |
| Observer registry | `ece8ef25e9fbb1d87b1d0b2e1aa033c3ce1a38fdc50c280f135b4ec59c4c281b` |

The published runtime/report files are byte-identical to the frozen native
worktree used for these runs. Full timings, validation JSON, CSV, counters,
sequences, and failed-run evidence remain in ignored `resources/`.

## Arrival attribution

The observer registry contains the unchanged unique rows for all 22 candidates,
the draw pin, and four menu-entry observation markers:

| Phase | Observation marker | Arrivals |
| --- | --- | ---: |
| Camp | `OVL_006A_1E6F` | 1 |
| Magic | `OVL_006A_13F1` | 1 |
| Rest | `OVL_006A_077A` | 1 |
| Order | `OVL_006A_1780` | 1 |

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
python3 tools/hybrid/r2_report.py resources/oracle-runs/r2-observer-b
```

## Measured candidate arrivals

Observer B records 120 candidate arrivals across 20 reached IDs, all after the
measured Camp entry; no candidate arrives before it. Two candidates remain zero.
Four marker arrivals and 30,533 draw entries bring the complete sequence to
30,657 rows. All 27 registry IDs are present, with no skipped or missing row.

| Candidate | Before Camp | After Camp | Shared with R1 |
| --- | ---: | ---: | --- |
| `OVL_006A_14A9` | 0 | 1 | no |
| `OVL_006A_1601` | 0 | 1 | no |
| `OVL_007D_03A3` | 0 | 3 | no |
| `OVL_007D_047A` | 0 | 1 | no |
| `OVL_007D_0565` | 0 | 12 | no |
| `OVL_007D_05F0` | 0 | 4 | no |
| `OVL_0086_028D` | 0 | 12 | yes |
| `OVL_0086_044E` | 0 | 0 | yes |
| `OVL_0086_051C` | 0 | 0 | yes |
| `OVL_0086_059C` | 0 | 3 | yes |
| `OVL_03B0_0000` | 0 | 23 | yes |
| `OVL_03B0_0067` | 0 | 2 | yes |
| `OVL_03B0_0711` | 0 | 4 | yes |
| `OVL_03B0_07A4` | 0 | 4 | yes |
| `OVL_03B0_08D7` | 0 | 12 | yes |
| `OVL_03B0_0923` | 0 | 12 | yes |
| `OVL_03B0_0969` | 0 | 6 | yes |
| `OVL_03B0_0A08` | 0 | 6 | yes |
| `OVL_03B0_0B03` | 0 | 6 | yes |
| `OVL_03B0_0C33` | 0 | 2 | yes |
| `OVL_03B0_0CC5` | 0 | 4 | yes |
| `OVL_03B0_11F3` | 0 | 2 | yes |

This establishes route arrivals, not full-state equivalence or C replacement.
Par-b owns independent review, merge, and subsequent V1/route validation. No
spell was selected for memorization and no timed rest was started.
