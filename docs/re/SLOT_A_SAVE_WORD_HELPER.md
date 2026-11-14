# Conditional SAVE operand-word helper

`pr_slot_a_save_word_helper` composes the `02A8` far call through a loaded
`:004D` far-jump stub to the existing raw U25 `06AF` word helper. It executes
on a staged copy of the full RAM and publishes the reached CPU/RAM and helper
status. The actual helper return CS:IP is exposed, including any altered frame;
callers must check it before continuing. Preconditions refuse atomically.

The caller independently authenticates the call, stub and 36-byte helper
references, supplies disjoint host objects, and ensures guest writes do not
modify code or bindings. Matching supplied references does not establish their
provenance. In the hash-pinned retained script image described in
[the first EXIT audit](SLOT_A_FIRST_EXIT_9974.md), the call targets
`084F:004D`, whose stub jumps to `308A:06AF`; this does not prove the current
invocation's binding. The helper contract and differential evidence are in
[the U25 loader note](U25_LOADER_0000.md); the preceding arguments are in
[the SAVE word-argument note](SLOT_A_SAVE_WORD_ARGS.md).

Synthetic tests compose those arguments with the raw helper, verify a `1234`
word and actual `02AD` return, and check atomic refusal for a short reference.
The subsequent SAVE store, same-invocation native binding and `345B` parity
remain unproved.

```
cc -std=c11 -Wall -Wextra -Werror -Iinclude src/replay/slot_a_save_word_helper.c src/replay/slot_a_save_word_args.c src/recovered/overlay_u25_loader_0000.c src/recovered/overlay_u25_string_cores.c tests/replay/test_slot_a_save_word_helper.c -o /tmp/slot-a-save-word-helper
/tmp/slot-a-save-word-helper
```
