# Conditional SAVE handler prefix

`pr_slot_a_save_handler_prefix` models unit 1C `0292..029A`: it pushes BP,
sets BP, allocates four stack bytes with x86 subtraction flags, sets AL to two,
and pushes AX. It stops at `029B` before the operand-loader LCALL. Stack and
physical addresses wrap as in the existing raw SAVE model. The caller supplies
full 1 MiB RAM, disjoint host objects, and an independently authenticated
nine-byte loaded reference. Invalid inputs refuse without changes; matching
a supplied reference does not establish its provenance.

The instruction attribution is from the existing
`overlay_u1c_save_0292_cpu` candidate model, whose loader helpers are doubles;
this slice stops before those helpers. The prior call-frame boundary is
[documented here](SLOT_A_SAVE_CALL_FRAME.md). Synthetic checks cover refusal,
full RAM and CPU output, and composition from the call frame. The loader,
handler return, same-invocation native parity and `345B` handoff remain
unresolved.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_save_handler_prefix.c src/replay/slot_a_save_call_frame.c tests/replay/test_slot_a_save_handler_prefix.c -o /tmp/slot-a-save-prefix
/tmp/slot-a-save-prefix
```
