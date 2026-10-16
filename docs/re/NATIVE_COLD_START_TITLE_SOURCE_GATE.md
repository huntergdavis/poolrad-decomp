# Cold-start title source and candidate framebuffer writer

This is an **offline source audit**, not a live title-transition watch. A
no-argument native run reached unit `0x17` entry 1 at `3909:0627` and
returned to resident image `+0x0122`; the separate continuous native video
contains the distinct title frame. The loaded caller body, its inline archive
name, and its resident bitmap display callee identify a bounded path from
`TITLE.DAX` block ID 2 to a direct EGA framebuffer store. No stop at the
block-2 call, display call, or store was recorded in that run, so the first
guest writer of the visible title remains unmeasured.

Before this audit, `timeout 20s deja "Poolrad native cold-start title frame
first writer transition from publisher splash U17 title route"` timed out
without an indexed result. The [native U17 entry/return](NATIVE_COLD_START_U17_PICTURE_ROUTE.md),
[continuous screen sample](NATIVE_COLD_START_SCREEN_GAPS.md),
[resident block-wrapper ABI](RESIDENT_464_BLOCK_WRAPPER.md), and
[title RGB asset comparison](COLD_START_TITLE_RGB.md) supplied the prior
boundaries reused here. All spans below were checked against the ignored
originals and the entry-stop RAM again. No original bytes, frame, or text
are included in this note.

## Pinned offline inputs

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| U17 entry-stop 640-KiB RAM | 655,360 | `11853e655ee5399644edc78c34ef3c3ae603ac685cec0cb8d3892f50d28e78a6` |

The entry-stop unit `0x17:+0627..+0808` body is 482 bytes, loaded SHA-256
`5205b089e2f8042ae894e958c60c4d515f9c6a9ab44a6e2b1bdef44e4406f4bf`,
matching the earlier native route note. Its original `GAME.OVR` source is
file `[0x006FB,0x008DD)`, SHA-256
`02adf3ddfc09a9cf558fe9735f53e78858bb4ec57471672d33be6082677f137d`.
The six-byte Pascal name immediately before entry at unit `+0621` maps to
file `[0x006F5,0x006FB)`. Original and loaded hashes both equal
`42fbba74977c4541a7f16191b22efe01ebe7d4da39c86754808510a8a18441c1`,
with zero changed bytes. Its five-character value matches the copied
`TITLE.DAX` stem case-insensitively.

## Two image calls in the entry body

The merged `RES_0464:+0111` ABI takes a far Pascal archive name, an ID WORD
whose low byte selects the block, two raw bytes, and a far destination slot.
The loaded U17 instructions push that same `CS:+0621` name and a stack-local
destination for both calls. The first passes ID 1 at `+0672`; after a draw
and delay, the `DS:49D1` nonzero arm passes ID 2 at `+06CA`. Both calls are
followed by `RES_0464:+1951` display calls with zero X/Y coordinates.

| U17 site | Source file | Static action | Original / loaded SHA-256 |
| --- | --- | --- | --- |
| `+0672` | `0x00746` | block-1 wrapper call | `1c195471da075e82f7faee008840fc076c4f152f248b8d42975b150269c996dc` / `8ea7e6175aeb493c30a4202e903079fe6227bc5a076e41a75930ad64cbea356f` |
| `+0683` | `0x00757` | first bitmap display | `6cc3bf5965381d41d3fb6117d07aba4d8e740e591ee4d4decca3cdf8e3690898` / `ac9be7058d9e3c62f4aa199563a8df0cfdaf35358460b3f3b89e17579be87346` |
| `+0690..+0696` | `0x00764` | compare `DS:49D1` with zero, jump to second arm if nonzero | `349191955f710cbf09de2d115647653dc4ac17004d72946ee1268689d6d02031` in both images |
| `+06CA` | `0x0079E` | block-2 wrapper call | `1c195471da075e82f7faee008840fc076c4f152f248b8d42975b150269c996dc` / `8ea7e6175aeb493c30a4202e903079fe6227bc5a076e41a75930ad64cbea356f` |
| `+06EE` | `0x007C2` | second bitmap display | `6cc3bf5965381d41d3fb6117d07aba4d8e740e591ee4d4decca3cdf8e3690898` / `ac9be7058d9e3c62f4aa199563a8df0cfdaf35358460b3f3b89e17579be87346` |

The two-byte differences at each far-call site are relocated segment
operands. The branch span is unchanged. At the three saved RAM stops (before
the U17 call, on U17 entry, and after return), **candidate** segment `14A0`
has `DS:49D1=1` and `DS:4A1E=1`. The entry-stop CPU `DS` register was not
retained, so those memory values do not prove the branch decision. The
separate asset comparison shows that `TITLE.DAX` block ID 2 produces the
stable native title RGB hash
`3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8`.

An additional private scratch copy of the merged C renderer changed only its
block selector from ID 2 to ID 1. It compiled under C11
`-Wall -Wextra -Wpedantic -Werror` against the merged format library. Its
768,000-byte RGB24 output hashes
`15e405dac036c9530d5f87ccb67c9427012e6860195ada76b56193ed03c557dc`,
exactly matching the decoded native publisher-splash sample at continuous
video slots 248–352: **0/768,000 differing bytes** and **0/256,000 differing
pixels**. The ID-2 result matches the later native title sample at slots
359–433 as documented in the separate renderer note. Thus the two archive
IDs account for the two consecutive stable screen images at the asset-to-RGB
level. The private scratch source, output, and native capture remain ignored;
these image comparisons do not by themselves prove that the guest executed
the conditional second call or identify its first video write.

## Static EGA writer boundary

`RES_0464:+1951..+1B51` is a 513-byte routine ending in `RETF 8`. Its
original `START.EXE` file span `[0x05FCA,0x061CB)` hashes
`d64bb18e4503a5679481894c5ddaad44b3c0f703b00d1810afbbba8e9f358de3`;
the loaded U17-entry RAM span at `0C88:1951` hashes
`b297f688470803b952d1304f5a0ba3c905d1ed63944e84a71a1810cbaf9d1d44`.
Four bytes differ in relocated operands. After its non-null object test,
`DS:4A1E=1` selects the EGA arm at `+19F9`. That arm sets the EGA plane
mask through ports `3C4/3C5`, sets `ES=A000`, and directly stores one byte
at `0C88:1AA2` via `ES:[DI]`. The three-byte store at original file
`0x0611B` is byte-identical in RAM, SHA-256
`7cc107bb3b15e14c0a2b41083f41a5748ab0fb6ac0bbd0122030d1da88bb4d92`.
This is a **candidate display writer site** for either bitmap call, not the
measured first writer of the title frame. Earlier clears, page selection,
other writers, and a display-page switch remain possible.

The next native stop, once the heavy-emulator slot is released, is U17
`3909:0690` with CPU `DS` and `DS:49D1`, followed by `3909:06CA`,
`3909:06EE`, and `0C88:1AA2`. Capture the far-call return frame at each,
the active/draw page state, and a frame or framebuffer hash before and after
the transition. This will distinguish the ID-1 and ID-2 invocations and
identify whether `+1AA2` is the first title-writing instruction. The current
audit launched no emulator, adds no C parity or game-function count, and
does not claim a full native startup route.
