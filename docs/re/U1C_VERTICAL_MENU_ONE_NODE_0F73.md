# Unit 0x1C +0x0F73: one-node VERTICAL MENU continuation

This is a bounded continuation of the merged
[U1C:+0F08 prefix](U1C_VERTICAL_MENU_PREFIX_0F08.md), not a complete
handler. The ignored DOS v1.3 `GAME.OVR` is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
This instruction range is file `[0x1CB9,0x1DCB)`, unit
`[+0x0F73,+0x1085)`: **274 bytes / 110 instructions**, SHA-256
`692863b3b57a534bd60b9920e560ab642b7fddf8cebcbf5b231259bdeea8ce6c`.
It begins at `LCALL 3C5:003E` and ends with `RETF` at `+0x1084`. The
entire original handler is 381 bytes / 149 instructions, file
`[0x1C4E,0x1DCB)`, SHA-256
`937fbfd3ae61653216beaff6084175a35be1b22c3f0c4158d5379463c7d12288`.

`3C5:003E` is an **overlay stub**, resolving to unit `0x3B0` entry 6
`+0x1140`; it is not resident code. Its output pointer, 0x2E-byte node
layout, link fields `+0x2A/+0x2C`, and `RETF 6` are modeled separately
in [PR #443](https://github.com/huntergdavis/poolrad-decomp/pull/443).
The merged [overlay inventory](OVERLAY_FUNCTION_INVENTORY.md) and local
`overlay_function_ledger.csv` gave the initial stub lead, verified against
the original bytes. A bounded pre-implementation `deja` query timed out
without a result. The merged prefix evidence provided the entry CPU and
stack contract. Its separate original-byte checker established parity for
that prefix.

The C model accepts the prefix's full 1 MiB RAM and 14-field CPU state at
`+0x0F73`. Every external call has a fail-closed callback. It covers one
non-null linked-list node whose next pointer is zero: allocate the head,
draw the frame, copy one label, write the node's `+0x29` byte, call the menu,
store its selected value, release the head, draw cleanup, and return through
the original `RETF`. It returns `BRANCH_STOP` for a null head or a nonzero
next link; those routes need separate models.

| Call site | Target | Callback return cleanup |
| --- | --- | ---: |
| `+0F73` | `3C5:003E` → U3B0 entry 6 node constructor | 6 |
| `+0FAA` | `709:0768` frame draw | 16 |
| `+0FD4` | `AF8:0B25` Pascal copy | 10 |
| `+1053` | `2B:0066` menu, U25 entry 14 | 40 |
| `+1060` | `2B:0052` selected-value store | 4 |
| `+106B` | `3C5:0043` → U3B0 entry 7 release | 4 |
| `+107C` | `709:0DD3` cleanup draw | 8 |

The original-byte Unicorn checker runs the merged C prefix to create
the exact `+0F73` entry state, then runs the original 274 bytes and this
C continuation from cloned states. Its 18 synthetic cases vary source
length, DS/SS aliasing, stack offset wrap, and menu selection. It compares
all 1 MiB RAM, all 14 CPU fields, and all seven call-entry register and
argument traces through `RETF`. External call effects are identical test
callbacks on both sides. The focused strict C test checks completed-route
return state, null-head stop, and second-node stop; it passes under
`-Wall -Wextra -Wpedantic -Werror` with ASan/UBSan.

No reached native `+0F73` frame was found in the bounded local capture
search. Callback effects for the menu, drawing, store, release, and copy
are synthetic; native menu selection and actual heap state remain open.
This continuation is **provisional** and takes no full-handler, strict
full-C, new-identity, or count credit.
