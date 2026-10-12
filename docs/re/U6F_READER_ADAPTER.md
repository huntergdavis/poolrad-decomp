# Unit 0x6f entry 6 -> resident reader adapter

Host adapter (`overlay_u6f_reader_adapter.{h,c}`) between the merged `pr_u6f_entry6` hooks (`block_802_0716`, `free_af8_0254`) and the merged `pr_rec_res_0802_0716` reader (issue #74,
`RESIDENT_ECL_DAX_BLOCK.md`). It adds no game behaviour and no claim about the reader's internals.

| Reader outcome | Adapter result |
|---|---|
| `OK` | hook returns 0; size = the reader's size; pointer = a unique synthetic far key (reserved table slot `key_segment_base + i`, offset 0) whose view *borrows* the reader-owned block at exactly that size (no copy, no padding) |
| `MISSING_FILE`, `MISSING_ID` | return 0, size 0, nil pointer (the DOS routine writes zero outputs and returns normally; the model then takes `ERROR_EXIT` for block 1 and skips blocks 2/3) |
| `PACKED_LIMIT_ERROR` | `PR_U6F_RA_STOP_PACKED_LIMIT` (DOS prints a message and terminates; no normal return) |
| any other status | `PR_U6F_RA_STOP_HOST_BASE + status` (host-only; never the zero-size path) |
| `OK` with size 0 | `PR_U6F_RA_STOP_ZERO_OK` after releasing the returned block (not a DOS outcome, not guessed) |

Other stops: `NO_SLOT` (all reserved slots live, before the reader runs), `STRING` (a Pascal string with an embedded NUL or a NULL string), `FREE_MISMATCH`, `ARGUMENT`.
The synthetic far key range must not wrap 16 bits. Before loading, the hook checks that its slots are in the supplied object table and that no other entry has the same far key; an invalid layout returns `ARGUMENT` before the reader runs.
Ownership: each load takes one reserved slot; the u6f `FreeMem` hook releases it exactly once with the original key and size through `pr_rec_res_0802_0716_release`; afterwards the slot's view is
dropped so a stale key is `VIEW` in the u6f model. A wrong size, an unknown key or a second release returns `FREE_MISMATCH` and releases nothing. If the model stops before its `FreeMem`
(a failed roll, a unit read past the size, ...), `pr_u6f_ra_teardown` releases the still-live blocks as host cleanup and counts them separately (`teardown_releases`): it does not claim the DOS path freed them.
The 20-character name truncation stays inside the reader; the id is passed as its low byte.

Synthetic tests only (`tests/recovered/test_overlay_u6f_reader_adapter.c`, three fabricated archives through the real reader). The file-open helper, Pascal I/O and heap semantics remain unresolved
boundaries inside the reader. No strict full-C, count, identity, DOS-native or one-Up parity claim.
