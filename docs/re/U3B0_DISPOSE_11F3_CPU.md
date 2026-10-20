# U3B0 entry 7 +11F3: complete raw list disposal

The caller now composes the complete raw SYSTEM FreeMem entry, core,
merge helper and rounding helper. It reads the first far pointer through
its argument cell, saves each next link before freeing the current node,
and returns after the saved link becomes nil. There are no host node maps
or allocator callbacks. The argument cell is not explicitly cleared;
ordinary heap/stack aliases can still change its bytes.

## Identity and selection

| Item | Evidence |
| --- | --- |
| Caller | U3B0 entry 7, stub 03C5:0043; complete [11F3,1245) |
| Extent | 82 bytes, 29 instruction starts, RETF4 at1242; no trailing data |
| GAME.OVR file interval | [187074,187156) |
| Caller SHA-256 | ab914628d39b710f43ef46e48c393403e2d4e078df6fcf45207f4cf8f4b21dd8 |
| GAME.OVR SHA-256 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| START.EXE SHA-256 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| Dependency | CALL1238 to SYSTEM0AF8:0254, then near03D3/0432/0491 |
| Dependency extent | 233 bytes /106 instructions, pinned individually by the audit |

The current schema-8 queue succeeds the requested schema-4 queue. On the
selection base this row is rank302, score37, with 13 fixup-backed callers
and one of one runtime callees ready. Its inline_in_model tag is a false
positive: the sole source hit is the neighboring +1140 test asserting that
its last instruction ends at11F3. That assertion is boundary evidence,
not an implementation of this caller. The following +1245 prologue is the
end boundary. No higher-ranked substantive unimplemented caller was found;
other ready source mentions were small prologues or already composed bodies.
The U1C+271F queue extent also includes data (14 actual code bytes).

The classifier still says **helper_candidate / provisional**. Composable raw
behavior alone does not establish game ownership or independent acceptance; the
argument below is made under the two-factor standard. This change edits no
classifier, callable ledger, manifest, checkpoint or credit record.

## Identity under the two-factor standard (an argument, not a credit)

The standard in [REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#identity-standard-two-factor-adopted-2026-10-20)
needs a relocated call site inside game code **and** a subject of documented game
state. Values below are from the CSVs and the media, re-derived here.

1. **Reached from game code, relocated.** The queue lists 13 fixup-backed far
   callers of the entry stub `03C5:0043`. Each lies inside a function that
   `overlay_classification.csv` calls `game_evidenced`, except `OVL_001C_0F08`
   (`game_verified`): `OVL_0086_008A` (+0279), `OVL_006F_069E` (+0782),
   `OVL_006C_056C` (+091E, +0A33, +0D75, +0EDE), `OVL_001E_09D7` (+0C13),
   `OVL_007F_0CEB` (+0F2E), `OVL_001C_0F08` (+106B), `OVL_006A_0FBC` (+13C3),
   `OVL_0077_2156` (+2459), `OVL_0077_24A7` (+2749) and `OVL_006C_306D` (+3288).
   **None of those functions is strict-credited**, so the basis is the
   classification, not a credited caller.
2. **Operates on documented game state.** The body frees a chain of 0x2E-byte
   blocks linked at node+2Ah/+2Ch. That is exactly the node of the list at
   `DS:4766/4768` that [U86_STRING_NODE_028D.md](U86_STRING_NODE_028D.md) documents:
   46-byte nodes rounded to 48 by GetMem, link at +2Ah/+2Ch. `OVL_0086_028D` is in
   both `strict_full_c_addresses.csv` and `reviewed_function_identities.csv`. The
   sibling [U86_LIST_BUILD_059C_CPU.md](U86_LIST_BUILD_059C_CPU.md) (also credited)
   resets `DS:4766/4768` and "discards the old list pointer without freeing nodes",
   so this routine is the disposal those two leave out. The caller
   `OVL_0086_008A` at +0279 passes that global cell directly:
   `mov di,4766h; push ds; push di; lcall 03C5:0043`.

**Limits of the argument.** Only the `+0279` caller is shown to pass the
`DS:4766` cell; the other twelve pass stack locals (`push ss; push di`), and this
document does not claim that their lists are the same list. The node size and link
offset are the same layout, which is what the body operates on. The stub
`03C5:0043` is the overlay-manager entry stub; it is not modelled, and the
model enters the body at +11F3 directly.

## Raw ABI and behavior

At entry SS:SP contains return IP/CS, then the offset/segment of the
argument cell. The caller allocates eight local bytes. It reads the cell's
two words in source order, tests both for nil, copies the current pointer
to locals BP-8/-6, and snapshots node+2A/+2C into BP-4/-2. FreeMem receives
size **0x2E**, rounded to48 bytes by the original heap manager. This is
separate from the U1C CLEARMONSTERS handler's size0x3F contract.

The original heap manager writes size/link words, walks the live sorted
free list, merges adjacent blocks and contracts HeapPtr. It can stop with
RunError204 after partial writes. The caller preserves the full CPU and
RAM at that boundary; it does not clear the head, roll back frees, or
normalize malformed/cyclic lists. Heap errors also leave the next pointer
already saved in the caller's local frame.

The API accepts exact1MiB A20-off RAM, entry IP11F3, initial SP0034..FFF8,
and separate CPU/diagnostic storage. Each effective offset wraps at16 bits;
words, LES pairs and return frames read physically contiguous bytes with
20-bit physical wrapping. All reached return frames must be nonwrapping,
including alias-induced changes. Loaded code is immutable; the far-call
opcode/offset is checked before CALL, and its live segment binds SYSTEM.
Other caller/callee bytes are identity preconditions. This flat-RAM profile
excludes MMIO, faults, IRQs and self-modifying code. Logical AF follows the
measured DOSBox-X normal/386 zero convention.

- RETURNED executes the final RETF4, including a live modified caller frame.
- CODE stops before CALL1238 when its opcode/offset is wrong, after earlier
  caller pushes and local writes.
- CALLEE preserves a nonzero FreeMem status and its exact CPU/RAM.
- TRANSFER preserves an unexpected successful FreeMem far return.
- LIMIT stops at1209 after an exhausted taken JMP123D budget. Zero allows
  the first free, then stops before testing the next link, even when nil.
- ARGUMENT rejects invalid API inputs without changing CPU/RAM. FreeMem's
  own walk budget is separate and resets for each call.

## Measured evidence

Final counts are recorded below and pinned by the private receipt.

- Original-byte oracle: **196 cases**, all14 CPU words and all1,048,576 RAM
  bytes unmasked. Caller29/29 starts and2/2 conditional outcomes; closure
  135/135 starts and35/36 outcomes. The missing outcome is the callee's
  zero-size shortcut03D5->0431: this caller always pushes0x2E. It remains
  in the reported denominator.
- DOSBox-X normal/386: **96 cases**, 88 returns and eight RunError204 stops;
  CPU14 plus1,024 stack,65,536 DS and65,536 heap bytes, all unmasked.
  RESULT.BIN SHA-256:
  69417130ea2c4c7a97f850ab42a28215b9515f958ab68012a3f8c408882e6deb.
- **799 ASan/UBSan semantic checks** and focused CTest1/1.

Original cases include empty and multi-node lists, both pointer-word nil
tests, ascending/reverse adjacency, merges on both sides, HeapPtr
contraction, offset8 allocations, invalid/range/duplicate pointers,
cycles, caller/heap limits, opcode/offset refusal, both changed near and
far returns, head/node/local aliases, SS=DS, boundary/odd SP, wrapped node
links and head words, A20 wrapping, relocated SYSTEM, randomized flags,
random lists and randomized background RAM. Semantic checks assert
retained argument cells, sorted coalescing/top contraction for lists of
1..64 nodes, exact partial stop frames and unchanged argument rejection.

The native comparison covers the 14 CPU words, the 1,024-byte stack window, the
65,536-byte DS segment and the 65,536-byte heap arena: **132,096 RAM bytes**. The
other **916,480 bytes** of the 1 MiB space are excluded from it, because real DOS
memory outside the captured windows is not reproducible by a flat-fill model; the
original-byte oracle above compares all 1,048,576 bytes with no mask.

Native fixtures are synthetic and cover return/RunError paths in initialized
memory. They do not establish natural gameplay reach, native alias/limit/
transfer parity, or a general memory-mapped hardware contract. An initial
native random list used overlapping allocations and followed a corrupted
link outside the initialized arena, yielding a DX mismatch. The fixture
was corrected to use disjoint allocations; overlapping lists remain in
the original full-memory audit. The C implementation and comparisons were
unchanged. The failed native log/capture remain private. Final native input
is reproduced byte-for-byte by the final harness, without repeating an
unchanged native run when original-only cases were added.

### Declared substitution: the DOSBox-X binary

The native oracle is **not** the pinned DOSBox-X source build (git
`5a9c7c53481c2e933948039c33256776653f4faf`). It is the packaged binary on PATH,
`/usr/bin/dosbox-x`, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, reporting
`DOSBox-X version 2026.01.02 SDL2`, run with `core=normal`, `cputype=386`,
`cycles=max`. The receipt's `dosbox_x` block records path, hash and version. The
pinned build was neither rebuilt nor re-run here. What could differ from it: the
lazy-flag conventions (AF after OR/XOR/AND/TEST/MUL/DIV and non-zero shifts, OF after
SHL, ZF/PF after MUL), the LES wraparound above FFFC and stack-offset wrap at FFFF.
This body's only flag-setting instructions are `OR` and `SUB`; it also uses `LES`,
moves, pushes, `POP`, jumps and the far call, and has no `ADD`, `INC`, shift, `MUL` or
`DIV`; its profile keeps entry SP at or below FFF8 with nonwrapping
return frames. All 14 CPU words, FLAGS included, matched in every replayed case, so no
divergence was observed; that does not prove agreement with the pinned build on states
the replay does not reach, and the composed FreeMem closure carries its own conventions.
A reviewer who needs the pinned result must re-run the native harness with that binary.

## Assertions the checker makes

Beyond comparing every case, `u3b0_dispose_11f3_cpu_audit.py` asserts that a set of
named fixtures reaches its expected stop (`empty`, `one`, `empty-bad-call` return;
`bad-call-opcode`, `bad-call-target` CODE; `list-limit`, `list-limit-two` LIMIT;
`bad-offset` CALLEE:2), that the caller is covered in full, and that the closure's
only missing outcome is the named zero-size shortcut `0AF8:03D5->0431` with no missing
start. The receipt stores every named fixture's measured stop (`named_stops`).

## Checker registration

`CHECKER_ROWS=('OVL_03B0_11F3',)` is declared in the audit, which implements
`--compile-only` (it builds the model with the real SYSTEM FreeMem source and binds
the entry point before reading any media, and prints `CHECKER_BUILD_OK`).
**Gate run:** on main `b0f19a30` with this branch, `check_checker_builds.py OVL_03B0_11F3`
reports `PASS: 1 actual checker builds for 1 rows`, `check_body_doc_pairing.py --base b0f19a30`
reports every body and checker paired with evidence and registration, the focused CTest target
`poolrad_u3b0_dispose_11f3_cpu_tests` builds from CMake and passes 1/1, and the ASan/UBSan C
suite (`-Werror`) passes its 799 checks. `--compile-only` needs no media.

The private receipt (`--unicorn --native --output`, generated at code commit `59d7f28f`, which the
branch head carries unchanged apart from merges of main) has SHA-256 `b7f1c26663ac8d573fd09ce943fb4862c0694dd97db694483f70b182ea59b0ab`. It pins the
media and the FreeMem span hashes, the Unicorn report with every named fixture's stop, and the native
report including the `dosbox_x` binary block. A re-run of the final harness reproduced the previous
RESULT.BIN hash `69417130…` bit-for-bit.

## Readiness counterfactual (measured on the pinned queue)

Ten candidate rows list this body as a callee: `OVL_007F_0CEB`, `OVL_006F_069E`,
`OVL_006C_056C`, `OVL_0077_24A7`, `OVL_006C_306D`, `OVL_006A_0FBC`, `OVL_0077_2156`,
`OVL_001E_09D7`, `OVL_001C_0F08` and `OVL_0086_008A`. It is already `modeled`, which
the queue counts as ready, in all ten, and none is gated on it: a strict credit changes
no readiness and unlocks no caller (zero delta). Per the queue rule for completeness
declarations, no manifest row is proposed. The value of crediting it is the credit and
the identity, not an unlock.

## Proposed rows (text only; nothing here edits a CSV)

    strict_full_c_addresses.csv:
    OVL_03B0_11F3,docs/re/U3B0_DISPOSE_11F3_CPU.md

    reviewed_function_identities.csv (review_pr and reviewed_commit are the acceptance PR's to fill):
    OVL_03B0_11F3,docs/re/U3B0_DISPOSE_11F3_CPU.md,<review_pr>,<reviewed_commit>,ab914628d39b710f43ef46e48c393403e2d4e078df6fcf45207f4cf8f4b21dd8,82

## Reproduction and reuse

With the pinned private media under resources/DOS/POOLRAD:

    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u3b0_dispose_11f3_cpu_audit.py --unicorn --native --output resources/dispose-evidence.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_u3b0_dispose_11f3_cpu_tests
    ctest --test-dir resources/build -R '^poolrad_u3b0_dispose_11f3_cpu_tests$' --output-on-failure

The pre-implementation shared-memory lookup for U3B0+11F3/FreeMem returned
prior [SYSTEM FreeMem #1011](https://github.com/huntergdavis/poolrad-decomp/pull/1011)
evidence. Its [raw API and pinned oracle](SYSTEM_FREEMEM_0254_CPU.md) supply
the unchanged allocator, heap fixtures and disk-fed native capture idioms.
The recalled older +11F3 disposal document was absent from current main;
the VP independently cleared ownership. The neighboring
[+1140 boundary report](U3B0_ENTRY6_FULLSTATE_1140.md) supplies only the
prologue boundary. No inferred older implementation is reused.

## Author assessment

| Field | Assessment |
| --- | --- |
| target_function | U3B0 entry7 complete [11F3,1245), composing SYSTEM0254/03D3/0432/0491 |
| source_candidates | overlay_u3b0_dispose_11f3_cpu.c with unchanged system_freemem_0254_cpu.c |
| signals | Pinned bytes, neighboring boundaries, CALL/RETF ABI, ordered pointer/heap effects, CPU14/full-RAM differential, native replay, semantic checks |
| score | Five evidence categories: identity, structure, differential, native, contract tests; no probabilistic score |
| verdict | likely_match within the declared profile, pending independent verification |
| review_notes | Provisional helper identity; false queue inline tag; immutable code, nonwrapping reached frames, flat RAM, explicit partial stops; native synthetic scope and fixture correction above; no self-credit |
