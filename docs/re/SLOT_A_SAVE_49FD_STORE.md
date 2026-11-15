# SAVE store to 49FD

`SLOT_A_UP_ECL_GATE.md` identifies the next SAVE destination as 49FD. The
raw `U25_STORE_WORD_0C77.md` contract identifies its class-0 window write and
subsequent DS:84E8 flag write. The shared-state fixture uses source value 5
synthetically; it does not establish the saved script's actual source value.

`pr_slot_a_save_49fd_store` composes the raw helper for destination 49FD and
a caller-supplied value. It checks authenticated call, stub, body, hook and
classifier references and excludes the bounded stack frame from DS and the
window target, and the window target from DS. It exposes actual helper status
and CPU/RAM; unsupported profiles refuse atomically. The caller authenticates
references, keeps code immutable, and supplies disjoint host objects.

The synthetic chain checks the window word, 84E8 flag, helper return, prior
SAVE effect, and atomic window/DS-alias refusal. It does not execute the SAVE
return, establish same-invocation provenance, complete the Up route, or prove
native 345B parity.
