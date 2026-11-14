# Conditional first ECL dispatcher branch

`pr_slot_a_dispatch_first_branch` continues the conditional dispatcher
prologue at `2F33`, executing `CMP AL,0` and `JNE`. It stops before the EXIT
call setup at `2F37` when AL is zero, or before the next comparison at `2F3E`
otherwise. It changes arithmetic flags and IP only; it does not invoke an
opcode handler or service. Caller-authenticated loaded code, a full 1 MiB RAM
image, and disjoint host objects are required. Invalid inputs refuse without
changes.

The instruction sequence was decoded from the hash-pinned retained script
image identified in [the first EXIT audit](SLOT_A_FIRST_EXIT_9974.md); the
preceding model is [the dispatcher prologue](SLOT_A_DISPATCH_PROLOGUE.md).
The synthetic test checks all 256 AL values, flags, full-RAM preservation,
refusal, and composition from that prologue. This is conditional execution,
not a native same-invocation comparison or proof of a `345B` return.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_dispatch_first_branch.c src/replay/slot_a_dispatch_prologue.c tests/replay/test_slot_a_dispatch_first_branch.c -o /tmp/slot-a-first-branch
/tmp/slot-a-first-branch
```
