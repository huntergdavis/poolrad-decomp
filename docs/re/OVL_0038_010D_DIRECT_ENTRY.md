# OVL_0038_010D: direct entry to the existing dispatcher adapter

The t73 hybrid adapter needs an entry at `0038:010D`. The new
`pr_u38_entry2_010d_cpu` entry in `overlay_u9e_dispatch_0000_cpu.c` calls the
existing dispatcher slot-1 `h_010d` adapter. It does not change that adapter's
semantic model or the dispatcher slot table. This adds no recovery or
replacement credit.

The direct entry requires exactly 1 MiB of A20-off RAM, IP `010D`, and entry
SP in `[0200,FFEE]`. Null pointers, another RAM size, another IP, or a stack
outside that interval are refused without changing state. Original
instruction bytes must remain unchanged by stack/data writes. The original
body adds five to DS byte `6825`, increments DS byte `6822`, preserves the
ADD carry through INC, restores the saved BP, and returns with ten argument
bytes removed. The entry reuses the existing adapter for those effects.

`tools/hybrid/check_r3_010d.py` checks the original 16-byte, six-instruction
body against this direct entry. The preliminary run measured **545 cases**,
comparing **14 CPU words and all 1,048,576 RAM bytes with zero masks**.
It covers all byte values for both operands, arithmetic edges, stack/data
aliases, odd stacks, and A20 wrapping. **Seven argument refusals** preserve
state. **Six negative controls** are detected: wrong argument cleanup, ADD
carry, INC zero flag, state addition, saved-BP write, and entry-IP gate.
The exact final-head rerun is required in the PR receipt; the JSON records
its tested head and source hashes.

Original body SHA-256:
`870dfc9d737ed044d6ff9eae323534b19716dc36b040f6e126296cabcf5cb5ac`.
The pinned inputs are START.EXE
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`
and GAME.OVR
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Original bytes and generated fixtures remain under ignored `resources/`.

This direct Unicorn/C comparison is not a native replacement receipt.
[The R3 batch-14 protocol](../HYBRID_GLUE_BATCH14.md) separately requires a
fresh original pair, V1 and routed runs using one recorded emulator binary,
matching route actions and final pixels, and explicit per-target outcomes.
It does not establish off-route coverage, arbitrary layouts outside this
entry contract, or Bakshi's Sleep.
