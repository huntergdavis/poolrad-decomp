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
| calls into verified routines | direct near calls, or far calls through entry slots, to a byte-verified routine | 73 |
| verified routine | the routine itself is byte-verified: the ECL loop, dispatcher and its 49 handlers, the reviewed unit 0x25 helpers, the window initializer, five unit 0x38 entries and the unit `0x3bd:0x0155` view setter in [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md), sixteen unit 0x38 entries in [OVERLAY_UNIT38_EFFECT_ENTRIES.md](OVERLAY_UNIT38_EFFECT_ENTRIES.md) (PR #63), four more in [OVERLAY_UNIT38_EFFECT_ENTRIES_NEXT.md](OVERLAY_UNIT38_EFFECT_ENTRIES_NEXT.md) (PR #69), and entries 81 and 110 in [OVERLAY_UNIT38_WALK_ENTRIES.md](OVERLAY_UNIT38_WALK_ENTRIES.md) (PR #85), plus the class-1 store hook (PR #96), unit `0x3bd:+0x0e08` (PR #106), two comparison helpers (PR #108), unit `0x3bd:+0x10c7` (PR #114), unit `0x3bd:+0x057a` (PR #124), unit `0x3bd:+0x047b` (PR #126), unit `0x3bd:+0x0350` (PR #128), unit `0x7f:+0x0130` (PR #130) and `+0x0185` (PR #132, ownership reviewed after merge), unit `0x25:+0x19db` (PR #136), and unit `0x68:+0x068d` (PR #138) | 102 |
| leaf | no calls and no DS-relative globals | 70 |
| runtime-only | every call goes to the SYSTEM segment 0xAF8, no DS-relative globals | 16 |

## 2. Classes and rules

| class | rule (applied in this order) | confidence | count |
| --- | --- | --- | ---: |
| `game_verified` | verified routine | verified (identity and role; semantics verified for the translated subset in FUNCTION_LEDGER.md and both unit 0x38 effect-entry notes) | 102 |
| `game_evidenced` | string constant, documented engine global, or call into a verified routine | provisional: printable text and engine state are evidence of application code, not proof of game logic | 297 |
| `helper_candidate` | leaf or runtime-only, no strings, no DS globals, not verified | provisional: small utilities; a leaf can still be game arithmetic | 80 |
| `game_or_other_library_unresolved` | everything else (calls resident game-code segments or overlay routines, or touches undocumented globals, without text or verified engine state) | unresolved | 194 |

By evidence grade from the function ledger: `game_verified` 44 A, 57 B, 1 D; `game_evidenced`
178 A, 118 B, 1 D; `helper_candidate` 58 A, 22 B; unresolved 122 A, 72 B.
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

What the rules do not do: they do not identify third-party or standard library units. If a
library unit is linked into the overlay, its routines fall into `helper_candidate` (if they
are leaves or runtime-only) or into the unresolved class; the 194 unresolved routines are
exactly where such code would have to be, and their split is not determined by these bytes.
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
