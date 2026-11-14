# First COMPARE dispatcher return

The existing `U1C_COMPARE_DISPATCH_RETURN_SHARED_BUS.md` contract identifies
the `2F5C` jump to the shared `316F` pop-BP/far-return tail. The retained
script-entry RAM has a three-byte near jump at that location. This adapter
uses caller-authenticated jump and tail references, reads the live return
frame, and exposes its actual destination. Invalid inputs, mismatched spans,
and wrapping stack-offset frames refuse atomically; host objects are disjoint.

The synthetic shared-state chain reaches the supplied runner return `31A8`
while preserving the earlier SAVE and numeric comparison effects. It checks
reference refusal. These conditional fixtures do not establish
same-invocation provenance, a complete Up route, or native 345B parity.
Subsequent runner and script instructions are not executed here.
