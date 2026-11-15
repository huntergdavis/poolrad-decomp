# Next SAVE handler and loader

The saved-script audit `SLOT_A_UP_ECL_GATE.md` identifies a SAVE at PC 9927
with destination 49FD. The shared-state fixture supplies a synthetic immediate
source value and numeric address operand. It composes the existing SAVE
handler prefix, loader call and loaded-stub/raw-loader adapters, retaining
those components' caller-authenticated references, immutable-code, binding,
alias and disjoint-host requirements.

The fixture checks the actual loader status, returned frame, PC and operand
tables, and a prefix-reference refusal. It does not execute the SAVE store or
return, establish same-invocation provenance, complete the Up route, or prove
native 345B parity. Its source value is not a claim about the saved script.
