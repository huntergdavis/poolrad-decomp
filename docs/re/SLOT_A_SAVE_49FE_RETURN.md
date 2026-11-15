# 49FE SAVE return and next fetch

The hash-matched retained save identifies immediate 9 to 49FE at PC 992E and
the saved-script audit identifies COMPARE at 9934. The synthetic shared-state
chain reuses the existing SAVE epilogue, dispatcher return and runner adapters,
with a supplied COMPARE opcode at 9934. Earlier window value 11 remains
synthetic; component reference, binding, immutable-code, alias and host-object
contracts remain in force.

The fixture checks the return frames, next fetch, preserved window and flag
effects, epilogue-reference refusal and runner-jump refusal. It does not
execute the COMPARE body, establish same-invocation provenance, complete the
Up route, or prove native 345B parity.
