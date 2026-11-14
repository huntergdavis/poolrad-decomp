# Conditional SAVE numeric slot value

`pr_slot_a_save_slot_value` composes SAVE `02B7` argument setup and far call
through a loaded `:0025` jump to the raw U25 `0136` slot-value model. It
accepts only slot-1 tag `02`, and conservatively refuses stack overlap with the
three slot bytes that choose and feed that path. This prevents the unbounded
party-list path from being selected through stack aliasing. It exposes the
actual helper status and reached CPU/RAM, including an altered return; callers
must check the return before continuing. Invalid or unsupported inputs refuse
atomically. The caller authenticates all four code references and loaded
bindings, supplies disjoint host objects, and ensures guest writes do not
modify code. Reference matching does not establish provenance.

In the retained script-entry image recorded in
[the first EXIT audit](SLOT_A_FIRST_EXIT_9974.md), the call targets
`084F:0025`, whose stub jumps to `308A:0136`; this does not establish the
binding for a current invocation. The reused complete slot-value model and
its raw-state limitations are documented in [U25 slot value](U25_SLOT_VALUE_0136.md).
Synthetic tests cover a numeric `1234` return, short-reference and unsupported
tag refusal, and stack/table alias refusal. Other tag paths and the subsequent
SAVE store are not executed. No same-invocation native or `345B` parity is
claimed.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_save_slot_value.c src/recovered/overlay_u25_slot_value_0136.c src/recovered/overlay_u25_entry1_0136.c src/recovered/overlay_u25_load_word_0e0c.c src/recovered/overlay_u25_party_index_0756.c tests/replay/test_slot_a_save_slot_value.c -o /tmp/slot-a-save-slot-value
/tmp/slot-a-save-slot-value
```
