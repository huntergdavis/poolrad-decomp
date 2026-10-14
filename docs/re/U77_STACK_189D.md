# Unit 0x77 +0x189D: provisional sidecar stack pass

`src/recovered/overlay_u77_stack_189d.c` models the complete in-body RAM,
CPU, stack, and far-return state of this nonpublic routine with its one
resident call supplied by a callback. It is **provisional** because no
native frame was reached and the resident call's live effects have not been
replayed jointly with this caller.

## Source and caller

The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
This candidate starts at file offset 141,527 (`+0x189D`, unit `0x77`). The
493-byte ledger bound `[141527,142020)` has SHA-256
`3b2597236166428adc977c31724e007e3942ac24a74e740a39d81ea174d10091`.
The reachable code is **474 bytes / 154 instructions** through `retf 4` at
`+0x1A74..+0x1A76`, SHA-256
`0a9c119606497ff144da91e7b4340f80842319c6ccc6ea719951cabb9a4d16d2`.
The remaining 19 bytes `+0x1A77..+0x1A89` are text data; they are not
included in the C source. All code branches have aligned in-body targets.

One aligned near call from public entry `+0x0EF8` reaches `+0x189D` at
`+0x1433`: the caller pushes a far pointer and `CS` on its `J` branch.
The adjacent `+0x1423` site calls the follow-on `+0x17ED` routine.
The sole outgoing call here, at `+0x1A13`, is resident `BA:1509` with
two far pointers: current sidecar node and active character. Its complete
callback-based translation and 63-byte node layout are documented in
[RESIDENT_SIDECAR_REMOVE.md](RESIDENT_SIDECAR_REMOVE.md); this model
reuses that verified contract, including `retf 8` cleanup, and independently
checks the pushed caller arguments. The active-character pointer
`DS:[5D92/5D94]` was reused from
[U7F_DIV5_ENTRY10.md](U7F_DIV5_ENTRY10.md) and rechecked in these bytes.
A bounded `deja` lookup returned no match, so no prior-session result was
reused.

## Behavior in the decoded body

The first far argument is a target node. Its byte at `+0x39` starts a
running count. The active character's sidecar head is at `+0xC8/+0xCA`;
each node's next pointer is at `+0x2A/+0x2C`. The routine walks the
chain, saving next before any removal call. It skips the target itself,
zero-count nodes, and nodes whose bytes at `+0x2E..+0x33`, `+0x36`, or
word at `+0x37` differ from the target. The original compares bytes
`+0x3C`, `+0x3D`, and `+0x3E` against the **current node itself**; those
comparisons are always equal. It separately requires current `+0x3C < 2`.

When running count plus current count is at most 255, it adds the current
count to a stack local and calls `BA:1509` to remove the current node.
When the sum exceeds 255, it writes 255 to the current target, writes the
remainder to the current node, and makes that node the new target for later
iterations. At chain end it writes the saved count to the final target's
`+0x39` and returns with `retf 4`. Far pointers use both words; only an
all-zero pointer ends the walk. The C model preserves scratch-stack writes,
source-order reads, CPU registers, and defined flags through the return.

## Verification and open gate

`test_overlay_u77_stack_189d.c` checks null-head and merge behavior and
host argument guards. `check_overlay_u77_stack_189d.py` verifies source
hashes, the 474-byte code/data boundary, all instruction starts, branch
targets, and call site. Twenty-six synthetic cases execute the original bytes
under Unicorn and the C model from identical 1 MiB RAM and CPU states.
They cover empty/head-target, count merge, overflow split, two merges,
zero count, every true field-mismatch exit, the self-comparisons, sums of
255 and 256, the `+0x3C` gate, and target offset wrap.
The audit compares the **entire 1 MiB RAM image**, all returned registers,
defined flags (AF is masked after final `OR`), and resident-call arguments.
`BA:1509` is stubbed without effects on both sides, so its separate
sidecar removal parity does not become a native caller parity claim.

A bounded search of route notes and 17 local RAM captures found no
`+0x189D` entry-byte hit or native frame. No strict full-C credit,
verified identity, or count claim is requested from this provisional PR.
