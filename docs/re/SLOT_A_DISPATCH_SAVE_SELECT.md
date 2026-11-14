# Conditional SAVE dispatcher selection

`pr_slot_a_dispatch_save_select` accepts only AL `09` at unit 1C `2F3E`.
It models the comparisons and branches through the matching `CMP AL,09`, then
stops at `2F85`, before the SAVE call frame is pushed. Other opcodes, missing
backing, mismatched code and invalid entry refuse without CPU or RAM changes.
The caller authenticates the 71-byte loaded reference, supplies a full 1 MiB
image and keeps host objects disjoint. Matching the caller's reference does not
establish its provenance.

The path was decoded from the hash-pinned retained script image listed in
[the first EXIT audit](SLOT_A_FIRST_EXIT_9974.md), whose projected script trace
starts at `9914` with SAVE. The preceding model is
[the first dispatcher branch](SLOT_A_DISPATCH_FIRST_BRANCH.md). Synthetic tests
check all non-09 opcode refusals, the final CPU/RAM state and composition from
the prologue. The SAVE call frame, handler, services and runner return are not
executed; no same-invocation native or `345B` parity is claimed.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_dispatch_save_select.c src/replay/slot_a_dispatch_first_branch.c src/replay/slot_a_dispatch_prologue.c tests/replay/test_slot_a_dispatch_save_select.c -o /tmp/slot-a-save-select
/tmp/slot-a-save-select
```
