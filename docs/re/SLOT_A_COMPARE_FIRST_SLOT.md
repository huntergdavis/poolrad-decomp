# First COMPARE numeric slot read

The retained script described in `SLOT_A_UP_ECL_GATE.md` has a COMPARE at
`991A` whose first operand is tag 01, address `49C9`, followed by immediate
14. The saved values described there give window 0 value 10. This is a
conditional continuation of the preceding synthetic shared-state chain; it
is not evidence that this script entry and the post-Up invocation occurred
together.

`pr_slot_a_compare_first_slot` models the reached call at 017E through the
bound unit 25 entry 0136 and exposes its actual status, CPU and RAM. It accepts
only tag 01 and address 49C9, authenticates caller-provided code references and
loaded far bindings, and refuses stack aliases with DS or the window word.
The caller must authenticate the references and ensure guest writes do not
overlap loaded code; other profiles are refused. The synthetic chain checks
that a prior SAVE effect survives and that the window value 10 reaches AX at
0186. It does not establish native 345B parity or a complete Up path.
