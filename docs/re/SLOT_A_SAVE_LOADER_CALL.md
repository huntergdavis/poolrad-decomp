# Conditional SAVE operand-loader call frame

`pr_slot_a_save_loader_call` executes the SAVE handler's far call at `029B`,
pushing the caller segment and return IP `02A0`, and stops at the segment in
the loaded instruction, offset `002A`. The caller authenticates the five-byte
loaded reference and provides disjoint host objects and full 1 MiB RAM. The
model checks that the loaded instruction is a far call to offset `002A` and
refuses stack/code overlap before writes. Matching caller-supplied bytes does
not establish their provenance, the stub's identity, or its residency.

The existing candidate SAVE handler attributes this call to unit 25's operand
loader; see [the raw loader contract](U25_LOADER_0000.md) and
[the preceding SAVE prefix](SLOT_A_SAVE_HANDLER_PREFIX.md). Synthetic tests
check atomic refusal, full RAM/frame state, code/stack alias refusal and
composition from the prefix. The loaded stub and operand loader remain
unexecuted. No same-invocation native or `345B` parity is claimed.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_save_loader_call.c src/replay/slot_a_save_handler_prefix.c tests/replay/test_slot_a_save_loader_call.c -o /tmp/slot-a-save-loader-call
/tmp/slot-a-save-loader-call
```
