# Conditional SAVE operand-loader stub

`pr_slot_a_save_loader_stub` models a caller-authenticated loaded far-jump stub
at `:002A` whose destination offset is zero, then stops before its destination
body. It changes CS:IP only. The caller supplies disjoint host objects, full
1 MiB RAM, and an authenticated five-byte stub reference. Invalid inputs or a
nonmatching stub refuse without changes. This comparison does not establish
which overlay is loaded or that the destination body is the intended loader.

In the retained script-entry RAM whose hash is recorded in
[the first EXIT audit](SLOT_A_FIRST_EXIT_9974.md), the SAVE instruction points
to loaded `084F:002A`; that stub jumps to `308A:0000`. This is an observation
of that image, not proof of a current same-invocation binding. The preceding
conditional call frame is [documented here](SLOT_A_SAVE_LOADER_CALL.md), and
the raw loader's bounded contract is [documented here](U25_LOADER_0000.md).
Synthetic tests check full-RAM refusal and success, target shape, and call-frame
composition. The loader, its SYSTEM binding, and all its effects remain
unexecuted. No same-invocation or `345B` parity is claimed.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_save_loader_stub.c src/replay/slot_a_save_loader_call.c tests/replay/test_slot_a_save_loader_stub.c -o /tmp/slot-a-save-stub
/tmp/slot-a-save-stub
```
