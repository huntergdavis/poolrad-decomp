# U38 +2814: two sidecar lookups and conditional append

Public entry85 looks for key62h in a record's sidecar list, then key3Bh.
Either hit returns immediately. If both miss, the raw U9E +0E52/GetMem
chain appends a node with key3Bh, word3, byteFFh, byte1 and nil next.
The implementation composes the unchanged raw resident lookup and append
models, preserving the full CPU and ordered RAM effects without callbacks.

## Identity and ABI

The complete body is **[2814,2866), 82 bytes / 37 instructions**, ending
at RETF0Ah at +2863. Two conditionals have four outcomes; no trailing data.

- GAME.OVR file span [0x1146F,0x114C1).
- Body SHA-256:
  e588698b50ccee5e8ce1d48aefa38594666e742bb3cb9ac643cd67e8ace8ce08.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- Resident2199 SHA-256:
  721e86627eac3e72deed41d2a26283e7c95444d99fc4f762a9714b11504dfa9c.
- U9E +0E52 SHA-256:
  cfde5096f098f1f0c4aa15f805eb6a02c3540f56a2f0df8632b99501a6366b20.

The audit asserts entry85, body hash/count/bounds, next function at2866,
all three LCALL targets and their segment fixups. Lookup calls +2828 and
+283F target resident2199; append +285B targets stub0052 / U9E0E52. Loaded
segments are read live. Reached opcode/offset and append EA binding are
validated before making real far-call frames. Resident2199 has no callees;
U9E0E52 reads its GetMem binding live.

The five word arguments include record offset/segment at entry SS:SP+0Ah/
0Ch; the other three words are unused. PUSH BP/MOV BP,SP/SUB SP,4 allocate
a local cursor at SS:[BP-4]. Each lookup pushes the live record pointer,
key word with AH retained, and that cursor address. A hit's AL is tested
with OR AL,AL. A miss proceeds to the next lookup or append. The lookup
budget resets per call. Both lookups mutate the same live cursor, which
may alias the record head or node fields.

On two misses, the append pushes record segment/offset, key3Bh, word3,
byteFFh and byte1. MOV AX,3 clears AH for the last three pushes. The
append allocates nine bytes rounded to sixteen, links the node, clears
next, and writes payload fields in the dependency's original order. The
caller restores SP/BP and RETF0Ah. Stack/heap/record aliases and changed
callee returns are preserved; no Boolean normalization or rollback occurs.

## Execution profile and stops

Exactly 1 MiB A20-off RAM; entryIP2814 and conservative SP[0040,FFF2].
CPU, limits and diagnostic storage are distinct from RAM. Loaded code and
bindings are immutable; all reached dependency return frames must remain
nonwrapping, including after alias-induced changes. Offset arithmetic wraps
at16 bits and physical addresses at20 bits. Dependency profiles apply;
logical AF follows the measured normal/386 zero convention. IRQs, faults
and self-modifying code are outside scope.

All three dependency budgets may be zero. Lookup loop-back limits reset
for each call. CODE stops before a malformed reached call/stub and retains
the last completed-call diagnostic; skipped calls do not validate their
bindings. The first CODE boundary retains SUB SP,4 flags, and later ones
retain the appropriate OR AL,AL flags. CALLEE retains raw status, nested
GetMem status and exact partial state. TRANSFER retains a changed direct
return. ARGUMENT changes no CPU/RAM and initializes diagnostic -1/-1/-1;
null diagnostic is allowed, null limits rejected. Diagnostic callee is -1
for none, 0 for lookup, 1 for append. RunError/custom HeapError bodies are
explicit dependency stops, not translated continuations.

## Measured evidence

- **295 original-chain cases**: CPU14 + all1,048,576 RAM bytes unmasked.
  264 returns, 26 dependency stops (17 RunError203, four lookup limits,
  one append-list limit, two heap limits, custom HeapError, nested binding),
  one changed direct return, four direct binding rejections.
- **37/37 caller starts and4/4 conditional outcomes**. Observed chain:
  231 starts /30 conditional outcomes; not exhaustive dependency coverage.
- **120 DOSBox-X normal/386 cases**:107 returns /13 RunError203. CPU14,
  SS1,024 + DS65,536 + heap65,536 + record/old-node16,384 bytes unmasked.
- **2,072 ASan/UBSan semantic checks**. Focused CTest **1/1**.
- Native RESULT.BIN SHA-256:
  c964874832897afa50c401c4d4a501cd6b777b55eb247acacceea5958a4a3d89.
- Combined private resources/u38-2814/evidence.json SHA-256:
  e874c98c2e0721fbd585ee13fc515317d111535437c273d21cee88b148c78fd7.

Cases include first/second/head/tail/both hits, two misses, per-call budget
reset, found-at-zero-budget stops, cyclic lists, cursor/record/node aliases,
randomized heaps/CPU registers/flags, exact/split/top/full allocations,
custom HeapError, direct/nested binding errors, skipped malformed calls,
heap/stack aliases, changed returns, odd/boundary SP, SS=DS, A20 wrap and
random background RAM. Semantic tests check every incoming AH byte,
payload/link order, exact argument pushes/SUB flags at first rejection,
second/append rejections, skipped malformed calls and partial diagnostics.

Native executes original caller/resident/U9E/SYSTEM code with IRQs masked
and a far jump at RunError entry capturing incoming state. The resident
lookup is relocated to7000:2199 through the caller's loaded segment fixups;
its body is unchanged. Native frames are synthetic and do not establish
natural gameplay reach. All original media, decoded bytes, generated
assembly/COM and captures remain ignored under resources/.

With owner media in resources/DOS/POOLRAD and Capstone/Unicorn installed:

    mkdir -p resources/u38-2814
    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u38_lookup_append_2814_audit.py --unicorn --native --output resources/u38-2814/evidence.json
    cmake -S . -B resources/u38-2814/build -DBUILD_TESTING=ON
    cmake --build resources/u38-2814/build --target poolrad_overlay_u38_lookup_append_2814_cpu_tests
    ctest --test-dir resources/u38-2814/build -R '^poolrad_overlay_u38_lookup_append_2814_cpu_tests$' --output-on-failure

## Reuse and assessment

Pre-implementation deja recall timed out during an index update.
deja-health found an interrupted staging directory and timed-out probe;
deja-health --repair started a background rebuild. No indexed result was
reused. Direct reuse follows [U38 +1472](U38_SIDECAR_1472_CPU.md) composition
and native capture, [U86 +044E](U86_SIDECAR_LIST_044E.md) raw lookup handling,
[resident2199](RESIDENT_SIDECAR_LOOKUP_2199_CPU.md), and the
[U9E append](U9E_LIST_APPEND_0E52_CPU.md) fixture. Resident2199, U9E0E52
and SYSTEM GetMem source and interfaces remain unchanged.

- target_function: OVL_0038_2814, entry85, complete [2814,2866).
- source_candidates: overlay_u38_lookup_append_2814_cpu.c and unchanged
  raw resident2199 / U9E0E52 / SYSTEM GetMem.
- signals: body/entry/bounds/fixups, two-stage lookup control flow and ABI,
  CPU14/full-RAM differential, native replay, semantic sanitizer checks.
- score: five evidence categories (identity, structure, differential,
  native replay, contract checks); no probabilistic score.
- verdict: likely_match, pending independent verification.
- review_notes: synthetic native frames, nonwrapping reached returns,
  bounded walks and explicit allocator stops. Queue readiness indicates
  composition eligibility, not acceptance. No classifier, accounting,
  acceptance or credit edits.
