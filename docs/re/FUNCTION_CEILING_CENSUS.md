# DOS v1.3 static function ceiling: 815

This report records the PM-accepted **static upper-bound candidate
denominator** of **815** distinct callable original function bodies in
non-Borland-library code. The census is static and read-only. It adds no
translation or identity credit and states no completion percentage.

| Input | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Baseline queue (898 originals) | `2c91fd5cae252fd5d06f954bf81290bac14b074ebdb9299264890801b8d55322` |

The baseline queue was regenerated with
`tools/overlay/function_queue_audit.py` at main `5811ca4` using Capstone
5.0.7. Its hash equals the
[accounting checkpoint](function_accounting_checkpoint.json). Every one of the
898 queue originals is present in this census.

## Arithmetic

| Part | Count | Derivation |
| --- | ---: | --- |
| Overlay | 675 | 673 queue starts + 2 in a previously missed overlay unit |
| Resident, non-library | 140 | 135 queue starts + 5 in the segment-`0x8AE` timer driver |
| **Ceiling** | **815** | 818 audited non-SYSTEM candidates − 10 library rows + 2 + 5 |

The library tier is excluded at segment level:

| Segment | Library | Evidence | Starts |
| --- | --- | --- | ---: |
| `0xAF8` | SYSTEM runtime | Prior classification; first init call; installs INT 00h/23h/24h/3Fh | 91 (80 queue + 11) |
| `0xA40` | CRT | Second init call; installs INT 1Bh | 17 (9 queue + 8) |
| `0xAA2` | Overlay manager | Installs the INT 3Fh handler | 21 (1 queue + 20) |
| `0xA32` | DOS unit | INT 21h date/time wrappers and register-pack shape | 2 |

Adding these 131 seeded library starts gives an all-code count of **946**
seeded starts. A linear sweep also splits the unexplained library bytes into
114 terminator-delimited chunks. In a scenario where each such chunk holds
exactly one more body, the all-code count would be 1,060. That chunk split
does not prove at most one body per chunk, so 1,060 is a scenario figure,
not an unconditional upper bound. Neither figure is the game denominator.

The **game-ownership interval** is **[143, 815]**. The lower end is the
reviewed verified game identities in the checkpoint. Every other start in
the 815 has unknown ownership.

### What the 815 includes and how it is deduplicated

- **Overlay unit inits.** 35 overlay entry-0 unit initializations and 11
  resident init-call targets. Of these, 44 are empty
  `push bp; mov bp,sp; pop bp; retf` bodies. The two non-empty inits are
  `OVL_0016_00AC` and `RES_042B_00A4`.
- **Distinct addresses stay distinct.** Three byte-identical body families
  are not merged:
  - 45 five-byte members, body SHA-256 prefix `fc1e97b8fe4158fd`;
  - 4 seven-byte members in unit `0x38`, `3d5d31028d8927cd`;
  - 2 members of 108 bytes, `RES_0709_0C3F` and `RES_08AE_0000`,
    `a8e33767ba204b74`.

  Collapsing each family to one member would give 767 unique bodies. That
  is a porting-effort view only.
- **No double counting.**
  - The five `+0x800` public-entry alias slots add 0.
  - The six jump-table targets in `RES_08AE_160B` are interior labels.
  - The loader prefix at image `0x0006` is part of the main block (below).

## Corrections to the queue

1. **Missed overlay unit at GAME.OVR [8, 212).**
   - **What it is.** Resident segment `0x16` (image `0x160`) is an INT 3Fh
     stub descriptor: overlay offset 8, 190 code bytes, 14 fixup bytes, two
     entries at `+0x00AC` and `+0x0042`. Unit code SHA-256 is
     `9f97cdd0064229e47e7686386e21db93b17d6cb017c3736d7b12c29462657ec5`.
   - **Why it was missed.** The descriptor lies in the part of the image the
     startup loader expands. `overlay_inventory.find_units` scans the packed
     file, never saw it, and labelled the range file header containing two
     data prologues.
   - **`OVL_0016_00AC`** is entry 0, the unit init, far-called through
     the resident stub slot `0x16:0x0020` from the init sequence. It stores
     the far pointer `0x16:0x0025` at DS:`0x4414`. That address is not the
     handler body. It is the resident INT 3Fh stub slot for entry 1, whose
     entry word the overlay manager resolves to the overlay body at
     `+0x0042`.
   - **`OVL_0016_0042`** is entry 1, a heap-error handler returning with
     `retf 2`. DS:`0x4414` is the pointer through which SYSTEM makes its
     indirect calls at images `0xB337`/`0xB343`, so those now resolve to
     game code.
2. **Segment `0x8AE` timer driver.** `RES_08AE_137C` installs vector 08h
   through INT 21h AH=25h with DX=`0x13EA`. The handler `RES_08AE_13EA` and
   its near helpers `+0x1477`, `+0x14EC`, `+0x157A` and `+0x160B` are new
   starts.
3. **Library rows.** Ten queue non-SYSTEM rows sit in library segments: nine
   in CRT and one in the overlay manager.

## Method: ingress and gaps

- **Regions.** Each GAME.OVR unit's code, and each resident code segment of
  the unpacked image. Segment bounds come from relocated segment values plus
  overlay stub extents; DS is `0xC7C`.
- **Seeds.**
  - standard BP-frame prologues (`push bp; mov bp,sp`);
  - public entries, with `+0x800` aliases normalized;
  - fixup- and relocation-backed far-call targets;
  - `mov ax,off / mov dx,seg` procedure-pointer constants;
  - INT 21h AH=25h setvect handlers;
  - the loader transfer.
- **Descent and closure.** Recursive descent stops at other seeds, and the
  near-call closure is iterated to a fixed point.
- **Every segment reference accounted for.**
  - 4,228 overlay fixups: 1,792 far calls to SYSTEM, 1,064 to resident code,
    1,162 to stubs, and 210 pointer constants, all of them to stubs.
  - 561 loader relocations: 336 far calls to SYSTEM, 122 to resident code,
    89 to stubs, 5 DS loads, and 9 self-segment loads in `0x8AE`.
  - Initialized DS has no relocations.
- **Ingress.** Every one of the 815 starts has observed ingress: an entry or
  alias slot, a far or near call, a pointer constant, a setvect, the init
  sequence, or the loader. The one explicit exception is `RES_0000_00F6`, the
  main block, reached only by fallthrough. The loader's final transfer
  enters image `0x0006`. The 48 contiguous init far calls then fall through
  into the prologue at `0x00F6`. `0x0006` is kept as a prefix label and is
  not counted.
- **Gaps.** Every uncovered game-tier byte is resolved:

| Game-tier uncovered bytes | Bytes | Resolution |
| --- | ---: | --- |
| Overlay length-prefixed string runs | 8,498 | Data |
| Overlay strings, 32-byte set constants, zero pad | 2,287 | Data, including four string runs checked by hand |
| Overlay pad | 11 | Data |
| Overlay residual code bytes | 44 | Dead interior code. Unit `0x19 +0x10A` is an instruction skipped by an unconditional jump. Units `0x6C +0x2CDA` and `0x3B0 +0x107A` are dead jumps after unconditional jumps. Unit `0x19 +0x330` is a 37-byte block with no ingress that falls into its parent's epilogue |
| Resident strings and pad | 657 | Data |
| Resident `0x899` image `[0x8A9C, 0x8AAE)` | 18 | Known unentered interior block of `RES_0899_0104` |
| Resident `0x8AE` image `[0x8CB7, 0x9E5C)` | 4,517 | Driver data (below) |

### Segment `0x8AE` range: no functions

**Part A: no ingress.** Executing a byte in the range requires CS=`0x8AE`
with IP inside it. Each possible path was enumerated:

- **Far transfers.**
  - The segment constant `0x8AE` occurs at 18 relocations and 28 overlay
    fixups.
  - 37 of those are far calls, to `+0x00DC`, `+0x017B`, `+0x01A5` and
    `+0x01D2`, all outside the range.
  - The other 9 are register loads for DS, ES, BP or DX.
  - There are no pointer constants to the segment.
- **Near transfers.**
  - All 1,675 bytes of the segment outside the range decode; none inside it
    is reached.
  - No direct jump or call targets the range.
  - The only two indirect transfers go elsewhere: the six-entry opcode
    table, interior to `+0x160B`, and the far jump through the saved
    previous INT 08h vector, which is stored only from INT 21h AH=35h.
- **Code pointers built from CS.**
  - All 21 `push cs` instructions are `push cs; call near` far-call idioms
    to counted starts.
  - The only CS-relative stores save that vector.
  - There is no `push imm`.
- **Interrupt vectors.** The only setvect handler is `+0x13EA`.

**Part B: every byte typed.** A local byte map assigns all 4,517 bytes
exactly once, with no overlaps, to structures the decoded driver code
addresses:

| Structure | Bytes |
| --- | ---: |
| Header variables | 21 |
| Bit-7-masked message printed by the handler before exit, with terminator | 34 |
| Opcode jump table | 12 |
| Two arrays of 8 × `0x30` channel records | 768 |
| Two 12-word note-divisor tables and one variable | 50 |
| Two 21 × 8 song tables | 336 |
| Wave tables (pointer and length set by stream commands) | 800 |
| Music command streams, parsed with the interpreter's own grammar from 61 table pointers and 2 call targets | 2,258 |
| Envelope tables (pointers set by stream commands) | 192 |
| Saved INT 08h vector | 4 |
| Unreferenced monotone table at `+0x20E` | 19 |
| Stream tail after a return command at `+0xF77` | 23 |
| **Total** | **4,517** |

## Residual caveats

- **Layout-typed bytes in `0x8AE`.** The 19-byte table and the 23-byte
  stream tail are typed from their layout, not from a direct reference.
  Part A independently makes the range's function contribution 0.
- **Near-only code pointers.** A near-only procedure pointer built without a
  relocation would appear as an undecoded code gap in game segments. None
  remains.
- **Library segments.** Library bytes outside the seeded closure are left
  as unseeded chunks by design.
- **Ownership.** Segment-level library exclusion and the game-ownership
  interval are judgments for the PM. The census measures callable bodies,
  not authorship.

## Local evidence (not published)

The census scripts and their raw outputs stay outside the repository because
they contain original-byte snippets. Their SHA-256 values pin the reviewed
state:

| Local artifact | SHA-256 |
| --- | --- |
| Full report | `af69a0208c5b30ac5a6e58a7ae7fc3c0e0fa63549e13d7f662397c320f3c7816` |
| Starts CSV (947 rows: 815 game, 131 library, 1 prefix label) | `9efb9c49daec38ecc78d0431e5575db8813a0a44ee08e296efb79efeb7beea65` |
| Gaps CSV with resolutions | `d7b4639ee887f245b973992ac1ec38236ada5d575ad569da7e7f388005826bd4` |
| `0x8AE` no-ingress proof output | `161acbf82f689f969a1c1b71ef8031f4cb0e03d9d8fc4dd8579defb184fed59b` |
| `0x8AE` byte map output | `9ee321dbdd4b49d15b809ffaeae21b3d63db484254158c97c67c53dca3065508` |
| `0x8AE` stream parse output | `0b5760c717006a01c5df30de45862a98e6eb35843da5c9d974c20d9d7bf0bf08` |
| Census pass 1 / pass 2 scripts | `371452bdeb3d5aa814645ac677cc500fff5798c74770f857edb724f179c3606c` / `c3998301cd9b910de11f5606925f3b7af962560fb6a994dbc45cc40bfb6e1694` |
| Proof / stream / byte-map scripts | `d1a2428ad1e0283408231bab3885bf503df52248d49c94cd6c0bc8409821f77c` / `29ef64bb57ecac004f6432b6a2c2cc96c29e9def00bb89ee3e87bdba906a2a5d` / `fb233555af47bf3676dc2d114730422bc8cb9ee80194ad782f8be997bf7e5b51` |
| Shared loader shim | `32b67558a11080b26133ae998338916b4b55e749afe8367e90ffdc2043921cb2` |

Two runs of the census produced byte-identical CSVs.

Prior-session recall found nothing to reuse: the bounded `deja` query timed
out, and the MCP recall matched only this session. This report builds on the
checked-in [denominator range](FUNCTION_DENOMINATOR_RANGE.md),
[queue audit](FUNCTION_CENSUS_QUEUE.md),
[alias audit](FUNCTION_ALIAS_AUDIT.md),
[resident callable coverage](RESIDENT_CALLABLE_COVERAGE.md) and
[accounting refresh](FUNCTION_ACCOUNTING_REFRESH.md).
