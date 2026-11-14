# Conditional ECL dispatcher prologue

`pr_slot_a_dispatch_prologue` continues a caller-supplied raw CPU/RAM state at
unit 1C `2F2D` through `PUSH BP`, `MOV BP,SP`, and the opcode-latch load,
stopping before `CMP AL,0` at `2F33`. It retains flags and observes stack/latch
aliasing in instruction order. The caller authenticates the six loaded code
bytes, supplies disjoint host objects and full 1 MiB RAM; bad inputs refuse
without changes. Code matching is conditional on the supplied reference, not
independent evidence of its provenance.

The entry instructions were decoded from the hash-pinned retained script image
listed in [the first EXIT audit](SLOT_A_FIRST_EXIT_9974.md). Synthetic tests
check full RAM on refusal and success, stack/latch aliasing, and composition
from [the first-dispatch runner boundary](SLOT_A_RUNNER_FIRST_DISPATCH.md).
The first opcode comparison and all dispatch branches, handler calls, service
effects and return remain unresolved here. No native same-invocation or 345B
parity is claimed.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_dispatch_prologue.c src/replay/slot_a_runner_first_dispatch.c src/recovered/overlay_u1c_run_3171_cpu.c tests/replay/test_slot_a_dispatch_prologue.c -o /tmp/slot-a-dispatch-prologue
/tmp/slot-a-dispatch-prologue
```
