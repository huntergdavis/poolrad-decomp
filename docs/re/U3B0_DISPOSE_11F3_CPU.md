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

The identity remains **helper_candidate / provisional**. Composable raw
behavior does not establish game ownership or independent acceptance.
This PR changes no classifier, callable ledger, or credit record.

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
