# U33 +0615: complete four-cell grid pattern

pr_u33_grid_pattern_0615 executes the complete 507-byte, 196-instruction
caller through its actual RETF2. Two real +01AD calls obtain cell-pair
results, the caller selects four glyph bytes, and four real +0221 calls
write the resulting clipped 2-by-2 grid pattern. All lookup and writer
CPU/RAM effects remain live. There are no caller-supplied hooks.

## Selection and reuse

The refreshed queue against main d1398522 is schema 5 (successor to the
requested schema-4 queue). OVL_0033_0615 ranks 76 with 196 instructions,
two composable dependencies and one aligned incoming near call. U25+1DF0
ranks higher but depends on RES_00BA_1509's external-effect callback API,
which does not provide a CPU-state callee. ds4 confirmed that distinction
and that +0615 had no reservation. The operations ledger had no +0615
body event; all-ref history showed no existing implementation. Claude's
active +1434/+1577 work was checked for overlap and the +0615 claim was
sent to that lane before implementation.

Targeted deja queries indexed sessions but timed out without reusable
session text. A focused RETF-seam query also returned no reusable finding;
the seam correction is established by the new original-execution case. Concrete reused repository evidence and code:

- Merged #846: [U33 +01AD](U33_CELL_PAIR_01AD.md), its
  [+012B lookup](U33_CELL_LOOKUP_012B.md), and folded U3BD +0350/+047B.
- Existing [U33 +0221 clipped writer](U33_CLIPPED_WRITER_0221.md) and
  [U3BD +031F range leaf](U3BD_IN_GRID_031F.md), linked unchanged.
- The +012B audit's pinned lookup bodies, loaded-stub layout, and map
  fixtures informed the new audit. The +01A0 audit supplies generic CPU14,
  byte-address helpers and original-input hashes. All original instructions
  in this new caller's execution chain are executed afresh.

The original caller is translated in instruction order with 196 address
annotations. The audit independently checks every annotation against the
decoded original. Dependency APIs and implementations remain unchanged.

## Identity and call graph

| Item | Measured value |
| --- | --- |
| START.EXE | 65231 bytes; SHA-256 d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 bytes; SHA-256 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| U33 base | GAME.OVR 0xC5D1 |
| Caller extent | [0615,0810), file [0xCBE6,0xCDE1), 507 bytes / 196 instructions |
| Caller SHA-256 | c973402de9f97dbf004ece46254273f351e9a0a8600bc1f442dbbd9b1b4371e9 |
| Incoming edge | U33+088D calls +0615 after PUSH CS |
| Pair calls | +0631 and +064D call +01AD after PUSH CS |
| Writer calls | +07D4, +07E5, +07F6, +0807 call +0221 after PUSH CS |
| Return | +080D RETF 2 |
| Lookup dispatch | +012B uses fixup-backed far calls at +0155/+0170 and loaded stubs 03D5:002F/002A |

The audit pins seven original bodies independently:

| Unit / entry | Extent | Instructions |
| --- | --- | --- |
| U33 +0615 | [0615,0810) | 196 |
| U33 +01AD | [01AD,0221) | 54 |
| U33 +012B | [012B,01AD) | 49 |
| U33 +0221 | [0221,02BA) | 64 |
| U3BD +031F | [031F,0350) | 18 |
| U3BD +0350 | [0350,047B) | 120 |
| U3BD +047B | [047B,057A) | 103 |

Every dependency hash is checked and printed by the audit. The two loaded
far-jump stubs are synthetic dispatch fixtures, with relocated original
call segment words. No original media or extracted listing is checked in.

## ABI and behavior

SS:SP contains return IP/CS; SS:SP+4 contains an inherited frame offset.
The caller repeatedly loads that offset before reading SS-relative bytes:

| Frame displacement | Role in this caller |
| --- | --- |
| -3, -4 | Lookup x and y |
| -7 | Glyph category |
| -9 | Secondary glyph condition |
| -2, -1 | Projection inputs consumed by +0221 |

The pair calls use (x, y-1, facing 2) and (x+1, y, facing 0), preserving
CBW sign extension and all pushed AX high bytes. Their results are 0, 1,
or 3. A local flag records whether both are zero. The four-pass selector
loop reads the category and secondary condition live and selects the four
glyphs in the original order. Categories outside the explicitly handled
0, 1, and 3 leave two glyph locals at their prior RAM values; the C does
not invent defaults or zero-initialize them.

The writer calls use (column delta,row delta) = (5,0), (6,0), (5,1), (6,1).
For signed inherited bytes a=frame[-2], b=frame[-1], +0221 computes the
low bytes of column=5*a+6*b+21+delta and row=5*a+10+delta. It writes
glyph+1 through the live DS:6716 far grid pointer only when the signed
resulting column is 0..49 and row is 0..24. All argument, inherited-frame,
grid-pointer and table reads remain live. A grid write can change a later
argument, inherited frame, saved BP, or return frame.

The API accepts exactly 1 MiB synchronous RAM, distinct stable CPU/RAM
objects, IP=0615 and SP in [0080,FFFA]. The stack interval conservatively
reserves 128 bytes below entry and keeps all nested far frames away from
16-bit stack wrap. Overlap of that reserved interval with modeled code in
the caller's segment is rejected atomically. All modeled code and loaded
stubs must remain original and immutable; writes to other code segments
are outside the contract. Effective offsets wrap at 16 bits, while word
and LES bytes are physically contiguous with A20 wrapping.

Argument and reserved-code-alias failures are atomic. CALLEE preserves
the dependency model's reached failure state; TRANSFER preserves the actual
changed nested return. Normal completion exposes the actual outer RETF2.
A grid alias can change saved BP and place the final outer far frame at
offset FFFE or FFFF. RETF reads all four frame bytes physically contiguously;
only the resulting SP wraps. Both seams are covered by explicit original
regressions, including different segment bytes at the contiguous and wrapped
addresses. The four-pass loop needs no host iteration limit. Undefined logical AF
follows the linked models' zero profile; no interrupt, concurrent mutation,
or universal hardware undefined-FLAGS behavior is claimed.

## Fresh validation

The audit compares all 14 CPU words and every byte of 1 MiB RAM, without
masks, for 956 cases: 952 actual returns and four changed-return transfers.
The same RAM backing is mapped at 0 and 1 MiB for A20 wrap. Every callee
executes as original instructions; no scalar-result stubs are substituted.
A nested return monitor executes a changed RETF once and compares its
actual resulting state.

Coverage is 196/196 caller instructions and 60/62 caller branch edges.
The two excluded edges are structurally unreachable:

- +06EB -> +0707: results 0 and 3 were already handled, leaving result 1.
- +0763 -> +07BD: loop values 1, 2 and 3 were already handled, leaving 4.

The audit requires exactly these two missing edges. Complete-chain execution
reaches 582/604 instructions: +012B 49/49, +01AD 54/54, +0221 64/64,
+031F 16/18, +0350 110/120, +047B 93/103, plus the caller's 196/196.
The 22 unvisited dependency instructions are not claimed as newly covered;
the existing dependency evidence remains separately attributed.

Fixtures cover all 135 real lookup-result/category/secondary combinations,
81 projection cases, 192 coordinate/map cases, stack endpoints, wrapped
parent frames, 144 parent/stack aliases, 80 grid/stack aliases, A20 data,
relocated code/stubs, incoming flags and 300 deterministic random cases.
The four changed nested-return cases arise from real grid writes to stack
frames. Two additional cases complete through outer RETF frames crossing
FFFE/FFFF after saved-BP aliases. They exposed and corrected a separately
wrapped word-pop implementation during author review.

Strict C11 warnings and ASan/UBSan pass for the new regression, which checks
real lookup/writer composition, exact four-cell values, stale glyph locals,
clipping, reached invalid-stub failure and atomic guards. The complete
recovered library builds and all five focused CTests pass.

| Review field | Assessment |
| --- | --- |
| target_function | OVL_0033_0615, complete [0615,0810) |
| source_candidates | pr_u33_grid_pattern_0615 and unchanged full-state dependency APIs |
| signals | Pinned body/descriptor, aligned call ABI, 196 source sites, actual original execution, full CPU/RAM comparison, branch-edge accounting, changed returns |
| score | 956 whole-state cases; caller 196/196 instructions and 60/60 feasible edges; chain 582/604 instructions |
| verdict | Author evidence submitted for independent acceptance |
| review_notes | Synthetic parity is not saved-game reachability; arbitrary alias space is not exhaustive; explicit stack/code/FLAGS profile applies |

No acceptance event, strict manifest, credit count or denominator is changed.
The independent reviewer owns verification and credit.

## Reproduction

~~~sh
python3 tools/overlay/u33_grid_pattern_0615_audit.py \
  --exe /path/to/ignored/resources/START.EXE \
  --ovr /path/to/ignored/resources/GAME.OVR
cmake -S . -B /tmp/poolrad-build
cmake --build /tmp/poolrad-build --target poolrad_overlay_u33_grid_pattern_0615_tests
ctest --test-dir /tmp/poolrad-build --output-on-failure -R grid_pattern_0615
~~~

The audit requires Capstone and Unicorn. Original media remain owner-provided
under ignored resources.
