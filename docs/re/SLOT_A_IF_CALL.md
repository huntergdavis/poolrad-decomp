# IF >= handler entry

The existing `U1C_COMPARE_IF_ENTRY_SHARED_BUS.md` contract identifies the
3021 PUSH CS and the 3022 near call to 0B52, with return IP 3025. The saved
script audit identifies the next operation as IF >=; this fixture supplies
the corresponding synthetic opcode after the numeric COMPARE.

`pr_slot_a_if_call` builds the live far-return frame. Entry-code or argument
refusal precedes mutation; a malformed near call stops at 3022 after the CS
push, exposing the partial state. The caller authenticates the four-byte
reference, keeps guest code immutable, and supplies disjoint host objects;
stack writes overlapping the call bytes are refused. The synthetic shared
chain checks both the reached 0B52 frame and malformed-call stop while
preserving earlier SAVE and comparison effects. It does not execute the IF
handler, establish same-invocation provenance, complete the Up route, or prove
native 345B parity.
