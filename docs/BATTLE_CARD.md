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

Reviewed translation checkpoint: **66 verified game-specific full C
translations** (57 overlay, including PR #108's two ECL comparison helpers, PRs #124/#126/#128's unit `0x3BD` entries 4/2/3, PRs #130/#132's unit `0x7F` entries 10/11, PR #136's unit `0x25` entry 19, PR #138's unit `0x68:+0x068D` preflight, PR #145's unit `0x68:+0x05AB` record scan, PR #149's unit `0x68:+0x05EE` list search, PR #152's unit `0x68:+0x0648` record clear, PR #150's four unit `0x3C6` slot helpers, PRs #158/#159's unit `0x68:+0x0236` party-byte test and `+0x0109` map updater, PR #162's `+0x0000` cell-field clear, PR #194's unit `0x9B:+0x01AF` record update, PR #276's unit `0x9B:+0x0255` table leaf, PR #279's unit `0x9B:+0x0329` field leaf, PR #281's unit `0x9B:+0x0000` caller, PR #286's unit `0x5E:+0x2879` party value, and unit `0x9E` entry-8 dice roll,
and nine resident, including PR #79's `RES_00BA_2E8B`). PRs #65/#72's script initializer and block loader are
models. The independently translated `0x464` helper from PR #64 has unresolved
game/library ownership and is outside that count. The regenerated overlay
classifier has 118 verified identities. With ten verified resident identities
from the callable ledger, the evidence-backed game-identity floor is **128**.
PRs #96, #106 and #114 establish the class-1 hook and unit `0x3BD` entries 6 and 7 identities,
respectively, without strict full-C credit because their callees remain unresolved.
The merged PR #153 native DOS watch establishes the first value-changing
minute store for one saved Up; C-native replay remains open. Refresh all of
these counts before issuing a live card.

The native one-Up
resident-dispatch replay now measures mode 1, active word 100, argument 10,
and its selected local calls. The first live script RNG call advances seed
`3140872353 → 3238563622` and stores result 10; C-native one-Up parity remains
open.

Keep the visible card in an ASCII code block with a conspicuous `*` border and
short lines. Adapt the rows to the current evidence:

```text
************************************************************
* POOL OF RADIANCE DOS DECOMP -- BATTLE CARD              *
* AS OF: YYYY-MM-DD HH:MM TZ                              *
************************************************************
* GAME FUNCTIONS:    <verified> / <verified total|unknown> *
* FULL C ROUTINES:   <game-specific count> / unknown         *
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
