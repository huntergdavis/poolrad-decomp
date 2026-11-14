# Conditional SAVE call frame

`pr_slot_a_save_call_frame` models unit 1C `2F85` PUSH CS and `2F86` near
CALL, stopping at SAVE handler entry `0292`. It pushes CS and return IP `2F89`
in order, including 16-bit SP and 20-bit physical-address wrapping; it leaves
flags unchanged. The caller authenticates the four loaded code bytes, supplies
full 1 MiB RAM, and keeps host objects disjoint. Invalid inputs refuse without
changes. Matching a supplied code reference does not establish its provenance.

The instruction sequence was decoded from the hash-pinned retained script
image identified in [the first EXIT audit](SLOT_A_FIRST_EXIT_9974.md), and the
preceding conditional path is [SAVE selection](SLOT_A_DISPATCH_SAVE_SELECT.md).
Synthetic tests check refusal and full RAM, frame words, wrapping, and selection
composition. The SAVE handler is unexecuted; no same-invocation native or
`345B` parity is claimed.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_save_call_frame.c src/replay/slot_a_dispatch_save_select.c tests/replay/test_slot_a_save_call_frame.c -o /tmp/slot-a-save-call
/tmp/slot-a-save-call
```
