# Conditional first SAVE return

`pr_slot_a_save_epilogue` models the SAVE `02CD` jump and `02DC..02DF`
epilogue, restoring BP and taking CS:IP from the actual stack frame. It
accepts a non-wrapping six-byte stack-offset frame (`BP <= FFFA`) and full
1 MiB RAM. Invalid inputs or mismatched caller-authenticated code references
refuse without changes. The caller keeps host objects disjoint and is
responsible for reference provenance. Return words are read from live RAM;
an altered destination is exposed, not replaced by `2F89`.

The instruction attribution follows the existing raw candidate
`overlay_u1c_save_0292_cpu`. A synthetic shared-state test composes
[the first SAVE slot/store](SLOT_A_FIRST_SAVE_COMPOSE.md) through this
return: value `11` is written to the supplied window and the supplied frame
returns to `2F89`. The isolated test also checks refusal and an independently
supplied return. These are conditional synthetic results, not evidence of a
same-invocation saved-game run or native full-RAM parity. The dispatcher
continuation, subsequent script effects and `345B` return are not executed.

Run the linked tests with `ctest --test-dir BUILD -R
'poolrad_slot_a_(save_epilogue|first_save_return)_tests' --output-on-failure`
after building those two targets.
