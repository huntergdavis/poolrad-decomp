# U68 entry 1 supplied-state prefix projection

`pr_u68_e1_prefix` projects a boundary from explicitly supplied state. It does not invoke `pr_u68_entry1`, any callback, or any effect. For selector 1 and a window word below FF, it requires a full 1024-byte field buffer and uses `pr_u3bd_entry3`. Result 1 stops before 073B, result 0 before 8AE, and results 2/3 before the message path. Other selectors and the window-clear path stop before those paths. Missing or short field input refuses the supported lookup. The input is not modified.

This is a synthetic model-flow projection, not CPU/RAM composition, a native branch observation, same-run parity, or a complete Up frame. The hash-locked native 348D and 3492 RAM images and callback effects remain separate evidence blockers. Evidence and branch boundaries: `U68_ENTRY1.md`, `U3BD_FIELD_ENTRY3.md`, and `SLOT_A_POST_EXIT_NATIVE_RETURN.md`. The t4492 design packet SHA-256 is `bc560e98d2f8ccb4a31a61c3c3511223eeb926613b4254300036d2b93f7d9e8d`.
