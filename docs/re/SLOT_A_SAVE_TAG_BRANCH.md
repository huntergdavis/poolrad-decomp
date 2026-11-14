# Conditional SAVE local store and tag branch

`pr_slot_a_save_tag_branch` models SAVE `02AD..02B6`: store AX at
SS:`BP-4`, compare the live tag at DS:`7006` with `80`, and branch. It stops
before the word path at `02B7` or string path at `02CF`. The store precedes
the read even when stack and tag alias. The caller supplies full 1 MiB RAM,
disjoint host objects and an independently authenticated ten-byte code
reference. Invalid inputs and stack/code overlap refuse without changes.
Matching supplied bytes does not establish their provenance.

Instruction attribution is from the existing raw candidate
`overlay_u1c_save_0292_cpu`; the preceding conditional helper is
[documented here](SLOT_A_SAVE_WORD_HELPER.md). Synthetic tests cover all 256
tag values and arithmetic flags, full-RAM changes, refusal, and stack/tag
aliasing. Neither branch's helper or the SAVE return is executed. No
same-invocation native or `345B` parity is claimed.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_save_tag_branch.c tests/replay/test_slot_a_save_tag_branch.c -o /tmp/slot-a-save-tag
/tmp/slot-a-save-tag
```
