# Conditional first COMPARE operand loader

`pr_slot_a_compare_loader` composes the COMPARE `0130` far-call frame into
the existing conditional `:002A` stub and raw U25 operand loader. It reports
call refusal before pushes, stub/loader precondition refusal with the reached
call frame retained, or the raw loader's reached status and CPU/RAM, including
budget and transfer stops. A reached status is not a claim of return to
`0135`; callers must check the raw status and actual CS:IP. The caller must
authenticate the fixed-size call/stub/body references and SYSTEM binding,
provide disjoint host objects, and satisfy the underlying loader's immutable
code and data/nonalias contract. Matching supplied references does not prove
provenance.

The earlier [COMPARE prefix](SLOT_A_COMPARE_HANDLER_PREFIX.md) stops at this
call; the reused raw loader contract is [documented here](U25_LOADER_0000.md).
The [saved script audit](SLOT_A_UP_ECL_GATE.md) identifies the first COMPARE
as `49C9` versus `14`. Synthetic tests exercise those supplied operand bytes,
a two-operand return, budget stop, call refusal and retained-frame stub
refusal; a linked shared-RAM test carries the prior SAVE chain into the loader.
These fixtures do not establish same-invocation saved-game provenance or
native full-RAM parity. The actual comparison, later script effects and
`345B` return remain unexecuted.

Build the two `poolrad_slot_a_compare_loader` test targets and run `ctest
--test-dir BUILD -R '^poolrad_slot_a_compare_loader(_chain)?_tests$'
--output-on-failure`.
