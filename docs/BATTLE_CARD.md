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
The independently reviewed checkpoint after PR #1188 is **245 strict full-C game routines** (219 overlay, 26 resident) and **291 verified game identities** (264 overlay, 27 resident). Both whole-game denominators remain unknown. Counts and queue provenance are in [the generated accounting checkpoint](re/function_accounting_checkpoint.json).
<!-- FUNCTION_ACCOUNTING:END -->

The saved slot-A **first-view** C page matches the same-state native page in
32,000/32,000 plane bytes; after measured AC/DAC palette conversion, the full
320×200 C RGB frame matches the native screenshot in **64,000/64,000 pixels**
([frame evidence](re/SLOT_A_RGB_FRAME_PALETTE.md)). The joined C route still
stops at key dispatch for one Up, so a complete **post-input** C frame has
not been measured.

## Next visible milestone: one complete post-Up C slot-A frame

Show four concrete outputs and the PM's integration gate:

1. **Native post-Up oracle:** pin the owner save, one-Up input, resulting
   coordinates, display planes, palette, and 320×200 RGB frame in one run.
2. **C key route:** carry that input through the reached game dispatch and
   movement state updates. The current joined C route explicitly returns
   `BLOCKED_KEY_DISPATCH` before this point.
3. **C post-input render:** compose art, text, and display transfer from the
   resulting state, then compare all 32,000 plane bytes. The first-view C
   page differs from native post-Up by 725 plane bytes and cannot be reused.
4. **PM integration:** compare all 64,000 same-state RGB pixels and show DOS
   and C screenshots side by side only when the entire route passes.

These are evidence gates, not implied staffing. Name actual active, held, and
proposed workers from current PM evidence. Startup, title, protection, and
music remain separate experience gates. After one Up is playable, extend the
route to repeated movement and an encounter. The longer roadmap is full DOS
decompilation, then Amiga graphics and C64 sound/music, then Nintendo DS.

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
* FOUR OUTPUTS TO POST-UP C FRAME                       *
* 1 NATIVE ORACLE: <owner, result, status>                *
* 2 C KEY ROUTE: <owner, result, status>                  *
* 3 C RENDER: <owner, result, status>                     *
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
