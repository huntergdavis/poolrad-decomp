# Project battle card

Use this format when the CTO asks for Pool of Radiance progress. Refresh
main, open PRs, local verification, the PM, and active worker panes first.
Give the observation time in America/Los_Angeles. Distinguish merged work,
open PRs, and unpublished findings. Lead with the next owner-visible result.

## Two separate scoreboards

**Player experience** follows the owner's journey: startup, title, credits,
copy protection and music; load a saved game; see its first complete view;
then take one movement step. For each gate, report what the original DOS
oracle has demonstrated and what the portable C program can do end to end.
A translated helper, library test, or isolated renderer does not complete an
experience gate. Do not assign an overall percentage to these unequal gates.

**Code recovery** records strict full-C game routines and independently
verified game identities. State both counts against unknown whole-game
denominators. A larger count does not by itself make the game playable.
Keep ECL opcode mapping, assets, and bounded models separate from strict
full-C routine credit. Candidate Pascal prologues are not a verified function
inventory.

<!-- FUNCTION_ACCOUNTING:BEGIN -->
The independently reviewed checkpoint after PR #472 is **78 strict full-C game routines** (69 overlay, 9 resident) and **139 verified game identities** (129 overlay, 10 resident). Both whole-game denominators remain unknown. Counts and queue provenance are in [the generated accounting checkpoint](re/function_accounting_checkpoint.json).
<!-- FUNCTION_ACCOUNTING:END -->

The current portable-C static-view checker compares **128 of 64,000 pixels**
with zero mismatches in that bounded region. The other 63,872 pixels are
unknown. This is comparison scope, not screen-completion percentage or proof
of a full frame. Refresh this measurement before a live card.

## Next visible milestone: one complete C slot-A frame

Show four concrete outputs and the PM's integration gate:

1. **Art and geometry:** determine every first-view wall, door, and item
   selection, placement, and constructed offscreen source object. Compare
   the full native draw-call list and resulting offscreen object bytes with C.
2. **Display transfer:** trace the reached path from offscreen objects to
   actual display memory, then compare changed plane-byte addresses and
   values with C. A matched first transfer does not prove the whole frame.
3. **Text and UI:** recover the reached saved-view text callbacks, glyph
   positions, and covered indexed pixels; compare with the native frame.
4. **Palette and integration:** measure the native palette for that exact
   slot-A state, join the three outputs, and compare all 64,000 indexed
   pixels and their colors with a normalized DOS oracle. Only then show
   matching DOS and C screenshots side by side.

The first three outputs can be researched independently if interfaces and
files do not overlap. The fourth consumes all three and is owned by the PM.
This is a four-part **work map**, not a request to staff four agents. Name
actual active, held, and proposed workers from current PM evidence. The
startup/title/credits/protection/music research is a separate experience
track; it does not silently count as first-frame delivery.

The next experience gates after the full frame are C save loading and one
Up movement with DOS parity. The longer roadmap is full DOS decompilation,
then Amiga graphics and C64 sound/music, then a Nintendo DS port.

## Capacity and evidence rules

For Claude, state the observed time, remaining included Current session,
Current week all-model, and Current week Fable bars, and their reset times.
Report Codex quota separately. If a panel is unavailable, say when the last
direct reading was taken. Monthly Claude usage credits are extra/overage
capacity, not the included subscription balance. State the current owner or
PM staffing rule; do not reuse an old rule without checking.

Present an original DOS screenshot only beside a matching portable-C
screenshot from the same state. If C imagery is not ready, use text. Cite
the evidence behind material claims. If no worker pane is accessible, mark
the age of the last published state.

## Executive card layout

Keep the visible card in an ASCII code block with large `*` separators and
short lines. Adapt every row to fresh evidence:

~~~text
**********************************************************
* POOLRAD DOS  |  CTO BATTLE CARD                         *
* AS OF: YYYY-MM-DD HH:MM TZ                              *
**********************************************************
* PLAYER EXPERIENCE             DOS         PORTABLE C     *
* Startup/title/credits         <seen>      <actual gate>  *
* Music                         <observed/unknown> <C gate>*
* Protection and save load      <seen>      <actual gate>  *
* First saved view              <seen>      <actual gate>  *
* One movement step             <seen>      <actual gate>  *
* NEXT DEMO: <one owner-visible result>                   *
**********************************************************
* CODE RECOVERY                                            *
* STRICT FULL-C: <count> / unknown game total             *
* IDENTITIES:    <count> / unknown game total             *
* PIXEL CHECK:   <covered>/<64000>; <mismatch>; scope only*
**********************************************************
* FOUR OUTPUTS TO FIRST COMPLETE C FRAME                  *
* 1 ART/GEOMETRY: <owner, result, status>                 *
* 2 DISPLAY TRANSFER: <owner, result, status>             *
* 3 TEXT/UI: <owner, result, status>                      *
* 4 PALETTE + INTEGRATION: <PM gate, status>              *
* STAFFED NOW: <actual workers and separate tracks>      *
**********************************************************
* CAPACITY & DECISION                                     *
* CLAUDE: <verified bars, time, reset, active lanes>       *
* CODEX:  <verified quota, time, reset, active lanes>      *
* RULE:   <current stop/scale decision>                   *
**********************************************************
* ROADMAP: C frame > C load > one Up > full DOS           *
*          > Amiga art/C64 music > Nintendo DS            *
**********************************************************
~~~

After the card, give one short paragraph on the main blocker and next step.
Link the current PR or evidence file, such as
[SLOT_A_STATIC_VIEW_FIRST_REGION.md](re/SLOT_A_STATIC_VIEW_FIRST_REGION.md)
and the DOS first-view trace
[SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md](re/SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md).
