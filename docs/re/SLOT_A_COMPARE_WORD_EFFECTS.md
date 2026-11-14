# First COMPARE word effects

The retained script audit in `SLOT_A_UP_ECL_GATE.md` identifies the first
COMPARE as window 0 value 10 versus literal 14. The caller instruction and
frame attribution is in `U1C_COMPARE_0126_CPU.md`; the raw word helper's
semantics and validation are in `OVL_0025_18EB_CPU.md`.

`pr_slot_a_compare_word_effects` continues at 0191, stores the second local,
pushes both operands, and calls the existing 18EB model. It exposes the helper
status and actual CPU/RAM, including altered transfers. Its bounded frame
excludes DS/stack aliases; callers authenticate the supplied instruction,
stub, body and FillChar spans, maintain immutable code, and use disjoint host
objects. Unsupported bindings and frames refuse atomically. The synthetic
shared-state test checks the six unsigned relation bytes for 10 versus 14 and
the earlier SAVE effect. It does not establish same-invocation provenance, a
complete Up path, or native 345B parity.
