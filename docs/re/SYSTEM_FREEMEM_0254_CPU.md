# SYSTEM FreeMem 0AF8:0254: raw CPU and free-list state

The complete FreeMem entry composes its near core, two calls to the merge
helper, and size rounding. It inserts a freed block in the sorted list,
coalesces adjacent blocks in either direction, and contracts HeapPtr when
the free range meets the top. The source preserves all CPU words and
ordered RAM effects, including aliases and failures; there are no callbacks.

## Identity

START.EXE SHA-256:
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Addresses below are offsets in SYSTEM at unpacked load segment 0AF8.
Ranges are half-open and each digest is asserted by the audit.

| Span | Bytes | Instructions | SHA-256 |
| --- | ---: | ---: | --- |
| Entry [0254,026E) | 26 | 11 | 47b372b606692fcf3038cd6734cf3f3c600d9dff7298dce2a50d2d0a03196c96 |
| Core [03D3,0432) | 95 | 42 | 828388f0c2e899b6aaaf00c9c294397c879075f524eff052e73d26cde7e79311 |
| Merge [0432,0491) | 95 | 45 | b839d3ecc1ecd5e593a51dbb7c22231ad96156638c4676c7a3753bccff5bb7c9 |
| Rounding [0491,04A2) | 17 | 8 | 440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d |

Total exercised chain: **233 bytes / 106 instructions**. The 17-byte /
eight-instruction rounding helper is shared with accepted GetMem; this
report does not count it as a new game function. Entry/core/merge account
for 216 bytes / 98 instructions. The audit checks all four near-call
sites/targets and the RETF6 / RunError204 entry ABI.

## Behavior and ABI

After a far call, SS:SP contains return IP, return CS, size, pointer offset,
pointer segment. The entry loads AX=size and BX:CX=pointer, calls the core,
restores BP, and either RETF6 or sets AX=204 and jumps to CS:00E2. At that
RunError boundary, the original caller's far-return frame is still on the
stack. The RunError body is outside this translation.

A zero size returns without validating the pointer. For nonzero sizes,
the original requires pointer offset 0 or 8 and pointer in the lexicographic
range [HeapOrg,HeapPtr), using DS words 4400/4402 and 4404/4406. It does
not verify allocation ownership or that the supplied size fits before
HeapPtr. C retains this behavior, including oversized or inconsistent
frees; it does not impose stronger allocator invariants.

Size rounding uses the 17-bit sum size+7: paragraph count in DX and the
8-byte remainder in AX. The rounded size is written to the freed block
at +4/+6 **before** searching the free list at DS:440C/440E. Thus a duplicate
pointer fails only after changing that node's size fields. Bad offsets and
out-of-range pointers fail before those writes.

Each list edge is read live. Once the insertion point is found, +0432
first links/merges the new node with its successor, then links/merges its
predecessor with the result. Adjacent HeapPtr contracts without copying
a node. Other adjacency copies the successor's link and adds its size.
MOVSW/LODSW/STOSW execute in original order after CLD; only this merge-copy
path clears DF. Each intermediate stack push/pop and operand read remains
live, so heap/global/stack overlaps may alter subsequent reads, saved
segments, return addresses and results.

## Profile and status boundaries

The API takes exactly 1 MiB A20-off RAM and a CPU at IP0254, with conservative
initial SP[0020,FFF6]. CPU storage must be distinct from RAM. Loaded code
is immutable; every reached return frame must remain nonwrapping, including
after alias-induced changes. Offset arithmetic wraps at 16 bits, physical
addresses at 20 bits; words, LES pairs and return frames are read physically
contiguously. Logical AF follows the measured normal/386 zero convention.
IRQs, faults and self-modifying code are outside scope.

- RETURNED: RETF6 executed; all returned CPU/RAM state retained.
- ARGUMENT: null pointer, wrong RAM size/IP or initial SP outside profile;
  nothing changed.
- RUNERROR: invalid or duplicate pointer, AX204 at current CS:00E2.
- TRANSFER: a near RET popped a different return offset; exact partial
  state is retained.
- LIMIT: more than max_steps taken walk edges would return to +040F;
  execution stops there before the next iteration. Zero permits insertion
  at the first examined node. Cyclic/malformed lists are not normalized.

No stop rolls back mutations. Flags reflect the original last writer;
intermediate arithmetic flags are folded only when an unconditional later
instruction overwrites them before any observable boundary.

## Measured evidence

- **353 original-chain cases** compare CPU14 and all 1,048,576 RAM bytes
  unmasked: 344 returns, six RunError204, two list limits, one changed
  near return.
- **106/106 instruction starts and 34/34 conditional outcomes** across
  entry, core, merge and rounding.
- **128 DOSBox-X normal/386 cases**: 122 returns, six RunError204.
  All CPU14 words, SS1,024 + DS65,536 + heap65,536 bytes match unmasked.
- **2,584 ASan/UBSan semantic checks**; focused CTest **1/1**.
- Native RESULT.BIN SHA-256:
  fa40a3e0269cdcdee668ae06c0974e0433e58c452f26a38b0f923bfc270551cc.
- Combined private resources/freemem-0254/evidence.json SHA-256:
  6de39596f6b6304053372fed60705c9f1f99bec52da019e3ec659415387b4426.

Original fixtures exercise zero/invalid/range-boundary/duplicate pointers,
insertion before/after equal-segment nodes, both merge directions,
coalescing with HeapPtr, segment-equal offset-mismatch branches, maximum
size rounding, cyclic/budgeted lists, randomized free lists/sizes/flags,
stack/global aliases, altered near returns, SS=DS, A20 wrap, odd and
boundary SP, and randomized background RAM. Semantic checks assert size
rounding 1..512 and FFFFh, link order, two-sided merging, top contraction,
DF effects, duplicate-free partial writes, error/limit/transfer state and
argument rejection.

Native cases initialize all referenced list nodes in the captured 64 KiB
heap arena. The wider original-chain audit also exercises list nodes
outside that arena. An initial native fixture included such an uninitialized
node and exposed a two-byte mismatch; the final native selection constrains
nodes to initialized memory, without changing C or masking comparisons.
The supplied native frames are synthetic and do not establish natural
gameplay reach. Original SYSTEM bytes execute with IRQs masked and a far
jump at RunError entry to capture incoming state. All media, generated
assembly/COM, input snapshots and captures stay under ignored resources/.

With START.EXE under resources/DOS/POOLRAD and Capstone/Unicorn installed:

    mkdir -p resources/freemem-0254
    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/system_freemem_0254_audit.py --unicorn --native --output resources/freemem-0254/evidence.json
    cmake -S . -B resources/freemem-0254/build -DBUILD_TESTING=ON
    cmake --build resources/freemem-0254/build --target poolrad_system_freemem_0254_cpu_tests
    ctest --test-dir resources/freemem-0254/build -R '^poolrad_system_freemem_0254_cpu_tests$' --output-on-failure

## Reuse and assessment

Pre-implementation deja recall for SYSTEM FreeMem0254/03D3/0432 returned
no indexed matches. No indexed session result was reused. The word/stack,
flags, near-return and size-rounding helpers adapt accepted
[SYSTEM GetMem](SYSTEM_GETMEM_023F.md); the original-chain audit also reuses
its heap fixture and comparison hooks. GetMem itself remains unchanged.
The disk-fed native capture adapts
[PR #1005](https://github.com/huntergdavis/poolrad-decomp/pull/1005), whose
lineage includes accepted [U86 +044E](U86_SIDECAR_LIST_044E.md). There is no
runtime dependency on either overlay model.

- target_function: SYSTEM FreeMem0AF8:0254 with03D3/0432/0491.
- source_candidates: system_freemem_0254_cpu.c.
- signals: pinned entry/core/helper identities, calls/ABI/branch structure,
  ordered heap effects, CPU14/full-RAM differential, native replay, sanitizer
  semantic checks.
- score: five evidence categories (identity, structure, differential,
  native replay, contract checks); no probabilistic score.
- verdict: likely_match, pending independent verification.
- review_notes: limited original pointer validation, malformed-list budgets,
  nonwrapping reached returns, synthetic native frames and explicit RunError
  stop. Queue readiness for callers still requires independent acceptance
  and full-chain composition; this PR changes no classifier or credit data.
