# U33 +040F: complete four-cell grid pattern

pr_u33_grid_pattern_040f executes the complete 518-byte, 209-instruction
caller through its actual RETF2. It composes the real +01AD cell-pair and
+0221 clipped-writer implementations, including +012B and U3BD lookup
callees. The first nonzero pair result skips the second pair call. Four
selected glyphs are written through the live grid pointer with all CPU
and RAM effects preserved. There are no caller-supplied hooks.

## Selection and reuse

The refreshed queue against main 1bbaa582 is schema 6, the current successor
to the requested schema-4 queue. OVL_0033_040F is the highest-ranked
composable hard caller: rank 100, score 50, 209 instructions. Read-only
operations-ledger inspection found no +040F body event; all-ref history
found no implementation. ds4 had confirmed +040F was free. Claims were
sent to ds4 and Claude; Claude's active +1434/+1577 work does not overlap.

The prerequisite targeted deja query returned no matching indexed sessions.
Concrete reused repository evidence and code:

- Merged #877, head 58b9496527795c00617e051beb0c4958f435562a:
  [U33 +0615](U33_GRID_PATTERN_0615.md) supplies CPU/RAM helpers, the
  contiguous far-return treatment, and the generic original-execution oracle.
  The new audit imports that oracle and adapts only its host candidate entry.
  It executes this caller and all reached original callees afresh.
- Merged #846: [U33 +01AD](U33_CELL_PAIR_01AD.md),
  [+012B](U33_CELL_LOOKUP_012B.md), and their folded U3BD +0350/+047B calls.
- Existing [U33 +0221](U33_CLIPPED_WRITER_0221.md) and
  [U3BD +031F](U3BD_IN_GRID_031F.md), linked unchanged.

All dependency APIs and implementations remain unchanged. The audit checks
all 209 instruction-address annotations against the decoded original.

## Identity and call graph

| Item | Measured value |
| --- | --- |
| START.EXE | 65231 bytes; SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 bytes; SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| U33 base | GAME.OVR 0xC5D1 |
| Caller extent | [040F,0615), file [0xC9E0,0xCBE6), 518 bytes / 209 instructions |
| Caller SHA-256 | 880652652d8a0222747e0f99b090418fc29d674e547b9809e0d5855a0f4bcc04 |
| Incoming edge | U33+0888 calls +040F after PUSH CS |
| Pair calls | +042B and +0448 call +01AD after PUSH CS |
| Writer calls | +05D9, +05EA, +05FB, +060C call +0221 after PUSH CS |
| Return | +0612 RETF 2 |
| Lookup dispatch | +012B uses fixup-backed far calls at +0155/+0170 and loaded stubs 03D5:002F/002A |

The audit independently pins seven bodies by extent, hash and instruction
count. Original media and extracted listings remain under ignored resources.
The loaded far-jump stubs are synthetic dispatch fixtures, with relocated
original far-call segment words.

## ABI and behavior

SS:SP contains return IP/CS; SS:SP+4 contains an inherited frame offset.
That offset is repeatedly loaded before reading these live SS-relative bytes:

| Frame displacement | Role |
| --- | --- |
| -3, -4 | Lookup x and y |
| -7 | Glyph category |
| -8 | Secondary glyph condition |
| -2, -1 | Projection inputs consumed by +0221 |

The first pair uses (x, y-1, facing 6). Only a zero result reaches the
second pair, (x-1, y, facing 0). CBW sign extension and pushed AX high
bytes are preserved. A local flag is one only when both pair results are
zero. The four-pass loop reads the category and secondary byte live and
selects glyph locals in original instruction order. Unhandled categories
or secondary values can leave glyph locals at their previous RAM values;
there is no invented default or local zero initialization.

The four writers use (column delta,row delta) = (1,0), (2,0), (1,1), (2,1).
For signed inherited bytes a=frame[-2], b=frame[-1], +0221 computes the low
bytes of column=5*a+6*b+21+delta and row=5*a+10+delta. It writes glyph+1
through DS:6716 only when the signed column is 0..49 and row is 0..24.
Frame, grid, table and stack aliases remain live; a write may change a
later argument, saved BP, or return frame.

The API accepts exactly 1 MiB synchronous RAM, distinct stable CPU/RAM,
IP=040F and SP in [0080,FFFA]. It conservatively reserves 128 bytes below
entry SP and atomically rejects overlap with modeled code in the caller's
segment. Original loaded code and stubs must remain immutable; data writes
into code are outside the contract. Effective offsets wrap at 16 bits;
word, LES and far-return bytes are physically contiguous with A20 wrapping.

Argument and reserved-code-alias failures are atomic. CALLEE preserves the
reached dependency failure state; TRANSFER preserves a changed nested
return. Completion exposes the actual outer RETF2. Grid aliases may move
that return frame across FFFE/FFFF by changing saved BP; explicit cases
verify all four contiguous return bytes and the separately wrapping SP.
The four-pass loop needs no artificial iteration limit. Undefined logical
AF follows the dependencies' zero profile. Interrupts, concurrent mutation,
and universal hardware undefined-FLAGS behavior are outside the profile.

## Fresh validation

The original-execution audit passes 1,046 cases comparing all 14 CPU words
and every byte of 1 MiB RAM without masks: 1,042 actual outer returns and
four changed nested-return transfers. RAM has the same backing at 0 and
1 MiB for A20 wrap. Every reached callee runs original instructions; no
scalar-result stubs replace calls. The nested-return monitor executes a
changed RETF once before comparing its resulting state.

| Body | Executed instructions / pinned instructions |
| --- | --- |
| U33 +040F | 209 / 209 |
| U33 +01AD | 54 / 54 |
| U33 +012B | 49 / 49 |
| U33 +0221 | 64 / 64 |
| U3BD +031F | 16 / 18 |
| U3BD +0350 | 110 / 120 |
| U3BD +047B | 93 / 103 |
| Total | 595 / 617 |

Caller branch coverage is 71/72 edges. The sole missing edge,
+0565 -> +05C2, is structurally unreachable: the preceding selector arms
handled loop values 1, 2 and 3, leaving 4. The audit requires exactly that
missing edge. The 22 unvisited dependency instructions are not claimed as
newly covered; their existing evidence remains separately attributed.

Fixtures include 225 lookup-result/category/secondary combinations, 81
projection cases, 192 coordinate/map cases, stack endpoints, wrapped
parent frames, 144 parent/stack aliases, 80 grid/stack aliases, two outer
far-return seam cases, A20 data, relocated code/stubs, incoming flags and
300 deterministic random cases. The four changed nested returns arise
from real writes into stack frames.

Strict C11 warnings and ASan/UBSan pass for the focused regression. It
checks real lookup/writer composition, the nonzero-first-pair path, exact
four-cell values, stale glyph locals, clipping, both far-return seams,
reached invalid-stub failure and atomic guards. The complete recovered
library builds and all five focused CTests pass.

| Review field | Assessment |
| --- | --- |
| target_function | OVL_0033_040F, complete [040F,0615) |
| source_candidates | pr_u33_grid_pattern_040f and unchanged full-state dependency APIs |
| signals | Pinned bodies/descriptor, aligned call ABI, 209 source sites, original execution, whole CPU/RAM comparison, branch accounting, changed returns |
| score | 1,046 whole-state cases; caller 209/209 instructions and 71/71 feasible edges; chain 595/617 instructions |
| verdict | Author evidence submitted for independent acceptance |
| review_notes | Synthetic parity does not establish saved-game reachability; arbitrary alias space is not exhaustive; explicit stack/code/FLAGS profile applies |

No acceptance event, strict manifest, credit count or denominator is changed.
The independent reviewer owns verification and credit.

## Reproduction

~~~sh
python3 tools/overlay/u33_grid_pattern_040f_audit.py \
  --exe /path/to/ignored/resources/START.EXE \
  --ovr /path/to/ignored/resources/GAME.OVR
cmake -S . -B /tmp/poolrad-build
cmake --build /tmp/poolrad-build --target poolrad_overlay_u33_grid_pattern_040f_tests
ctest --test-dir /tmp/poolrad-build --output-on-failure -R grid_pattern_040f
~~~

The audit requires Capstone and Unicorn. Original media remain owner-provided
under ignored resources.
