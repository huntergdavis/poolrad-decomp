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

## Paired reference result (2026-10-24)

The original pair passed before observer submission. Comparison was confirmed
at 11:21:15 UTC; observer ticket 74 was submitted afterward. Both references
completed all 72 gates with the same 70 inputs, actual seed pin `12345678`,
identical driver/route/input/emulator hashes, identical final RGBA pixels, and
the same complete 33-value post-pin RNG sequence. Each complete DRAW log has
30,565 rows, agreeing with its final counter; the pin is at draw 30,533.

| Original run | Seconds to final gate | Cleanup |
| --- | ---: | --- |
| `r4-original-a` | 1402.685 | SIGTERM |
| `r4-original-b` | 1098.871 | SIGTERM |

Neither run needed SIGKILL. The final screen is `(1,3)`, north-facing, at game
time 11:22. The route and driver were frozen for these runs against main
`fb01b862`. Rebasing onto main `d08a1012` afterward left both runtime files
byte-for-byte unchanged. All 43 focused harness tests passed against the
initial base; the subsequent upstream change did not touch the harness.

| Evidence | SHA-256 |
| --- | --- |
| Private emulator | `af3000ec35c4c799d73cc2eb55e0670912b57a740343a6915bfdf8ed802286c0` |
| Driver | `600fdb18d80f11e90a6d4a6093e779558bfb3c63379bf7010951d397dac8eb32` |
| Route JSON | `f94abe6b68d7f6f0cde4e8bc3bf8a9bc7540233c376bcda08491d0ff03313c9a` |
| Input manifest | `adf69e4405ea60fe45037c018efe0b3bcdd72c357ff16927dae9a1d879735b55` |
| Final RGBA | `008650a6d2d674d16b71631cb77bc8577ca0ecebe361a315a9d36dc3c504a9d2` |
| Gate/key sequence (JSON) | `040aefad328fb89f3db832daaaec8a5ebdeb1de2a850cd8a64555fc50b6a8c2b` |
| Post-pin RNG values (JSON) | `1a1cf3c05b2f43b310f504fd392833bb0ebe3709638e2072beb9087087d97297` |
| Original pin registry | `b67de8edb2c8fb68620617cfd96e74cf28a16410203eff1797ee305ff9b3155a` |
| Observer registry (19 unique rows) | `15e59020914afd29cea61a579ef540627e4f55f3f11715c9642034d0a43fe94b` |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |

## Observer result

`r4-observer-a` completed the same 72 gates in 1458.364 seconds. Its provenance,
gate/key sequence, final RGBA pixels and all 33 post-pin RNG values match both
original references. No forced kill occurred. All 19 unique registry rows were
present, with no skipped rows. Its complete 30,581-row sequence agrees with the
final counters: 30,565 DRAW arrivals, 11 candidate arrivals and five marker
arrivals.

Seven of the 14 candidate IDs arrived. All seven occurred in the shop interval
(nine arrivals); two also occurred in the temple interval (two arrivals).
There were no candidate arrivals before the shop. The two temple IDs overlap
the shop set and must not be added to the seven-ID union.

| Candidate ID | Before services | Shop | Temple | Whole run |
| --- | ---: | ---: | ---: | ---: |
| `OVL_0077_17ED` | 0 | 0 | 0 | 0 |
| `OVL_0077_275B` | 0 | 2 | 0 | 2 |
| `OVL_0077_28BE` | 0 | 2 | 1 | 3 |
| `OVL_007F_00A8` | 0 | 1 | 1 | 2 |
| `OVL_007F_0130` | 0 | 1 | 0 | 1 |
| `OVL_007F_0185` | 0 | 1 | 0 | 1 |
| `OVL_007F_0203` | 0 | 0 | 0 | 0 |
| `OVL_007F_044C` | 0 | 0 | 0 | 0 |
| `OVL_007F_0A01` | 0 | 0 | 0 | 0 |
| `OVL_007F_0A88` | 0 | 1 | 0 | 1 |
| `OVL_007F_0B93` | 0 | 1 | 0 | 1 |
| `OVL_0086_044E` | 0 | 0 | 0 | 0 |
| `OVL_0086_051C` | 0 | 0 | 0 | 0 |
| `RES_00BA_15F7` | 0 | 0 | 0 | 0 |

The shop and temple entry markers each arrived once, at guest instructions
764,874,209 and 1,641,146,450 respectively. Buy arrived twice and Heal once;
all action markers were inside their corresponding service intervals.

Post-run capture checks independently found two hand axes in each of the three
inventory frames and the named 100-gold Heal quote in each payment gate. The
inventory RGBA hash is identical across all three:
`97699473cefb522da54fc186f6e5d9d866f27a96573dfd4f336f7c6615333173`.
Private pair/full comparison reports, the per-ID CSV and the outcome audit
remain under ignored `resources/` for review.

Par-b owns independent review, merge and subsequent V1/arrival-route
validation. These are original-guest arrival measurements; this route does
not award replacement credit. Successful healing and training remain outside
the measured route.
