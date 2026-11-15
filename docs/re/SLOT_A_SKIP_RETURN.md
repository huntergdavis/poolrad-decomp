# Skipped SAVE helper return

The recovered `overlay_u25_skip_1ea6.c` and `U25_SKIP_1EA6.md` identify the
1F2A jump to the 1FAF pop-BP/far-return tail after the two-operand loader.
The retained loaded image confirms a three-byte near jump at that site.

`pr_slot_a_skip_return` checks caller-authenticated jump and tail references,
reads the live stack frame and exposes the actual BP and far destination.
Invalid inputs and wrapping stack-offset frames refuse atomically. The caller
keeps guest code immutable and host objects disjoint. The synthetic shared
chain checks the return to 0BC6, prior effects, reference refusal and an
altered return destination. It does not execute the IF return, establish
same-invocation provenance, complete the Up route, or prove native 345B parity.
