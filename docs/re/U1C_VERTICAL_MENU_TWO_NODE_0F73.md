# Unit 0x1C +0x0F73: two-node VERTICAL MENU route

This is a separate bounded follow-on to the merged
[prefix](U1C_VERTICAL_MENU_PREFIX_0F08.md) and
[one-node route](U1C_VERTICAL_MENU_ONE_NODE_0F73.md), using the
[null-head follow-on](U1C_VERTICAL_MENU_NULL_HEAD_0F73.md)'s common
`+0FFB` tail. The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes,
SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The original U1C handler body is file `[0x1C4E,0x1DCB)`, 381 bytes /
149 instructions, SHA-256
`937fbfd3ae61653216beaff6084175a35be1b22c3f0c4158d5379463c7d12288`.
The containing continuation `[0x1CB9,0x1DCB)` is 274 bytes / 110
instructions, SHA-256
`692863b3b57a534bd60b9920e560ab642b7fddf8cebcbf5b231259bdeea8ce6c`.

The `3C5:003E` callback returns a head at `0x4000:0x2200` linked to a
second node at `0x4000:0x2300`; the latter's next pointer is zero. The
route traverses two nodes, invoking `AF8:0B25` at `+0FD4` twice, clearing
each node's `+0x29` byte, then passes the saved head to the menu. It follows
the shared menu/store/release/cleanup tail through `RETF` at `+1084`.
All external effects remain explicit callbacks. A third nonzero link
stops at `+0FB7`; that route remains outside this model.

The C model composes the merged one-node route through its first nonzero
next-link stop, models the second traversal, then invokes the separate
`+0FFB` tail. The original-byte Unicorn checker enters from the validated
prefix state, and in 18 synthetic cases compares complete 1 MiB RAM,
all 14 CPU fields, and eight call-entry register/argument traces against
the original continuation through `RETF`. Cases vary source length,
DS/SS aliasing, stack offset wrap, and menu selection. The focused strict
C test checks both copy destinations, final return, and abort at the second
copy. It passes ASan/UBSan with `-Wall -Wextra -Wpedantic -Werror`.

The bounded pre-implementation `deja` query timed out without a result.
Reused the linked U1C evidence notes for the entry contract, ABI, and
common tail; checked each against original bytes. No reached native
`+0F73` menu frame was found in the bounded local capture search. Actual
menu/release and heap effects remain unmeasured; checker callbacks are
synthetic and identical on both sides. This route is **provisional** and
takes no full-handler, strict full-C, new-identity, or count credit.
