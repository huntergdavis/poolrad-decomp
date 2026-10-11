# Resident LOAD PIECES display/list helper at `RES_00BA:0767`

Issue [#107](https://github.com/huntergdavis/poolrad-decomp/issues/107) recovers the
resident callback reached from the LOAD FILES / LOAD PIECES route. The C entry
`pr_rec_res_00ba_0767` is a **bounded local-control model**, not a strict full
translation. Its list traversal and argument selection follow the complete
decoded body, while calls into five display/text routines remain explicit
effect boundaries. Whether this routine was game-authored or linked from a
library is unresolved, so this work does not change either count.

| Ignored input | Bytes | SHA-256 |
| --- | ---: | --- |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |

## Extent and calling convention

The hash-locked `resident_loader_map.unpack` maps `0x00BA:0767` to loaded
image `0x01307` and packed START.EXE file `0x013E3`. Its 390-byte body is
loaded `0x01307..0x0148C`, source file `0x013E3..0x01568`, all contiguous
copy provenance with zero fill bytes. Body SHA-256 is
`d140492fbb4f613c4b25dfc96fdd850cfa758736ff2983a5202ef5d4f329effc`.
The next Pascal prologue starts at image `0x0148F`. Capstone 5.0.7 decodes
the 390 bytes as 159 instructions, including 23 direct branch instructions
(`jne` 4, `jb` 5, `ja` 5, `jmp` 9); every branch target stays in the body.
There is one return, `0x0148A RETF 4`.

The input is a DOS far pointer, offset at `[BP+6]` and segment at `[BP+8]`.
The route caller at GAME.OVR unit `0x01C:+0x0DE1` (file `0x01B27`)
pushes `[0x5D94]` then `[0x5D92]`, immediately after its call to
`RES_00BA:27D9` at `+0x0DD4`. It clears byte `[0x4432]` after this return.
The present function reads byte `DS:0x49F3` and the far list head
`DS:0x5D96/0x5D98`; it does not access the route's three flag bytes itself.
The only returned register value used by this model is AL from its terminal
call to `0x03F1:0137`; the body does not assign AL afterward.

The relocation-backed overlay inventory has **28 far callers in ten units**.
Each entry below is `unit:+offset (GAME.OVR file offset)`:

| Unit | Far call sites |
| --- | --- |
| `0x01C` | `+0x03A1 (0x010E7)`, `+0x0DE1 (0x01B27)`, `+0x2BFA (0x03940)`, `+0x32E5 (0x0402B)` |
| `0x01E` | `+0x0C36 (0x051AD)`, `+0x0D21 (0x05298)`, `+0x0F20 (0x05497)` |
| `0x020` | `+0x0DE3 (0x06361)`, `+0x117C (0x066FA)`, `+0x1199 (0x06717)` |
| `0x023` | `+0x0557 (0x0722B)`, `+0x0786 (0x0745A)` |
| `0x025` | `+0x1708 (0x08BDB)`, `+0x220A (0x096DD)` |
| `0x038` | `+0x0102 (0x0ED5D)`, `+0x063E (0x0F299)`, `+0x11BE (0x0FE19)` |
| `0x068` | `+0x0B96 (0x17261)`, `+0x0D05 (0x173D0)` |
| `0x06A` | `+0x1461 (0x18C92)`, `+0x17FA (0x1902B)`, `+0x181E (0x1904F)`, `+0x1A03 (0x19234)`, `+0x1CBE (0x194EF)`, `+0x1F3E (0x1976F)` |
| `0x06C` | `+0x01BF (0x19B3E)`, `+0x032C (0x19CAB)` |
| `0x07D` | `+0x08FF (0x246CB)` |

The candidate-seeded `resident_loader_map.decoded_near_calls` walk also
decodes **seven** direct resident near calls into loaded `0x01307`, at loaded
images `0x02F4E`, `0x0340B`, `0x0342D`, `0x0344F`, `0x0348B`, `0x03722`,
and `0x03841`. Thus 35 static direct sites have been identified. This is
static reachability; it does not prove every site executes on the active
LOAD FILES route.

## Decoded behavior and boundaries

| Loaded span | Local behavior | C boundary |
| --- | --- | --- |
| `0x0130D..0x0135F` | Choose x byte `1` when `[0x49F3]` is zero, otherwise `0x11`; initialize row byte to `2`. Copy each of two private Pascal literals with `0x0AF8:0B0B` and display through `0x0709:0626` at `(x,2,0x0F)` and `(0x21,2,0x0F)`. Then add two to row. | Two ordered title effects, without publishing the literals. |
| `0x01363..0x01378` | Read the far list head after both titles; a zero offset **and** zero segment skips the loop. | `read_head` callback. |
| `0x0137B..0x013CB` | Frame call `0x03F1:0137` with `(x,row,0x26,row)`. Compare both words of the node with the input far pointer. Equal calls active text `0x0709:0626`; unequal calls local `0x023B6`. | Ordered frame and one text effect. |
| `0x013CB..0x01415` | Read node byte `+0x111`. Values `0..0x32` and `0x3D..0x45` select class 1, `0x33..0x3C` class 2, and the rest class 0. Call local `0x0148F` with `(node,0x20+class,row)`. | `read_byte`, then field 111 effect. |
| `0x01418..0x01457` | Read node byte `+0x11B`. Values `0..9` select class 2, `0x0A..0x63` class 1, and the rest class 0. Call local `0x01513` with `(node,0x24+class,row,0)`. | `read_byte`, then field 11B effect. |
| `0x0145A..0x01470` | Increment row as a byte, then read the next far pointer from node `+0x104/+0x106` after the effect calls; repeat. | `read_next` callback. |
| `0x01473..0x0148C` | On null next pointer, call `0x03F1:0137` once more with the current row and return with `RETF 4`. | Final frame effect supplies returned AL. |

The two `jb` comparisons against zero are unreachable for an unsigned byte;
the table uses the resulting byte ranges. The local call at `0x013C8` targets
`0x023B6`; `0x01415` targets `0x0148F`; and `0x01457` targets `0x01513`.
The seven far call sites in this body target `0x0AF8:0B0B` twice,
`0x0709:0626` three times, and `0x03F1:0137` twice. Those called
routines consume the low bytes of their word position/style arguments;
the C events carry those low bytes and the full node identity separately.
The local callee bodies and display/runtime side effects are outside this
translation. In particular, the meanings of the two node fields, position
constants, title literals, frame result, and possible callback mutations are
not inferred as game concepts.
The decoded body writes stack locals but has no direct global write; any
other visible state change lies within a called routine.

Host callbacks are invoked at the original read/effect order, so a prior
effect can change a later list value in synthetic memory. `max_nodes` is a
host cycle bound, not a DOS branch. A host callback failure returns
`PR_REC_0767_BOUNDARY` with output AL unchanged and prior effects retained;
the limit similarly returns `PR_REC_0767_NODE_LIMIT`. Only a complete
terminal frame sets output AL. The model does not claim aliasing or register
side effects inside the external callees.
Only the `io` and AL pointers are checked before the first title call. A
missing callback reports `PR_REC_0767_BOUNDARY` when that call site is
reached. Thus a missing head reader still leaves both title effects, and an
empty head can complete without either node-field or next-pointer reader.

## Verification and prior work

Synthetic tests cover empty and two-node lists, both x choices, full far
identity comparison, each classification boundary, read-after-effect
mutation, row advance, a cycle bound, every callback failure position, and
missing callbacks at reached and unreached sites, plus invalid host
arguments. The focused test passes with
`-Wall -Wextra -Wpedantic -Werror`; full CTest and public hygiene results
are recorded in the PR.

Before implementation, `deja "poolrad RES_00BA_0767 LOAD FILES PIECES resident
function"` surfaced prior route research. Reused leads from the merged
[function ledger](FUNCTION_LEDGER.md), [new-game route note](ECL_NEW_GAME_ROUTE.md),
[resident loader map](RESIDENT_LOADER_MAP.md), and
[DAX open note](RESIDENT_DAX_FILE_OPEN.md). The binary hashes, packed span,
decode, frame, call sites, and control flow above were independently checked
against the ignored DOS files. No original executable bytes, private text,
media, or derived pixels enter Git.
