# Skipped SAVE loader entry

`U25_SKIP_1EA6.md` and the recovered skip model identify the two-operand setup
at 1F23 and near call at 1F27 to unit 25 loader 0000, returning to 1F2A.
`pr_slot_a_skip_loader_entry` builds that live argument and far-return frame.
A malformed reached near call stops after the argument and CS pushes; invalid
setup or a stack/code overlap refuses before mutation. The caller authenticates
the seven-byte reference, keeps guest code immutable and supplies disjoint
host objects.

The synthetic shared-state chain checks the count word, return frame,
malformed-call partial state and preservation of the earlier SAVE. The
loader body is not executed here. Same-invocation provenance, the complete
Up route and native 345B parity remain unverified.
