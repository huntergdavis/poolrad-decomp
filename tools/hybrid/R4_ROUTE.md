# R4 service route

R4 follows PM instruction i22827: the arms/armor shop first, then Sune temple.
It uses the pinned slot-A input and original guest code, with 72 gates and 70
inputs including canonical startup. The terminal area coordinate is `(1,3)`.
Successful training is deferred because the party needs legitimate progression.
No training branch is included in the current service candidate set.

Static DOS map/script analysis supports the city boundary from the saved
map-20 position `(15,4,6)`: Down turns around and Up crosses to map 0 at
`(0,4,2)`. The canonical harness first moves to `(14,4)`, so the R4 boundary
prefix is Down, Up, Up. The static shop path takes 20 movement/turn inputs from
that canonical position. Native preflight confirmed this path; the input count
excludes dialogues. No city interruption occurred on that pinned traversal.

The analysis reuses [save offsets](../../docs/formats/DOS_SAVGAM.md),
[boundary wrapping](../../docs/re/U25_MOVE_ENTRY19.md),
[load-word semantics](../../docs/re/U25_LOAD_WORD_0E0C.md), and the private
2026-10-24 pricing report. Original scripts, signatures, images and private
preflight records remain under ignored `resources/`.

## Navigation gates

An optional outer `navigation_states` object maps state names to `[x, y]`.
For these states, the driver requires both the state's prompt gate and the
exact displayed coordinate. The coordinate reader uses the pinned original
font from block 201 of the run's existing private `8X8D1.DAX` input, following
[the documented font mapping](../../docs/re/FONT_8X8D_201.md). The archive and
decoded font hashes must match. Six 16-by-16 display cells at `(464,424)` are
matched against their doubled 8-by-8 glyphs, ending at the first space. Each
2-by-2 pixel block must be uniform. Unknown glyphs, incomplete pixels, malformed
coordinates, and values outside 0–15 produce no coordinate and therefore no key.
Original glyph bytes stay in the ignored input media; there is no new library
dependency. The fallback is a bounded stop.

Whole-screen OCR merged an east-facing coordinate with the clock. Narrow
crops then confused the stylized digit 1 with 4 on some frames. Exact glyph
matching correctly decoded all six retained calibration frames, including both
facing directions and the first two city tiles. Existing canonical startup
gates retain their original digit OCR.

R4 also opts into `font_text` for the bottom prompt row and message region.
These use the same private font with uppercase/modulo-64 mapping, accepting
foreground pixels of any displayed text color. Ambiguous glyph aliases are
excluded. A complete exact row replaces OCR and records both readings; unknown
glyphs fall back to the existing OCR. This avoids accepting guessed aliases such
as the observed shop Yes/No OCR result. Four native menu captures were checked
against exact expected control text.

An optional outer `message_gates` map adds an any-of message-word condition
to the prompt gate. Message OCR uses the text region and participates in the
two-identical-observation stability check. The gateway transition needs this
because New Phlan's message appears while the coordinate display still shows
the previous map's edge. Subsequent movement refreshes the coordinate.

Recall sessions `01a13c73-c6d0-7540-a0f3-49feb1532032` and
`01a12855-addb-78d1-ad73-89ae2590dc75` supplied the prior font-evidence lead;
the pinned font and native frame matches were checked again for R4.

The existing six-field state format and R1/R2 gate definitions are retained.
Changes to the shared driver require a fresh original pair for each route used
with that driver; earlier native evidence does not validate a changed driver.

## Measurement requirements

Submit native jobs once through the shared heavy-job FIFO. Complete navigation
and menu preflight before freezing the route. Run two all-original references
with seed `12345678`; compare the exact driver, route, input and emulator hashes,
key sequence, final RGBA pixels and complete post-pin RNG sequence before
submitting an observer. C dispatch remains unset for these runs.

The observer candidate set comprises 14 previously unobserved credited bodies
reachable through static direct-call paths from the service menus. These are
branch candidates, not promised arrivals or new replacement credit. The five
Buy candidates include all four Heal candidates. Broader View, Take and
Appraise paths account for the expanded set.

| Marker | Guest entry |
| --- | --- |
| Shop menu | `OVL_0023_053E` |
| Buy action | `OVL_0023_0361` |
| Temple menu | `OVL_001E_0D09` |
| Heal action | `OVL_001E_09D7` |

`r4_report.py` requires complete per-ID counter/sequence agreement, monotonic
guest instruction order, all candidate and marker rows, exactly one entry to
each service in route order, and each action marker within its service phase.
It splits arrivals before services, after shop entry until temple entry, and
after temple entry until run end. These are event-order intervals, not recovered
caller stacks. Zero remains zero; markers are excluded from candidate counts.

The first native boundary probe loaded the save, pinned the seed, returned to
the east edge and displayed New Phlan's gateway message. Its coordinate display
remained at `(15,4)` and the probe timed out waiting for `(0,4)`; cleanup used
SIGTERM. This failed probe is retained. The second, interactive original-code probe confirmed that one further Up
refreshes the display at `(1,4)`. It followed the planned 20 inputs from the
canonical start and reached the arms/armor offer at `(8,11)`, with no city
interruption on that traversal.

The shop preflight pooled the existing funds, bought the highlighted hand axe,
shared the remainder, then bought another hand axe using personal funds.
View/Items showed both new axes. Appraise returned directly without an item
selection. Take opened the coin-type list, selected gold, then cancelled its
empty quantity prompt with Escape and exited the coin list. The pool was shared
before leaving the shop. No coin transfer or successful appraisal is claimed.
The area screen returned at `(8,11)`. Seventeen guarded movement/turn inputs
then reached Sune at `(1,3)`, with no city interruption. The priestess's offer
opened the temple menu and Heal opened its service list. End moved the selection
from the first to the second service; Down did not advance it. Probe B then
expired at its operator-input deadline and shut down with SIGTERM. Its captures
and failure remain retained; this is not a completed reference run.

Probe C replayed the complete shop and city path automatically, including the
two purchased axes visible in the inventory capture. It then verified the
remaining Heal controls interactively: two End inputs selected the third
service, Return opened its 100-gold quote, and Yes returned to the Heal list.
No result message was captured and no successful healing is claimed. The
previously measured combined funds were only 64 gold-equivalent after buying
the axes. Exit left the list, a second Exit left the temple, and the final
area screen showed `(1,3)`. No city interruption occurred in this traversal.
The route qualifies payment with the measured quote as well as the prompt.

Paired references and observer results are pending. Par-b owns independent
review, merge and subsequent V1/arrival-route validation.
