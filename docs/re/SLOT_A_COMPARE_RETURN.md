# First COMPARE handler return

The raw caller attribution in `U1C_COMPARE_0126_CPU.md` identifies the
`019F..01A2` tail as `MOV SP,BP; POP BP; RETF`, with the normal dispatcher
continuation at `2F5C`. `pr_slot_a_compare_return` reads the live frame and
exposes its actual destination; invalid inputs, reference mismatch and a
wrapping stack-offset frame refuse atomically. The caller authenticates the
four-byte code reference and keeps host objects disjoint.

The synthetic shared-state chain checks the return to 2F5C while preserving
the earlier SAVE and word-comparison flags. It also checks a mismatched tail
and an altered return destination. This is conditional composition; it does
not establish same-invocation provenance, a complete Up route, or native
345B parity. Subsequent dispatcher and script instructions are not executed.
