# Conditional COMPARE tag tests

`pr_slot_a_compare_tag_tests` models COMPARE `0135..0142`, testing the first
live tag against `80` and testing the second only when the first is numeric.
It stops at string setup `0143` or numeric setup `017E`, retaining flags from
the last reached comparison. The caller supplies full 1 MiB RAM, disjoint
host objects and independently authenticated fourteen-byte code; invalid
inputs refuse without changes. Reference matching does not establish
provenance.

The retained script image identified in [the first EXIT audit](SLOT_A_FIRST_EXIT_9974.md)
and the [raw COMPARE candidate](U1C_COMPARE_0126_CPU.md) supply instruction
attribution. The preceding conditional loader is [documented here](SLOT_A_COMPARE_LOADER.md).
Synthetic linked tests cover each tag across all 256 values, first-tag
short-circuiting, flags, refusal, and the shared SAVE-to-COMPARE chain through
the numeric branch. They do not establish same-invocation saved-game provenance
or native full-RAM parity. Slot reads, comparison effects, later script
operations and `345B` return remain unexecuted.

Build the two `poolrad_slot_a_compare_tag` test targets and run `ctest
--test-dir BUILD -R '^poolrad_slot_a_compare_tag_(unit|chain)_tests$'
--output-on-failure`.
