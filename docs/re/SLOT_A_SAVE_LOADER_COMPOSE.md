# Conditional SAVE operand-loader composition

`pr_slot_a_save_loader_compose` joins the conditional `:002A` stub to the
existing raw U25 `0000` operand loader on one CPU and RAM image. It validates
caller-supplied stub and 310-byte loader references, stack limits and positive
budgets before the stub transfer. The caller must independently authenticate
both references and the SYSTEM segment, ensure loaded code and bindings stay
immutable, and satisfy the raw loader's code/data nonalias and synchronous
execution contract. Matching supplied bytes alone does not prove provenance.

A precondition failure refuses with CPU, RAM and status unchanged. Once the
loader runs, its exact status and reached CPU/RAM are returned, including
operand/string budget, transfer and callee stops; no stop becomes a synthetic
return. See [the raw loader contract and differential evidence](U25_LOADER_0000.md)
and [the prior stub boundary](SLOT_A_SAVE_LOADER_STUB.md). Synthetic tests pass
two numeric operands through the stub and loader to a supplied `02A0` return,
and verify a one-operand budget stop and atomic reference refusal. They do not
establish a saved-game invocation, SYSTEM binding, native `345B` state, or
full-Up parity. The SAVE handler continuation after `02A0` remains unexecuted.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_save_loader_compose.c src/replay/slot_a_save_loader_stub.c src/recovered/overlay_u25_loader_0000.c src/recovered/overlay_u25_string_cores.c tests/replay/test_slot_a_save_loader_compose.c -o /tmp/slot-a-save-loader
/tmp/slot-a-save-loader
```
