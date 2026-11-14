# Conditional first SAVE store

`pr_slot_a_first_save_store` composes the SAVE `02C8` call and loaded `:0052`
stub into the raw U25 `0C77` store-word model for the exact argument frame
value `11`, address `6E7D`. It conservatively refuses stack/DS, window/DS and
window/stack overlap and bounds the stack profile, preventing aliases from
redirecting the classifier or hook into the raw model's potentially
nonterminating class-4 path. Unsupported frames, references and aliases refuse
without changes. Once invoked, the raw helper's actual status and reached
CPU/RAM are exposed; callers must check the return before proceeding.

The caller independently authenticates the call, stub, store body, hook and
classifier references and loaded bindings; guest writes must not modify code,
and host objects must be disjoint. Matching references does not prove
provenance. [The saved script audit](SLOT_A_UP_ECL_GATE.md) identifies the
first SAVE as `11 → 6E7D`; the retained script image identifies the loaded
`084F:0052 → 308A:0C77` route. Neither proves the current invocation. The
[raw store contract](U25_STORE_WORD_0C77.md) documents its other unresolved
paths. Synthetic tests verify the fixed window write and actual `02CD` return,
as well as frame/reference and alias refusals. The wider SAVE handler, other
addresses, same-invocation native binding and `345B` parity remain unproved.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_first_save_store.c src/recovered/overlay_u25_store_word_0c77.c tests/replay/test_slot_a_first_save_store.c -o /tmp/slot-a-first-save-store
/tmp/slot-a-first-save-store
```
