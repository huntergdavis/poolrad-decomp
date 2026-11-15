# IF handler return after skip

The recovered `overlay_u1c_if_0b52.c` and `U1C_IF_0B52.md` identify the
0BC6 pop-BP/far-return tail. `pr_slot_a_if_return` validates the caller-
authenticated two-byte reference and reads the live frame, exposing the
actual BP and far destination. Invalid inputs and wrapping stack-offset
frames refuse atomically; the caller keeps guest code immutable and host
objects disjoint.

The synthetic shared-state chain checks return to dispatcher 3025, retained
skip and earlier SAVE effects, a reference refusal, and an altered return
destination. It does not execute the following dispatcher return, establish
same-invocation provenance, complete the Up route, or prove native 345B parity.
