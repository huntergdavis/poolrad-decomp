# Unit 0x6F party insertion at +0x1446

`pr_u6f_party_insert_1446` translates the executable
`+0x1446..+0x157E` code path over explicit DOS far-pointer segment views.
Its caller at `+0x19E3` supplies a newly allocated character-record
pointer, after the record loader returns. A
[merged native first-slot selector measurement](https://github.com/huntergdavis/poolrad-decomp/pull/255)
(main `8e7f8ee`) establishes `AX=1` at `+0x19AD` and entry to `+0x19B1`
for one saved iteration. [Merged native caller evidence](https://github.com/huntergdavis/poolrad-decomp/pull/256)
(main `080e7de`) observes the GetMem return at `+0x19BA` and the
`+0x0AEC` record-loader return at `+0x19DA`. The merged public evidence
cited here ends at that return; it does not establish record contents,
completion of the insertion call at `+0x19E3`, or later `+0x02AE` effects.
This is a bounded, path-complete model for supplied mapped state, with
no strict full-C count. The host `max_nodes` guard can stop a finite DOS
list early, and head/active-character/window globals are split from the segment
views. This PR does not establish native saved-route insertion parity.

Before decoding, `deja "Poolrad unit 0x6F 0x1446 party insertion Move
membership u9B 01AF"` found no exact indexed session; `deja "u6f
1446"` found prior PM party-build context. Reused the
[first-slot continuation map](U6F_PARTY_CONTINUATIONS.md),
[party input model](SLOT_A_PARTY_INPUT.md), and merged
[u9B record updater](U9B_RECORD_INCR_01AF.md). The source bounds,
helper calls, membership set, and state writes below were independently
checked against the ignored DOS images.

## Exact executable boundary

`GAME.OVR`: 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
Unit `0x6F` file base is `0x1DFE6`. The code from `+0x1446..+0x157E`
maps to file `0x1F42C..0x1F564`: **313 bytes, 99 contiguous x86-16
instructions**, SHA-256
`370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245`.
Its last instruction is `RETF 4` at `+0x157C`; all ten decoded direct
branches land within this code span. The following
`+0x157F..+0x1601` is **131 bytes of inline data**, file
`0x1F565..0x1F5E7`, SHA-256
`1dfa5f64639ccd4fec61babcf5419475c3c8317f0d3ba3b69f5ab0731dd192ee`.
It is excluded from the instruction and translation counts. The
preceding 32-byte set at `+0x1426..+0x1445`, file
`0x1F40C..0x1F42B`, SHA-256
`d22f8e3450655784853879c7ecfee4ff36192352f6ffe0d40593dee56f583350`,
has exactly numeric members 0 through 7. Its original bytes are not
copied into C. The observed caller's `+0x19B1..+0x19E9` file span is
57 bytes, SHA-256
`76347194a33838c597b3fba609cb594dcd17a78bf4d74883330276032344a9c5`;
`+0x19E3` near-calls this routine with one far pointer and resumes at
`+0x19E6` after `RETF 4`.

`START.EXE`: 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.
The hash-locked loader expansion places SYSTEM `0xAF8:0x16A8` at loaded
`0xC628..0xC63B`, 20 bytes, SHA-256
`f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7`.
It performs `rep stosb` then `RETF 8`, clearing the eight-byte local at
`+0x14CA`. SYSTEM `0xAF8:0x0DC0` is loaded
`0xBD40..0xBD60`, 33 bytes, SHA-256
`753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c`.
It tests bit `index % 8` of set byte `index / 8` and returns with ZF
clear on membership; the `JE +0x1509` at `+0x14F7` skips marking a
nonmember. The third far call, `0x00AC:0x0025` at `+0x1574`, is the
merged unit `0x9B:+0x01AF` record updater. No other calls occur in the
99-instruction code span.

## State and branch order

| Code offset | Source effect | C representation |
| --- | --- | --- |
| `+0x144F` | Set incoming record byte `+0xBF` to `0xFF` **before** inspecting the head. | First reached write in the supplied segment view. |
| `+0x1455..+0x1478` | If both head words at DS:`0x5D96/0x5D98` are zero, set head and active character DS:`0x5D92/0x5D94` to incoming. | Ordered `head` then `active_character` far words. |
| `+0x147A..+0x14B9` | Otherwise follow each record's `+0x104/+0x106` next pointer until both words are zero, then write incoming offset followed by segment to that last link. | Mutable segment views; host traversal cap stops at the next loop head, retaining earlier writes. |
| `+0x14BE..+0x151C` | Clear eight stack bytes; walk from head, marking `used[record[+0xBF]]` only for indices in the 0..7 set. | Local eight-byte occupancy array. |
| `+0x151E..+0x1563` | Set active character to incoming, reset its `+0xBF` to zero, and increment until the first unused index or eight. | Writes the actual byte on each increment; all eight occupied yields index 8. |
| `+0x1565..+0x1569` | Increment the word at far `[DS:0x49D6]+0x67C` modulo 65536. | Reached mapped window-1 word write; a missing view stops here with list/index effects intact. |
| `+0x156E..+0x157C` | Call unit `0x9B:+0x01AF` with incoming, then return. | Invoke merged `pr_u9b_record_incr`; retain its partial byte increments if a reached view ends. |

DS:`0x5D92/0x5D94` is the active-character pointer, as also mapped by
the merged [monster-load pointer audit](U1C_LOAD_MONSTER_0450_MAP.md)
and [gameplay path](GAMEPLAY_PATH.md). The last list node is found by
following links from DS:`0x5D96/0x5D98`; the code does not read the
active-character pointer to locate it.

The far-pointer and word accesses use 16-bit wrapped offsets. Segment
views supply mutable host memory; the model performs no file I/O or
allocation. An absent reached view or a malformed cyclic list stops at
an explicit source boundary. The traversal limit is a host guard, not
a claimed DOS result. The older `pr_u6f_party_build` is a higher-level
party-input model; this function exposes the insertion routine's
separate source-order writes and failure boundaries.

## Reproduction

From the repository root:

```sh
bash tests/recovered/check_overlay_u6f_party_insert_1446.sh
bash tests/recovered/check_overlay_u6f_party_insert_1446.sh \
  resources/DOS/POOLRAD/GAME.OVR resources/DOS/POOLRAD/START.EXE
```

The focused C11 build uses `-Wall -Wextra -Wpedantic -Werror` and tests
empty and linked lists, occupied indices 0..8, both traversal-limit
points, reached missing segment/window views, word wrap, and completed
or partial u9B updates. The second command also checks six private
source/helper spans and the numeric set members, reporting no source
bytes. Neither command writes original data into the public tree.
