# Conditional SAVE store arguments

`pr_slot_a_save_store_args` models word SAVE `02BF..02C7`: store AX at
SS:`BP-2`, push the live word at `BP-4`, then push the live word at `BP-2`.
It stops before the store-word helper call at `02C8`. Stack/local aliases
follow instruction order, including a first push that alters the second read.
The caller supplies full 1 MiB RAM, disjoint host objects, and independently
authenticated nine-byte code. Stack/code overlap and invalid inputs refuse
before changes; matching a supplied reference does not prove its provenance.

Instruction attribution follows the existing raw candidate
`overlay_u1c_save_0292_cpu`; the preceding conditional slot-value model is
[documented here](SLOT_A_SAVE_SLOT_VALUE.md). Synthetic tests cover full-RAM
output, atomic refusal and live-read aliasing. The actual store helper, SAVE
return, same-invocation native binding and `345B` parity remain unresolved.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_save_store_args.c tests/replay/test_slot_a_save_store_args.c -o /tmp/slot-a-save-store-args
/tmp/slot-a-save-store-args
```
