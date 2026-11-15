# SAVE dispatch at retained PC 992E

The hash-matched ignored save identified in `SLOT_A_SAVE_RETAINED_SHAPE.md`
contains a SAVE at 992E with tag-00 immediate 9 and tag-01 destination 49FE;
the saved-script audit identifies the following PC as 9934. The synthetic
shared-state fixture supplies that operand shape and reuses the existing
SAVE dispatcher adapters to reach handler 0292. It checks the handler frame,
prior effects and call-reference refusal. Their authenticated-reference,
immutable-code, stack and disjoint-host contracts remain in force.

The handler has not consumed these operands in this test. Earlier window
value 11 remains synthetic; matching the retained operand shape does not
establish same-invocation provenance, a complete Up route or native 345B parity.
