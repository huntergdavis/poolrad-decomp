# DOS v1.3 game-function denominator: audited range

This note distinguishes the **audited candidate set** from a whole-game
function count. The inputs are the owner-supplied DOS v1.3 `START.EXE`
(SHA-256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`)
and `GAME.OVR`
(SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`).
Original bytes remain under ignored `resources/`.

## Counts that can be reconciled now

| Region | Audited starts | Ownership evidence | Treatment |
| --- | ---: | --- | --- |
| Overlay unit code | 673 | 402 entry/far-call grade A, 269 decoded near-call grade B, two public-alias grade D | Supported procedure starts; game versus linked-library ownership has not been classified for every row. |
| Resident code outside SYSTEM | 145 | Ten game verified, nine linked CRT inferred, 126 unresolved | Candidate starts: 86 overlay-called `RES_`, 48 other prologues `RPRO_`, 11 near-call-only `RNEAR_`. Not an exhaustive resident scan. |
| `0xAF8` SYSTEM/runtime | 48 distinct overlay-called targets | Segment classification | Excluded from the candidate set. This is a called-target count, not the total number of SYSTEM procedures. |

The overlay's 404 public entry slots overlap the 673 starts. Five encoded
`+0x800` alias slots normalize to existing starts and add **zero** functions.
Two prologue byte patterns in the overlay file header are data, outside the
673 unit-code starts. The resident `RES_`, `RPRO_`, and `RNEAR_` rows are
distinct in the current ledger; the 48 SYSTEM targets are separate. These
partitions must not be added again to the counts above.

The reviewed game-identity floor is **123**: 113 overlay identities and ten
resident identities. Of these, **61** game-specific routines have strict,
reviewed full-C translations. Bounded C slices and models, including the
`RES_0464:02A5` route slices, add no identity or full-C credit merely by
compiling; the game/library ownership of `0x464` support code remains open.

Within the **818 currently audited non-SYSTEM candidates** (`673 + 145`),
the conservative game-classification range is **123–818**: 123 verified game
identities at the floor, and no candidate discarded solely on a library
inference at the ceiling. If the nine high-confidence but unverified resident
CRT classifications are accepted as exclusions, the working candidate-set
range narrows to **123–809** (`673 + 10 + 126`). Both upper figures assume
every remaining candidate is a distinct game function; they are deliberately
loose. They are **not whole-game upper bounds or completion denominators**.
There may be further resident starts outside the candidate-seeded walk,
nonprologue entries, indirect targets, or code in unclassified executable
regions. Conversely, some candidates may eventually prove to be linked
library code or data.

## What closes the whole-game denominator

1. **Complete executable-region coverage in `START.EXE`.** Classify copied,
   filled, stub, data, and executable ranges using the loader map; expand
   direct-call closure from startup and all validated entries. Resolve the
   currently unentered resident block at image `0x8A9C`, indirect SYSTEM
   calls, and any nonprologue starts before calling the resident set complete.
2. **Classify ownership for each candidate.** The overlay ledger has no
   per-row game/library decision; the resident ledger still has 126
   unresolved rows and nine inferred CRT rows. Use call neighborhoods,
   decoded effects, data references, and linked-unit provenance. Record
   evidence per address rather than inferring ownership from a DS global,
   a short leaf, or a unit name alone.
3. **Close aliases and duplicate identities.** Preserve the five normalized
   overlay alias slots as aliases, test any resident alias/thunk suspicions,
   and ensure procedure-pointer and indirect destinations resolve to one
   canonical start each. A C helper or multiple C slices of one DOS routine
   must not increase the original-function count.
4. **Reconcile one canonical ledger.** Give each original start a region,
   canonical address, incoming-edge evidence, game/runtime/library status,
   uncertainty, and full/partial/untranslated C coverage. Only then divide
   verified full-C game routines by the verified game-function denominator.

Reproduction without publishing media: `docs/re/overlay_function_ledger.csv`
has 673 rows with grades A=402, B=269, D=2;
`docs/re/resident_callable_ledger.csv` has 145 rows with classifications
game_verified=10, linked_crt_inferred=9, unresolved=126. Their generation
and byte/provenance checks are documented in
[the overlay ledger](OVERLAY_FUNCTION_LEDGER.md) and
[resident callable coverage](RESIDENT_CALLABLE_COVERAGE.md). The 123/61
reviewed progress floors are maintained in [the battle card](../BATTLE_CARD.md).
