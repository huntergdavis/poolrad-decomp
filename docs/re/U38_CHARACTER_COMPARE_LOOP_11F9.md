# Unit 0x38 entry 39: eight active-character comparisons at +0x11F9

## Provenance and selection

This is the queued disjoint follow-on to U38 +0x1276, on a branch based on
`origin/main`. The exact prior `deja "poolrad U38 +11F9 active character eight
SYSTEM 0C10 comparisons full C"` query timed out after 20 seconds without
a session result. The SYSTEM helper implementation reuses the independently
decoded [resident comparator evidence](RESIDENT_STRING_0356.md), its
[reached unit-0x19 frame](NATIVE_COLD_START_U19_POST_RETURN_COMPARE.md), and
the separate U38 +0x1276 parity branch at commit
`ba5d612175b80063e361fd5f5c1c4cd9a167d0f5`.
This branch independently rehashes and executes the original helper bytes.

| Ignored local input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

The sole public index 39 enters U38 `[+0x11F9,+0x1276)`, `GAME.OVR` file
`[0xFE54,0xFED1)`: 125 bytes, SHA-256
`ab4666d6d7720151a70c54792f777202f01c9803880ae3911e019affb2aeb76c`.
Reachable code ends at `+0x1268`: 111 bytes, SHA-256
`9f175ddec9eef17cf10700899a26277932c64394b7caef4abcf666eb3ce10438`.
The remaining 14 bytes at `[+0x1268,+0x1276)` are Pascal constants used by
the next public entry; they are not instructions. They hash to
`eb19eb8681d04b84bfe569f341ba3b2de215ff19ee7c51282198074a48c57ae4`.
Their content stays in ignored owner media.

The sole loader fixup is `+0x1253`, the loaded segment word of the far call
at `+0x1250` to SYSTEM `:0C10`. The hash-locked START loader expands that
helper to image `[0x0BB90,0x0BBBB)`, 43 original bytes, SHA-256
`11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58`.

## Complete behavior

The wrapper follows a far link at `+0x108` from its caller-supplied far
pointer, reads the linked object's far pointer at `+0x0A`, and saves it at
DS `[0x6826]/[0x6828]`. It checks the active character through DS
`[0x5D92]/[0x5D94]`: byte `+0x9F` must equal one and low seven bits of
byte `+0x6C` must equal two. Either failed check returns immediately.

On the eligible path, it compares the saved string with eight 16-byte-spaced
Pascal strings at DS `0x3F2 + (index << 4)`, for index one through eight.
Each call uses the actual far frame and the original loaded SYSTEM comparator.
An equal comparison subtracts four from DS byte `[0x6822]`; the loop still
visits the remaining entries. It exits with `RETF 10`. The composed SYSTEM
body clears DF, compares Pascal lengths and bytes using `REPE CMPSB`,
restores DS, and exits with `RETF 8`. Its register results, call-frame writes,
and defined flags are modeled through the wrapper return.

The existing classifier calls +0x11F9 game-evidenced because it reads the
active-character pointer. The ledger lists public index 39 and no
fixup-backed direct caller. No reached game frame for this entry was found;
ownership and strict count credit remain provisional.

## Verification and gate

`tests/recovered/check_overlay_u38_character_compare_loop_11f9.py` checks
both input hashes, public entry, exact code/data extent, loader fixup, and
the expanded SYSTEM hash. Twelve cases execute both original byte bodies
in Unicorn, then compare all 1 MiB RAM and 14 CPU fields with C. They cover
both early exits, all eight unequal/equal/mixed calls, empty Pascal strings,
different loaded segments, DS/SS alias, and incoming DF set.

Four DOSBox-X normal-core synthetic far-entry frames execute the original
wrapper and SYSTEM bytes without a comparator stub. All 14 CPU words, 52
stack bytes, and DS `[0x6822]`/`[0x6826..0x6829]` match C. The COM,
original-byte copies, log, and capture stay under ignored
`resources/u38-11f9-parity/native/`; result SHA-256
`f7660a9b57371f3c7d7e65a5a91960619fccb9e7b1db8ee4c0bec3a6f37cbec1`.
The focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan test passes.

These native frames are synthetic, not reached game callers. This branch
makes no strict identity or count claim before PM review.
