# U38 +1472: append fixed sidecar entry

Public entry 49 appends a nine-byte node to the supplied record's sidecar
list. The payload is key 62h, word 0, byte FFh, byte 0, followed by a nil
next pointer. The complete caller composes the raw U9E +0E52/GetMem chain
and retains its ordered CPU/RAM effects without callbacks.

## Identity and ABI

The complete body is **[1472,1490), 30 bytes / 15 instructions**, ending
at RETF 0Ah at +148D. It has no conditional branches or trailing data.

- GAME.OVR file span [0x100CD,0x100EB).
- Body SHA-256:
  80480d29f87a095087d3bfe3485de2a659702f7214c758e797a03beffdaff680.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- U9E +0E52 body SHA-256:
  cfde5096f098f1f0c4aa15f805eb6a02c3540f56a2f0df8632b99501a6366b20.

The audit asserts public entry 49, the exact body hash and final RETF,
the next function at +1490, and the call/fixup at +1487/+148A. The call
uses loaded stub 00B0:0052, whose EA target is U9E +0E52. C validates the
reached LCALL opcode/offset and EA target offset, reads the loaded segment
live, then composes the unchanged append implementation. That dependency
reads its GetMem segment binding live as well.

The caller has five word arguments; the record pointer is entry
SS:[SP+0Ah]/[SP+0Ch]. After PUSH BP, these are [BP+0Ch]/[BP+0Eh]. The other
three words are unused. It pushes the record segment and offset, changes
AL to 62h and pushes AX with the incoming AH preserved. XOR AX,AX then
sets the arithmetic flags, and the remaining pushes supply word 0,
byte FFh, and byte 0. U9E +0E52 receives a real far-call frame and returns
with RETF 0Ch. The caller then restores BP and returns with RETF 0Ah.

GetMem rounds the nine-byte request to sixteen. The append publishes the
new head or links it after the previous tail, clears its next pointer,
and writes payload fields in the dependency's original order. Every
argument, pointer, and stack read remains live, including record/list/
heap/stack aliases. Allocation and walk failures preserve partial state.

## Execution profile and stops

The profile requires exactly 1 MiB A20-off RAM, initial IP=1472 and
SP in [0040,FFF2], immutable loaded code/bindings, synchronous execution,
and CPU/diagnostic storage distinct from RAM. All reached dependency
return frames must remain nonwrapping, including after alias changes.
Offsets wrap at 16 bits and physical addresses at 20 bits. Logical AF
follows the measured normal/386 zero convention; dependency profiles
remain in force. IRQs, faults, and self-modifying code are outside scope.

GetMem walk-edge and append tail-link budgets retain their dependency
meanings and may be zero. CODE stops before a malformed direct call or
stub, after all six argument pushes and the caller's XOR flags. CALLEE
retains the append status and nested GetMem status without rolling back
earlier effects. TRANSFER preserves a changed direct return. ARGUMENT
changes no CPU/RAM and initializes the diagnostic to -1/-1; a null
diagnostic is permitted. RunError and custom HeapError bodies remain
explicit stop boundaries rather than translated continuations.

## Measured evidence

- **281 original-chain cases** match CPU14 and all 1,048,576 RAM bytes:
  265 returns, 12 dependency stops (six RunError 203, two list limits,
  two heap limits, one custom HeapError, one nested binding error),
  two changed direct returns, and two direct call/stub rejections.
- **15/15 caller starts**, with no caller conditionals. Observed chain
  coverage is 172 instruction starts and 20 conditional outcomes; this
  is not exhaustive dependency coverage.
- **120 DOSBox-X normal/386 cases**: 107 returns, 13 RunError 203 stops.
  All CPU14 words and SS1,024 + DS65,536 + heap65,536 + record/old-node
  region16,384 bytes match unmasked.
- **2,062 synthetic C checks** pass ASan/UBSan; focused CTest **1/1**.
- Native RESULT.BIN SHA-256:
  850beff291b83556281c6bf24fb10021430ef921b7d6f4f6f7c0078b7d4bafcc.
- Combined private resources/u38-1472/evidence.json SHA-256:
  56d5a0d0213a860fe91c47a55f6279133780b866f89a7066504fa6818fbce5fc.

Original-chain cases include randomized heaps and CPU registers/flags,
nil/existing lists, cyclic and budgeted lists/heaps, exact/split/top/full
allocation, custom HeapError, direct and nested binding errors, overlapping
new/old nodes and record heads, heap/stack aliases, changed returns, odd
and boundary stacks, SS=DS, A20 wrap, and randomized background RAM.
No masks, memory normalization, or replacement callee results are used.
The semantic C checks cover every incoming AH byte, exact pushed arguments
and XOR flags at a rejected call, the payload/linking/return ABI, allocator
partial failures, list limits, and argument rejection.

Native replay executes original caller/U9E/SYSTEM code with IRQs masked.
A far jump at RunError entry records its incoming state. The harness
reserves the absolute SYSTEM memory range in its COM layout. These
supplied synthetic frames do not establish natural gameplay reach.
Original media, decoded bytes, generated assembly/COM, input snapshots,
and captures remain under ignored resources/.

With owner media under resources/DOS/POOLRAD and Capstone/Unicorn installed:

    mkdir -p resources/u38-1472
    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u38_sidecar_1472_audit.py --unicorn --native --output resources/u38-1472/evidence.json
    cmake -S . -B resources/u38-1472/build -DBUILD_TESTING=ON
    cmake --build resources/u38-1472/build --target poolrad_overlay_u38_sidecar_1472_cpu_tests
    ctest --test-dir resources/u38-1472/build -R '^poolrad_overlay_u38_sidecar_1472_cpu_tests$' --output-on-failure

## Reuse and assessment

Pre-implementation recall for poolrad U38 1472 failed with an index append
EOF. deja-health confirmed the error and deja-health --repair started a
background rebuild. No indexed session result was reused. Direct reuse
follows accepted [U38 +2DE4](U38_ROLL_CLEAR_2DE4_CPU.md) word/live-stub
semantics and the [U9E append](U9E_LIST_APPEND_0E52_CPU.md) heap/list fixture
from [PR #999](https://github.com/huntergdavis/poolrad-decomp/pull/999).
The author harness adapts [PR #1003](https://github.com/huntergdavis/poolrad-decomp/pull/1003),
whose disk-fed native capture derives from accepted
[U86 +044E](U86_SIDECAR_LIST_044E.md). This PR has no code dependency on
#1003. U9E +0E52 and SYSTEM GetMem are linked unchanged.

- target_function: OVL_0038_1472, public entry 49, [1472,1490).
- source_candidates: overlay_u38_sidecar_1472_cpu.c and unchanged raw
  U9E +0E52 / SYSTEM GetMem.
- signals: body/entry/bounds and call fixup, exact push/XOR ABI,
  CPU14/full-RAM differential, native replay, semantic sanitizer checks.
- score: five evidence categories (identity, structure, differential,
  native replay, contract checks); no probabilistic score.
- verdict: likely_match, pending independent verification.
- review_notes: synthetic native frames, nonwrapping dependency returns,
  bounded heap/list walks and explicit allocator error stops. No classifier,
  acceptance, accounting or credit changes in this author PR.

## Checker registration (2026-10-29)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0038_1472 --require-current-main
```
