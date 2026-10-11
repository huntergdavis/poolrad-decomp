# Overlay routine classification: game, helper, or unresolved (issue #26, part 2)

**Scope.** A bounded, reproducible classification of the 673 evidence-backed overlay routine
candidates from [OVERLAY_FUNCTION_LEDGER.md](OVERLAY_FUNCTION_LEDGER.md), for the DOS v1.3
files identified in [overlay_inventory.md](overlay_inventory.md) (`GAME.OVR` sha256
`872964a3…928614`, `START.EXE` sha256 `d825df2b…6a07bd`). It measures byte-level signals per
routine and applies fixed rules; it does not turn prologue hits into a game-function
denominator. Per-address rows: [overlay_classification.csv](overlay_classification.csv);
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
| documented engine globals | DS-relative operands equal to byte-verified engine state addresses from the ledgers and contracts (ECL PC, buffer, opcode latch, stop flags, call stack, compare flags, operand slots, the three windows, party and active-character pointers, window state, the procedure variable) | 248 |
| calls into verified routines | direct near calls, or far calls through entry slots, to a byte-verified routine | 55 |
| verified routine | the routine itself is byte-verified: the ECL loop, dispatcher and its 49 handlers, the eleven unit 0x25 helpers and the window initializer | 62 |
| leaf | no calls and no DS-relative globals | 70 |
| runtime-only | every call goes to the SYSTEM segment 0xAF8, no DS-relative globals | 16 |

## 2. Classes and rules

| class | rule (applied in this order) | confidence | count |
| --- | --- | --- | ---: |
| `game_verified` | verified routine | verified (identity and role; semantics verified only for the translated subset in FUNCTION_LEDGER.md) | 62 |
| `game_evidenced` | string constant, documented engine global, or call into a verified routine | provisional: printable text and engine state are evidence of application code, not proof of game logic | 302 |
| `helper_candidate` | leaf or runtime-only, no strings, no DS globals, not verified | provisional: small utilities; a leaf can still be game arithmetic | 82 |
| `game_or_other_library_unresolved` | everything else (calls resident game-code segments or overlay routines, or touches undocumented globals, without text or verified engine state) | unresolved | 227 |

By evidence grade from the function ledger: `game_verified` 6 A, 55 B, 1 D; `game_evidenced`
183 A, 118 B, 1 D; `helper_candidate` 60 A, 22 B; unresolved 153 A, 74 B.

What the rules do not do: they do not identify third-party or standard library units. If a
library unit is linked into the overlay, its routines fall into `helper_candidate` (if they
are leaves or runtime-only) or into the unresolved class; the 227 unresolved routines are
exactly where such code would have to be, and their split is not determined by these bytes.
No count here is a game-function denominator.

## 3. Coverage, non-prologue starts and indirect flow

- The bounded decode reaches 196,236 of 197,451 unit code bytes (99.4%). The 1,215
  uncovered bytes fall in 184 ranges: 166 are 8 bytes or less (alignment and inline
  constants after returns); 712 bytes precede a unit's first prologue (typed constants that
  Turbo Pascal places at the start of a unit's code segment; the largest such ranges are
  unit 0x19 at offset 0 with 253 bytes and unit 0x86 with 138).
- No uncovered range decodes as a self-contained, return-terminated routine, so no callable
  start without a prologue exists in the uncovered bytes.
- No `jmp cs:[table]` dispatch site exists in the overlay; the only indirect transfers are
  the three far calls through procedure variables documented in the function ledger, whose
  208 pointer constants all name entry slots of prologue routines.

## 4. Limits

- The string signal counts a routine that pushes any printable constant; error and prompt
  text from library code would count too. The engine-global list is the one verified so far;
  routines touching other game state fall into the unresolved class until those globals are
  documented.
- The linear decode stops at the next prologue; a routine whose body continues past inline
  data after an early return is measured only up to that data. This affects signal counts,
  not the start set.
- Dynamic confirmation (which routines run on which game paths) is not available until the
  DOS oracle (issue #3) delivers traces.

## 5. Reproduce

```sh
G=resources/DOS/POOLRAD; O=/path/outside/repo
python3 tools/overlay/overlay_inventory.py --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/overlay_inventory.json --md $O/inv.md
python3 tools/overlay/callgraph.py --inventory $O/overlay_inventory.json --exe $G/START.EXE --ovr $G/GAME.OVR --json $O/callgraph.json --md $O/cg.md
python3 -m unittest tools/overlay/test_classify_overlay.py
python3 tools/overlay/classify_overlay.py --inventory $O/overlay_inventory.json --callgraph $O/callgraph.json --ledger-csv docs/re/overlay_function_ledger.csv --ovr $G/GAME.OVR --csv docs/re/overlay_classification.csv --md docs/re/overlay_classification_summary.md
```
