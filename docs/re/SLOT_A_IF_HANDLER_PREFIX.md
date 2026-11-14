# IF >= false-handler prefix

The recovered handler and `U1C_IF_0B52.md` identify the 0B52 prologue,
script-PC increment, opcode checks, and predicate byte DS:6F7D. For opcode
1B with a zero predicate, the reached skip call is 0BC1. The preceding
synthetic COMPARE produces that predicate for 10 versus 14.

`pr_slot_a_if_handler_prefix` models this bounded path through the branch,
retaining the live saved BP and incremented script PC. It accepts only latch
1B and zero predicate and rejects stack/DS aliases; unsupported inputs and
reference mismatches refuse atomically. The caller authenticates the 111-byte
code span, keeps guest code immutable, and supplies disjoint host objects.
The synthetic shared-state chain checks the PC increment, reached 0BC1 stop,
preserved effects and unsupported-predicate refusal. It does not execute the
skip call, establish same-invocation provenance, complete the Up route, or
prove native 345B parity.
