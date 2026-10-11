# Project battle card

Use this format when the CTO asks for a Pool of Radiance decomp status update.
Refresh the evidence first: current `main`, open PRs/issues, local test results,
and active worker panes when available. Distinguish merged, PR, and unpublished
worker findings. Include an as-of time in America/Los_Angeles.

Report percentages only where both the completed count and denominator are
measured for the same scope. Give the ratio beside the percentage. For game
functions, count only translated functions that compile and pass a stated
behavior check; if the total is not yet verified, write `0 verified / total
unknown` (or the current count) rather than inventing a completion percentage.
The 675 Pascal prologues in the first audit are candidate entry points, not a
verified function inventory. For ECL, distinguish opcode mapping from handler
implementation. Do not combine asset coverage and engine work into a single
overall completion percentage without a measured weighting.

Keep the visible card in an ASCII code block with a conspicuous `*` border and
short lines. Adapt the rows to the current evidence:

```text
************************************************************
* POOL OF RADIANCE DOS DECOMP -- BATTLE CARD              *
* AS OF: YYYY-MM-DD HH:MM TZ                              *
************************************************************
* GAME FUNCTIONS:    <verified> / <verified total|unknown> *
* ECL OPCODES MAPPED: <n> / <total> = <percent|unknown>    *
* ECL HANDLERS IN C: <n> / <total> = <percent|unknown>     *
* DAX ARCHIVES READ: <n> / <supplied> = <percent>          *
* DAX BLOCKS READ:   <n> / <supplied> = <percent>          *
* GEO RECORDS READ:  <n> / <supplied> = <percent>          *
* DOS ORACLE:        <last observed milestone>             *
************************************************************
* CLAUDE: <branch/PR and current deliverable>              *
* CODEX:  <branch/PR and current deliverable>              *
* PM:     <integration status and next gate>               *
************************************************************
```

After the card, give one short paragraph on the main blocker/next step and
link the relevant PR, issue, or evidence file. If no worker pane is accessible,
use the latest published evidence and mark its age; do not imply live progress.
