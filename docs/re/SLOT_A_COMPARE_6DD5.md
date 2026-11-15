# COMPARE 6DD5 with zero

The hash-matched retained save described in `SLOT_A_COMPARE_9934_LOADER.md`
uses tag-01 address 6DD5 and immediate zero at PC 9934. The class-1 load hook
for 6DD5 belongs to the quiet set in `ECL_LOAD_HOOK_CLASSIFIER.md`.

`pr_slot_a_compare_6dd5_slot` bounds the raw slot reader to that address and
checks caller-authenticated loaded references and stack/DS/window exclusions.
It exposes actual helper status and state; unsupported profiles refuse
atomically. The caller authenticates bindings, keeps guest code immutable and
supplies disjoint host objects. The synthetic chain then reuses the immediate
second-slot and raw word-compare adapters.

The chain checks comparison bytes for a synthetic zero window value, a
separate nonzero source read and stack/window refusal. The saved-state audit
reports zero for its saved input, but this fixture does not measure a live
source value or establish same-invocation provenance. IF-equal execution,
the complete Up route and native 345B parity remain unverified.
