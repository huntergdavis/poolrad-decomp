# IF-equal after COMPARE 9934

The saved-script audit in `SLOT_A_UP_ECL_GATE.md` identifies IF-equal at 993A
after comparing 6DD5 with zero. `U1C_ECL_DISPATCH_PREFIX.md`,
`U1C_COMPARE_LATER_OPCODE_SHARED_BUS.md` and `U1C_IF_0B52.md` supply the
opcode checks and handler instruction attribution.

The bounded IF-equal dispatcher stages accept opcode 16 and stop at the IF
call site; the handler prefix accepts latch 16 and predicate byte 1, increments
the script PC and stops at the 0BC6 return tail. Other handler profiles refuse
atomically. Caller-authenticated references, immutable code, disjoint host
objects and the component alias restrictions remain required.

The synthetic chain composes COMPARE return, supplied IF opcode fetch,
dispatch and the true handler prefix, checking PC 993B, comparison flags,
prior effects and a predicate refusal. Its zero 6DD5 value is synthetic, not
a live measurement. The IF return and following GOTO are unexecuted;
same-invocation provenance, a complete Up route and native 345B parity remain
unverified.
