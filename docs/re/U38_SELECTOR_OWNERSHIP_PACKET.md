# U38 selector/effect handler ownership packet

Shared ownership evidence for the U38 effect-handler family, so each row is
checked against one derivation instead of eight.

## What this establishes

A row listed here is a **game function**, not library code, because its address
is fixed by two independent game-side structures:

1. **The overlay public-entry table.** START.EXE's unit-0x38 entry table maps an
   entry index to an original far stub; the stub is the address the game calls.
2. **The effect-table installer** `U38_EFFECTS_INSTALL_31A2.md`
   (`game_or_other_library_unresolved` by classifier, but documented as the game
   effect initializer): 138 four-MOV pointer groups writing `DS:682C..6A57`,
   consumed by the dispatcher at `DS:6828 + 4*type`.

Both must agree on the stub for the mapping to count as ownership evidence. The
classifier cannot see either structure, which is why these rows are
`game_evidenced` (application code) rather than `game_verified` until a reviewer
writes the reviewed-identity row.

## Verified mapping (original bytes, GAME.OVR `872964a3…`, START.EXE `d825df2b…`)

| handler | dispatch slot | table addr | original stub | entry index |
| --- | ---: | --- | --- | ---: |
| `OVL_0038_12B2` | 49 | — | `0041:00ED` | — |
| `OVL_0038_25BC` | 94 | `69A0` | `0041:01A6` | 78 |
| `OVL_0038_264B` | 96 | — | `0041:01B0` | — |
| `OVL_0038_2A87` | 115 | — | `0041:020F` | — |
| `OVL_0038_2AE6` | 116 | — | `0041:0214` | — |
| `OVL_0038_2B20` | 117 | — | `0041:0219` | — |
| `OVL_0038_2B7D` | 119 | `—` | `0041:0223` | — |
| `OVL_0038_2D88` | 123 | — | `0041:0237` | — |

Slot, stub and table address come from `u9e_dispatch_handlers.json` (139 bound
slots / 118 distinct handlers); the entry index comes from the function ledger's
`entry` column. `—` marks fields the current inventory does not carry for that
row; absence is reported, not filled in.

## Selector path (why these rows are effects, not utilities)

The handlers are reached as `DS:5D92` (active character) → the checked
`+0x101D` selector → resident `00BA:2E8B` selection → the per-handler
field/effect gate. That chain is already strict-credited
(`U38_TARGET_SELECT_101D.md`, `RESIDENT_CHARACTER_TARGET_SELECT.md`), so a
handler that reads the selected character through it is operating on game state.

## How to apply it

For each row: (a) confirm the row's slot/stub pair here; (b) run that row's own
body audit (unmasked CPU14 + full 1 MiB, 100% instruction starts, all feasible
conditional outcomes, unreached starts/edges enumerated and kept in the
denominator, exact extent with trailing data excluded); (c) only then write the
reviewed-identity and strict rows. **Ownership does not substitute for the body
audit, and the packet is not a blanket credit.**

## Limits

The installer's own classification stays `game_or_other_library_unresolved`; the
packet uses it as a documented game-side structure, not as a classifier override.
An entry index or a slot alone is not sufficient — both must resolve to the same
stub. No row here is credited by this document.
