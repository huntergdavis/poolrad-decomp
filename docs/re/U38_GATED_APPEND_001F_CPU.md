# U38 +001F: gated sidecar append

The complete internal helper checks DS:[681F]. A nonzero gate stores a
zero result byte in its stack local and skips allocation. A zero gate
stores one, then appends the supplied payload through raw U9E +0E52 and
SYSTEM GetMem. Both paths return AL loaded from the live stack local;
heap/stack aliases can change that result and the return frame.

## Identity and ABI

The body is **[001F,0058), 57 bytes / 22 instructions**, ending at RETF
0Ah at +0055. One conditional has two outcomes; there is no trailing data.
This is an internal helper, not a public overlay entry. The queue records
eight aligned PUSH CS / near CALL callers in U38.

- GAME.OVR file span [0xEC7A,0xECB3).
- Body SHA-256:
  342e00b6d9cd4b29bce77746ca25f02637040d3514807fe6063ddaa175bd23f8.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- U9E +0E52 SHA-256:
  cfde5096f098f1f0c4aa15f805eb6a02c3540f56a2f0df8632b99501a6366b20.

The audit pins the body hash, decoded count, final RETF, next function
boundary +0058, and LCALL/fixup +004A/+004D. It executes the original
caller, EA stub, U9E append and GetMem instructions without callee result
substitution. The loaded stub segment and its U9E target segment remain
live; the dependency reads its GetMem binding live as well.

The five word arguments at entry SS:SP+4 are a, c, b, record offset,
record segment. PUSH BP / MOV BP,SP / SUB SP,2 precede the gate read;
therefore a saved-BP write aliasing the gate affects the branch. The
local is SS:[BP-1]. On the append path, the pushes are record segment,
record offset, AX with AL=b, word a, AX with AL=c, AX with AL=1. Byte
loads preserve the incoming AH. The append receives a real far-call
frame, allocates nine bytes (rounded to sixteen), links the new node,
clears next, then writes b, a, d=1, c in the original order.

CMP byte,0 sets flags on both paths. SUB SP flags are overwritten before
any externally observable stop. The epilogue reads AL from the live
local, restores SP/BP, and RETF0Ah consumes the five words. No assumption
that AL must still be 0/1 is made after aliasing callee writes.

## Execution profile

Exactly 1 MiB A20-off RAM, entry IP001F and conservative SP[0040,FFF2].
CPU and diagnostic storage must be distinct from RAM. Loaded code and
bindings are immutable; all reached dependency return frames must remain
nonwrapping, including after alias changes. Data/heap/stack aliases stay
live. Addresses wrap at 16-bit offsets and 20-bit physical addresses.
The unchanged dependency profiles apply; logical AF uses the measured
normal/386 convention. IRQs, faults and self-modifying code are excluded.

Heap-walk and tail-link budgets retain dependency semantics and may be
zero. CODE stops before a malformed reached call/stub, after the argument
pushes. A skipped call does not inspect its binding. CALLEE retains raw
append/GetMem status and exact partial state. TRANSFER retains a changed
direct callee return. ARGUMENT leaves CPU/RAM unchanged and initializes
the optional diagnostic to -1/-1. No rollback occurs. RunError and custom
HeapError bodies are explicit stop boundaries, not translated continuations.

## Evidence

- **541 original-chain cases**, CPU14 + all 1,048,576 RAM bytes unmasked:
  522 returns, 15 dependency stops (eight RunError203, three list budgets,
  two heap budgets, custom HeapError, nested binding), two changed direct
  returns, two direct binding rejections.
- **22/22 caller instruction starts and 2/2 conditional outcomes**. Observed
  chain coverage: 179 starts / 22 conditional outcomes, not an exhaustive
  dependency coverage claim.
- **120 DOSBox-X normal/386 cases**: 110 returns / 10 RunError203. CPU14,
  SS1,024 + DS65,536 + heap65,536 + record/old-node16,384 bytes unmasked.
- **3,082 ASan/UBSan semantic checks**; focused CTest **1/1**.
- Native RESULT.BIN SHA-256:
  e921d0f67e640a1b1b52d16dd32d8b79668f83ba2b04818bcd0b0f7c0515c17b.
- Combined private resources/u38-001f/evidence.json SHA-256:
  347cd934374589d492a8bcb98465014d9e58bc13e8a4b91026ef99f8d66c3f30.

Original cases cover all 256 gate bytes, all incoming AH bytes in the C
semantic tests, randomized payload words/CPU registers/flags/heaps,
nil/existing/cyclic lists, exact/split/top/full allocations, custom
HeapError, heap/list budgets, malformed direct/nested bindings, skipped
malformed calls, saved-BP/gate/local aliases, record/new/old-node aliases,
changed direct returns, odd/boundary SP, SS=DS, A20 wrap and random RAM.
No comparison masks or memory normalization are applied.

Native replay uses original caller/U9E/SYSTEM instructions with IRQs
masked and a far jump at RunError entry to record its incoming state.
The COM layout reserves the absolute SYSTEM range. Native frames are
supplied synthetic inputs; these runs do not establish natural gameplay
reach. Original media, decoded bytes, generated assembly/COM and captures
remain private under ignored resources/.

With owner media in resources/DOS/POOLRAD and Capstone/Unicorn installed:

    mkdir -p resources/u38-001f
    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u38_gated_append_001f_audit.py --unicorn --native --output resources/u38-001f/evidence.json
    cmake -S . -B resources/u38-001f/build -DBUILD_TESTING=ON
    cmake --build resources/u38-001f/build --target poolrad_overlay_u38_gated_append_001f_cpu_tests
    ctest --test-dir resources/u38-001f/build -R '^poolrad_overlay_u38_gated_append_001f_cpu_tests$' --output-on-failure

## Reuse and assessment

Pre-implementation deja recall for the U38 candidates timed out after
updating four files; deja-health then reported healthy recall. No indexed
target result was reused. The author harness and live-stub composition
adapt [PR #1005](https://github.com/huntergdavis/poolrad-decomp/pull/1005),
which is pending independent review. This target has no code dependency
on that PR. Its harness lineage includes
[PR #1003](https://github.com/huntergdavis/poolrad-decomp/pull/1003) and
accepted [U86 +044E](U86_SIDECAR_LIST_044E.md). Word/return semantics follow
accepted [U38 +2DE4](U38_ROLL_CLEAR_2DE4_CPU.md). The unchanged
[U9E +0E52](U9E_LIST_APPEND_0E52_CPU.md) implementation and fixtures from
[PR #999](https://github.com/huntergdavis/poolrad-decomp/pull/999) compose
unchanged SYSTEM GetMem.

- target_function: OVL_0038_001F, complete internal [001F,0058).
- source_candidates: overlay_u38_gated_append_001f_cpu.c and unchanged
  raw U9E append / SYSTEM GetMem.
- signals: body/bounds/call fixup, gate/local/push ABI and branch structure,
  CPU14/full-RAM differential, native replay, semantic sanitizer checks.
- score: five evidence categories (identity, structure, differential,
  native replay, contract checks); no probabilistic score.
- verdict: likely_match, pending independent verification.
- review_notes: synthetic native frames, nonwrapping dependency returns,
  explicit allocator stops and bounded walks. No classifier, acceptance,
  accounting or credit edits.
