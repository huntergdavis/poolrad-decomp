# Unit 0x1C +0x0F73: null-head VERTICAL MENU route

This is a bounded follow-on to the merged
[prefix](U1C_VERTICAL_MENU_PREFIX_0F08.md) and
[one-node route](U1C_VERTICAL_MENU_ONE_NODE_0F73.md). The ignored DOS v1.3
`GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
The complete U1C handler body is file `[0x1C4E,0x1DCB)`, 381 bytes / 149
instructions, SHA-256
`937fbfd3ae61653216beaff6084175a35be1b22c3f0c4158d5379463c7d12288`.
This route starts at `+0F73` (file `0x1CB9`) and follows the branch at
`+0FB7` to the shared `+0FFB..+1084` menu tail and `RETF`. The containing
continuation `[0x1CB9,0x1DCB)` is 274 bytes / 110 instructions, SHA-256
`692863b3b57a534bd60b9920e560ab642b7fddf8cebcbf5b231259bdeea8ce6c`.

The `+0F73` call goes through overlay stub `3C5:003E` to U3B0 entry 6,
merged in PR #443. On this route its explicit callback leaves the far
output pointer at `SS:BP-0x106` zero. After the first draw, `OR AX,[BP-0x104]`
sets ZF, so `JE +0FFB` skips label copying and list traversal. The tail
restores the saved head, prepares 20 menu argument words, calls the menu,
stores its selection, releases the pointer, calls cleanup draw, and returns.
Its other external calls remain explicit callbacks with the original
cleanup sizes and call-entry register/stack state.

The C route composes the previously merged one-node model through its
zero-head branch stop, then executes a separate C model of the common tail.
The tail API starts strictly at `+0FFB`; it can be reused after later list
routes. The original-byte Unicorn checker compares the C route with the
original continuation from the same validated prefix entry state in 18
synthetic cases. It compares all 1 MiB RAM, all 14 CPU fields, and all six
external call-entry register/argument traces through `RETF`. Cases vary
source length, DS/SS aliasing, stack offset wrap, and selected menu value.
The focused strict C test checks the completed return and a fail-closed
menu-call abort under ASan/UBSan and `-Wall -Wextra -Wpedantic -Werror`.

The bounded pre-implementation `deja` query timed out with no result.
Reused the two merged U1C evidence notes above for the prefix contract,
call ABI, and node layout; each was checked against the original bytes.
No reached native `+0F73` menu frame was available in the bounded capture
search. Call effects in the checker are synthetic and identical on both
sides. This route is **provisional** and takes no full-handler, strict
full-C, new-identity, or count credit.
