# IF >= later dispatcher checks

The existing `U1C_COMPARE_LATER_OPCODE_SHARED_BUS.md` evidence and provider
identify the later opcode checks from 2F8C. For opcode 1B, the last check is
CMP AL,1B and the reached call site is 3021. The saved-script audit identifies
IF >= as the next operation; its opcode mapping is in `ECL_CONTRACTS.md`.

`pr_slot_a_if_later_dispatch` models this bounded opcode-1B path after the
preceding dispatcher prefix. It validates a caller-authenticated 149-byte code
reference and refuses unsupported inputs atomically. The caller maintains
immutable guest code and disjoint host objects. The synthetic shared-state
chain checks flags, the 3021 stop, reference refusal, and preservation of the
prior SAVE and numeric comparison bytes. It does not execute the IF call or
handler, establish same-invocation provenance, complete the Up route, or prove
native 345B parity.
