# Conditional COMPARE call and handler prefix

`pr_slot_a_compare_handler_prefix` models the conditional dispatcher call at
`2F58` and COMPARE `0126..012F` prologue: it builds the far-return frame,
saves BP, allocates `0204` stack bytes with x86 subtraction flags, sets AL to
two and pushes AX. It stops before the operand-loader call at `0130`. The
caller supplies full 1 MiB RAM, disjoint host objects and independently
authenticated call/body references. An unsupported stack profile or overlap
between written stack words and the guarded code refuses before changes.
Reference matching does not establish provenance.

Instruction attribution and the broader helper limits are documented in
[the raw COMPARE candidate](U1C_COMPARE_0126_CPU.md); this slice does not run
its loader or potentially unbounded slot-value paths. A synthetic linked test
continues [the prior SAVE-to-COMPARE chain](SLOT_A_COMPARE_DISPATCH.md) through
this prefix, retaining the SAVE window write. The isolated test covers full
RAM, frame, flags and refusal. These fixtures do not establish a same-invocation
saved-game execution or native full-RAM parity. Loader, comparison effects,
later script operations and `345B` return remain unexecuted.

Build the two `poolrad_slot_a_compare_handler` test targets and run `ctest
--test-dir BUILD -R '^poolrad_slot_a_compare_handler_(prefix|chain)_tests$'
--output-on-failure`.
