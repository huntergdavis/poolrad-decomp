# Overlay routine classification: game, helper, or unresolved (issue #26, part 2)

**Scope.** A bounded, reproducible classification of the 673 evidence-backed overlay routine
candidates from [OVERLAY_FUNCTION_LEDGER.md](OVERLAY_FUNCTION_LEDGER.md), for the DOS v1.3
files identified in [overlay_inventory.md](overlay_inventory.md) (`GAME.OVR` sha256
`872964a3…928614`, `START.EXE` sha256 `d825df2b…6a07bd`). It measures byte-level signals per
routine and applies fixed rules; it does not turn prologue hits into a game-function
denominator. The tool runs only on the audited v1.3 digests (checked against the overlay
file, the inventory and the call graph) and only when the function ledger's IDs equal the
inventory's candidate set. Per-address rows: [overlay_classification.csv](overlay_classification.csv);
generated counts: [overlay_classification_summary.md](overlay_classification_summary.md);
tool: `tools/overlay/classify_overlay.py` with synthetic tests in
`tools/overlay/test_classify_overlay.py`. The resident side uses the same class vocabulary in
Codex's [resident ledger](resident_loader_ledger.csv). An exact `deja` query for prior overlay
classification work returned no session.

## 1. Signals measured per candidate

From a bounded linear decode (Capstone 5, 16-bit) of each prologue candidate up to the next
prologue, after a digest check of `GAME.OVR`:

| signal | how it is measured | routines |
| --- | ---: | ---: |
| string constants | `mov di,imm16; push cs; push di` where `imm16` addresses a length-prefixed, fully printable byte string inside the unit's own code segment (Turbo Pascal typed string constant) | 216 (664 references) |
| documented engine globals | DS-relative operands equal to byte-verified engine state addresses from the ledgers and contracts (ECL PC, buffer, opcode latch, stop flags, call stack, compare flags, operand slots, the three windows, party and active-character pointers, shared dword block, window state, the procedure variable) | 249 |
| calls into verified routines | direct near calls, or far calls through entry slots, to a byte-verified routine | 175 |
| verified routine | an address in the reviewed verified set in `tools/overlay/classify_overlay.py`, with per-routine evidence linked below and in the recovered-code notes | 117 |
| leaf | no calls and no DS-relative globals | 70 |
| runtime-only | every call goes to the SYSTEM segment 0xAF8, no DS-relative globals | 16 |

## 2. Classes and rules

| class | rule (applied in this order) | confidence | count |
| --- | --- | --- | ---: |
| `game_verified` | verified routine | verified identity and role, with complete behavior reviewed for the translated subset | 117 |
| `game_evidenced` | string constant, documented engine global, or call into a verified routine | provisional: printable text and engine state are evidence of application code, not proof of game logic | 339 |
| `helper_candidate` | leaf or runtime-only, no strings, no DS globals, not verified | provisional: small utilities; a leaf can still be game arithmetic | 75 |
| `game_or_other_library_unresolved` | everything else (calls resident game-code segments or overlay routines, or touches undocumented globals, without text or verified engine state) | unresolved | 142 |

By evidence grade from the function ledger: `game_verified` 53 A, 63 B, 1 D; `game_evidenced`
208 A, 130 B, 1 D; `helper_candidate` 54 A, 21 B; unresolved 87 A, 55 B.
The sixteen PR #63 IDs are distinct from the prior verified set and promote
fifteen unresolved rows and one helper candidate. The generated CSV records
each exact ID, with no changes to measured call or byte-signal columns.
PR #65's view setter promotes one further unresolved row. Its far caller at
`0x3b4:0x016d` gains one verified-call signal; the caller's class does not
change. The adjacent unit `0x25:0x01b6` initializer remains a model.
PR #69 promotes four more unit `0x38` entries: three unresolved rows and one
helper candidate. PR #85 promotes two more unit `0x38` unresolved rows to verified. The PR #72 ECL block loader remains a model and is excluded.
PR #96 promotes the game-specific class-1 character-record store hook from
`game_evidenced` to `game_verified` after PM byte-level parity review. Its
resident and overlay callees remain callback boundaries, so it does not add
a strict full-C translation.
PR #106 promotes unit `0x3bd:+0x0e08` after a complete local-path review;
its external calls keep it a model. PR #108 promotes unit `0x25:+0x1815`
and `+0x18eb`; both comparison helpers have strict full-C translations.
PR #114 promotes unit `0x3bd:+0x10c7`, the wallset load route reached by
the opcode handler and saved-game loader. Its external calls keep it a model.
PR #124 promotes unit `0x3bd:+0x057a` after byte-level parity review and gives
it strict full-C credit. Caller `0x25:+0x19db` moves from unresolved to
provisional game evidence because it calls the verified entry; four other
callers gain a verified-call signal without changing class.
PR #126 promotes unit `0x3bd:+0x047b` after parity review of every selector
path, including the pre-existing stack byte returned on an unmatched selector.
Four callers move from unresolved to provisional game evidence because they
call this verified entry; those caller bodies are not translated by this PR.
PR #128 promotes unit `0x3bd:+0x0350` from provisional to verified after
byte-level parity review of its field lookup and nested entry-2 call.
Three caller rows gain a verified-call signal without changing class.
PR #130 promotes unit `0x7f:+0x0130` from provisional to verified after
independent parity review of its five-word clear and unsigned divide-by-five
split. Its three callers gain a verified-call signal without changing class.
PR #132 translated unit `0x7f:+0x0185` fully but deferred game ownership.
The PM's postmerge audit identified its 28-byte output block as state cleared
by a verified ECL handler and used by game callers. Documenting the block
promotes only this routine from unresolved to verified; other touched rows
retain their class. It earns one strict full-C game-function credit.
PR #136 promotes unit `0x25:+0x19db` from provisional to verified after an
independent byte-parity review of all movement and bounded-call paths. Its
verified ECL CALL-handler caller gains one verified-call signal. The isolated
routine earns strict full-C credit; whether the saved slot-A ECL run reaches
that CALL remains under investigation.
PR #138 promotes unit `0x68:+0x068d` from provisional to verified after
independent parity review of all 55 instructions and 174 bytes of the Up
preflight routine. Its entry-2 caller gains one verified-call signal while
remaining provisional. The isolated routine earns strict full-C credit;
the live direction-table bytes, the executed coordinate write and the minute
writer remain open in the full slot-A route.
PR #145 translated the call-free 67-byte unit `0x68:+0x05ab` record scan.
The PM's independent byte-parity and postmerge caller audit traced both near
call sites through party-list search and record-byte clearing. This promotes
the scan from provisional helper to verified identity and earns one strict
full-C game-function credit. Both callers gain a verified-call signal;
`+0x05ee` remains provisional and `+0x0648` moves from unresolved to
provisional. The scan's participation in the saved one-Up execution remains
unproved; the later native watch in issue #3 did observe the unit `0x7D` writer.
PR #149 translated the complete 90-byte party-list search at unit
`0x68:+0x05ee`, calling the verified record scan. The PM independently
matched its byte loop and the two direct caller sites. Its documented party
head and member links establish game-state ownership. Promote this routine
from provisional to verified and award one strict full-C credit. Two callers
gain verified-call signals without changing class. The live one-Up writer
observed in issue #3 does not by itself prove this search executed.
PRs #150/#152 add five reviewed full C game routines: four unit `0x3c6`
character-slot accessors and unit `0x68:+0x0648` party-record clear. The
`0x3c6` entries address the game character pointer/slot tables and are used
by the unit `0x9e:+0x0160` party-sidecar model; the latter remains bounded
and uncredited. The record-clear helper calls the already verified party-list
search and record scan, then mutates one party record byte. The five rows
move to verified, and 17 other unresolved rows move only to provisional
evidence because they call those verified slot helpers. In all, 43 CSV rows
have changed signals or classes; no other row gains strict C credit.
The separate native watch in issue #3 proves the coordinate/clock writer on
one saved Up, not execution of these five routines on that route.
PRs #158/#159 add two reviewed complete unit `0x68` game routines: the
party-list signed-byte test `+0x0236` and two-bit map-field updater `+0x0109`.
Independent DOS-byte parity review established their call sites and game-state
roles. The classifier promotes the party test from provisional and the map
updater from unresolved to verified; one caller gains a verified-call signal.
Both earn strict full-C credit. Their execution in the saved one-Up case is
unproved.
PR #162 translates all 112 instructions of the call-free cell-field clear at
unit `0x68:+0x0000`. The PM independently matched the exact private DOS span,
four selector masks, signed-byte coordinates, wrapped far address, and one-byte
store. It promotes one unresolved row to verified and earns one strict full-C
game-routine credit. Its execution on the saved one-Up route remains unproved.

A later local review verifies unit `0x9e:+0x0de3` as a complete count-and-sides
dice helper with an exact SYSTEM RandSeed step. The documented unit `0x68`
movement and encounter callers use its result for game decisions. This promotes
one helper row to verified and raises provisional call evidence for 26 caller
rows; see [the roll audit](U9E_ENTRY8_ROLL.md). The local integration is
unpublished under the owner's no-external policy.

PR #276 promotes unit `0x9b:+0x0255` from provisional helper to verified
after independent full-C parity review of its 212-byte record-table leaf.
The saved unit `0x9b:+0x0000` party-insertion route calls it; both that
caller and `0x6c:+0x056c` gain verified-call signals but remain
provisional. One strict full-C game-routine credit is awarded. The
surrounding dispatch remains untranslated.

PR #279 promotes the adjacent call-free unit `0x9b:+0x0329` leaf
from provisional helper to verified after 140 original-byte/C full-RAM
and register parity cases. The conditional `0x9b:+0x0000` caller and
`0x6c:+0x056c` each gain one verified-call signal and remain
provisional. One strict full-C game-routine credit is awarded. The
saved first insertion skips this leaf, so execution there is unclaimed.

PR #281 promotes unit `0x9b:+0x0000` from provisional to verified after
independent review of its complete 431-byte body, three internal near calls,
112 original-byte/C full-RAM and register cases, and a saved native RAM
comparison. The C API exposes an aliased near-return diversion as a
`TRANSFER` state for a higher-level executor; it does not execute the altered
target. The regenerated classifier adds four verified-call signals without
changing those callers' classes. One strict full-C game-routine credit is
awarded after merge; the total game-function denominator remains unknown.

What the rules do not do: they do not identify third-party or standard library units. If a
library unit is linked into the overlay, its routines can fall into `helper_candidate`
(if they are leaves or runtime-only) or the unresolved class. The 142 unresolved
routines may contain such code; their split is not determined by these bytes.
No count here is a game-function denominator.

## 3. Linear decode, non-prologue starts and indirect flow

- The bounded linear decode between prologue candidates covers 196,236 of 197,451 unit code
  bytes (99.4%). This is linear disassembly, not reachable-code coverage: it says which
  bytes decode as instructions when read from each prologue, not which code the game
  executes. The 1,215 undecoded bytes fall in 184 ranges: 166 are 8 bytes or less
  (alignment and inline constants after returns); 712 bytes precede a unit's first prologue
  (typed constants that Turbo Pascal places at the start of a unit's code segment; the
  largest such ranges are unit 0x19 at offset 0 with 253 bytes and unit 0x86 with 138).
- A heuristic flags an undecoded range as a possible routine when it decodes completely,
  starts with a plausible opcode and contains a return. It flagged none of the 184 ranges.
  That is the extent of the claim: the tested heuristic found no callable start without a
  prologue; it does not prove none exists.
- One pattern, `jmp cs:[table]`, was searched for jump-table dispatch and found no sites.
  Together with the procedure-pointer evidence in the function ledger (three far calls
  through procedure variables, 208 pointer constants naming entry slots of prologue
  routines) this covers the indirect transfers that were tested, not all possible ones.
- The call graph contributes a per-row `far_in` count of fixup-confirmed far calls into each
  candidate; every such target resolves to a candidate after alias normalization, or the
  tool refuses to run.

## 4. Limits

- The string signal counts a routine that pushes any printable constant; error and prompt
  text from library code would count too. The engine-global list is the one verified so far;
  routines touching other game state fall into the unresolved class until those globals are
  documented.
- The linear decode stops at the next prologue; a routine whose body continues past inline
  data after an early return is measured only up to that data. This affects signal counts,
  not the start set.
- Dynamic confirmation (which routines run on which game paths) is not available until the
  DOS oracle (issue #3) delivers traces; reachable-code coverage needs those traces or a
  full control-flow walk, neither of which this tool performs.

## 5. Reproduce

```sh
G=resources/DOS/POOLRAD; O=/path/outside/repo
python3 tools/overlay/overlay_inventory.py --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/overlay_inventory.json --md $O/inv.md
python3 tools/overlay/callgraph.py --inventory $O/overlay_inventory.json --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/callgraph.json --md $O/cg.md
python3 -m unittest tools/overlay/test_classify_overlay.py
python3 tools/overlay/classify_overlay.py --inventory $O/overlay_inventory.json --callgraph $O/callgraph.json --ledger-csv docs/re/overlay_function_ledger.csv --ovr $G/GAME.OVR --csv docs/re/overlay_classification.csv --md docs/re/overlay_classification_summary.md
```
