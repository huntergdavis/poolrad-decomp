# Conditional SAVE operand-word arguments

`pr_slot_a_save_word_args` models unit 1C `02A0..02A7`: load the live byte
at DS:`7087` into AL and push AX, then load DS:`7047` into AL and push AX.
AH and flags remain unchanged. It stops at `02A8` before the operand-word
helper call. Data and stack aliases follow instruction order. The caller
supplies full 1 MiB RAM, disjoint host objects, and an independently
authenticated eight-byte code reference. Stack/code overlap and invalid
inputs refuse before changes. Matching supplied bytes does not prove their
provenance.

Instruction attribution follows the existing raw candidate
`overlay_u1c_save_0292_cpu`; its helpers are not invoked here. The previous
conditional loader boundary is [documented here](SLOT_A_SAVE_LOADER_COMPOSE.md).
Synthetic checks cover full-RAM output, atomic refusal, and a first push that
changes the second operand before it is read. The helper call, handler return,
same-invocation native binding and `345B` parity remain unresolved.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_save_word_args.c tests/replay/test_slot_a_save_word_args.c -o /tmp/slot-a-save-word-args
/tmp/slot-a-save-word-args
```
