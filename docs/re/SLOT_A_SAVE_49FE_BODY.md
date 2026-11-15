# Retained-shape SAVE to 49FE

The hash-matched ignored save described in `SLOT_A_SAVE_49FE_DISPATCH.md`
identifies the SAVE at 992E as tag-00 immediate 9 to tag-01 address 49FE.
The synthetic shared-state chain composes the existing handler, loader, word,
tag and immediate-slot adapters, then a bounded class-0 raw store adapter.
Earlier window value 11 remains synthetic and is not a live measurement.

`pr_slot_a_save_49fe_store` accepts destination 49FE with a caller-supplied
value, checks authenticated loaded references, excludes the bounded stack frame
from DS and the window target, and excludes the window target from DS. It
exposes actual raw helper status and state; unsupported profiles refuse
atomically. The caller authenticates code and bindings, keeps code immutable,
and supplies disjoint host objects.

The chain checks the loader and helper statuses, PC 9934, window word 9,
prior effects, window/DS-alias refusal, and a separate 84E8 zero-to-one
witness. It stops before the SAVE return and does not establish same-invocation
provenance, a complete Up route, or native 345B parity.
