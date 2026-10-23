# U38 +1730: roll and append sidecar

Public entry 60 rolls 1d100 through the raw U9E +0DE3/SYSTEM chain. On a
result at most 95, it calls raw U9E +0E52/GetMem to append a nine-byte node
to the supplied record's sidecar list. The node contains key 19h, word 1,
byte 0Ch, byte 0, and a nil next pointer. A result above 95 returns after
the RNG effects without allocation. No result callbacks are substituted.

## Exact body and bindings

The executable body is **[1730,175E), 46 bytes / 22 instructions**,
ending at RETF 0Ah at +175B. The queue's 63-byte / 29-instruction upper
bound includes **17 trailing data bytes** before the next function at
+176F. They are excluded from the executable hash and translation.

- GAME.OVR file span [0x1038B,0x103B9).
- Body SHA-256:
  402e83b7c5036596f9af8cb3da21ea77519e58cca195e5385fc88d593e0e2f25.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- U9E dice span [0DE3,0E52) SHA-256:
  9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b.
- U9E append body [0E52,0F06) SHA-256:
  cfde5096f098f1f0c4aa15f805eb6a02c3540f56a2f0df8632b99501a6366b20.

| Call site | Loaded stub | Raw dependency |
| --- | --- | --- |
| 1739 | 00B0:0048 | U9E +0DE3, roll 1d100, RETF 4 |
| 1755 | 00B0:0052 | U9E +0E52, append, RETF 0Ch |

Both segment words have loader fixups. The audit asserts public entry 60,
body hash/exit, next function bound and both call targets/fixups. C checks
the reached LCALL opcode/offset and EA target offset, reads the loaded
segments live, and preserves the dependencies' complete state changes.

## ABI, ordering and limits

The caller has five word arguments and returns with RETF 0Ah. Its supplied
record pointer is entry SS:[SP+0Ah]/[SP+0Ch], or [BP+0Ch]/[BP+0Eh] after
PUSH BP. The other three words are unused. The dice call receives count 1
and sides 100 by changing AL and pushing the then-current AX. After the
roll, CMP AL,5Fh sets the flags for the unsigned JA.

On the append path, the caller rereads and pushes the supplied record's
segment then offset, changes AL to 19h and pushes AX, writes AX=1 and
pushes it, then pushes AX with AL=0Ch and AL=0. U9E +0E52 allocates nine
bytes, rounded by GetMem to sixteen, appends to record+7Fh/+81h through
node links +5/+7, and writes the payload in its original order. All stack,
record, list, and heap aliases remain live. A failed allocation retains
the RNG seed update and all earlier stack writes.

The profile requires exactly 1 MiB A20-off RAM, initial IP=1730 and
SP in [0040,FFF2], immutable loaded code/bindings, synchronous execution,
and CPU/limits/diagnostic objects distinct from RAM. All reached dependency
return frames must remain nonwrapping, including after alias changes.
Offsets wrap at 16 bits and physical addresses at 20 bits. U9E dice and
SYSTEM retain their pinned original layout, with SYSTEM at segment 0AF8
and its live multiplier at +1245. Dependencies retain their documented
flags conventions, including the measured normal/386 logical AF behavior.
IRQs, faults, and self-modifying code are outside this profile.

The three budgets retain dependency meanings and may be zero: SYSTEM
call slices in the dice composition, GetMem free-list walk edges, and
append tail links. CODE stops before a malformed reached direct call or
stub. CALLEE retains the dependency's exact state/status; APPEND also
exposes its nested GetMem status. TRANSFER preserves a changed direct
return. ARGUMENT changes no CPU/RAM and initializes the diagnostic to
NO_CALLEE/-1; a null diagnostic is allowed. No stop rolls back effects.
RunError/custom HeapError bodies are explicit stops, not translated
continuations.

## Evidence and reproduction

- **426 original-chain cases**, with CPU14 and all 1,048,576 RAM bytes
  matching: 413 returns, seven dependency stops (one dice limit, two list
  limits, RunError 203, custom HeapError, heap limit, nested binding error),
  two changed direct returns, and four rejected direct call/stub bindings.
- **22/22 caller starts and 2/2 branch outcomes**. Actual original rolls
  include both threshold values 95 and 96. The exercised chain reaches
  240 instruction starts and 25 conditional outcomes; these are observed
  totals, not exhaustive dependency coverage.
- **120 DOSBox-X normal/386 cases**: 107 returns and 13 RunError 203 stops.
  CPU14, SS1,024, DS65,536, heap65,536, and record/old-node16,384 bytes match
  unmasked. Both caller paths execute original dice/append/SYSTEM code.
- **3,159 synthetic C checks** pass ASan/UBSan; focused CTest **1/1**.
- Combined private resources/u38-1730/evidence.json SHA-256:
  f9c9d1509dc2896fa267802ad08809daabe9f47bb08073841eb731e225ffade8.
- Native RESULT.BIN SHA-256:
  2c0b88ad24bab21d95c63af4534006e75479b3bab5430bb9f585a79e0f275a89.

The original differential uses complete CPU/RAM comparisons without flag
masks, normalized memory, or substituted callee results. Cases include
400 seeds, nil/existing lists, cyclic/tightly budgeted lists, exact/split/
top/failing heaps, custom HeapError, nested and direct binding errors,
record and stack aliases, changed returns, odd and boundary stacks,
SS=DS, A20 wrap, and randomized background RAM. The semantic C tests
independently compute the documented seed recurrence and high-word
remainder, then check branch choice, payload, linking, seed updates,
return ABI and failure diagnostics.

The native harness reserves COM offsets 2E40..4087 for absolute SYSTEM
code and places data blobs above 4100. IRQs are masked during execution;
a far jump at RunError's entry captures its incoming state. The frames
are synthetic and do not demonstrate natural gameplay reach. Original
media, decoded bytes, generated assembly/COM, input snapshots and output
captures stay under ignored resources/.

With owner media under resources/DOS/POOLRAD and Capstone/Unicorn installed:

    mkdir -p resources/u38-1730
    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u38_roll_append_1730_audit.py --unicorn --native --output resources/u38-1730/evidence.json
    cmake -S . -B resources/u38-1730/build -DBUILD_TESTING=ON
    cmake --build resources/u38-1730/build --target poolrad_overlay_u38_roll_append_1730_cpu_tests
    ctest --test-dir resources/u38-1730/build -R '^poolrad_overlay_u38_roll_append_1730_cpu_tests$' --output-on-failure

## Reuse and assessment

Pre-implementation recall for poolrad U38 1730 0DE3 0E52 returned no
match; no indexed session result was reused. Direct reuse follows accepted
[U38 +2DE4](U38_ROLL_CLEAR_2DE4_CPU.md) memory/flags/live-stub composition
and dice fixtures, the [U9E append](U9E_LIST_APPEND_0E52_CPU.md) original
heap/list fixture from [PR #999](https://github.com/huntergdavis/poolrad-decomp/pull/999),
and the disk-fed [U86 +044E](U86_SIDECAR_LIST_044E.md) native capture design.
All U9E and SYSTEM C dependencies are linked unchanged.

- target_function: OVL_0038_1730, public entry 60, executable [1730,175E).
- source_candidates: overlay_u38_roll_append_1730_cpu.c with unchanged
  raw U9E +0DE3/+0E52 and SYSTEM RNG/seed/GetMem implementations.
- signals: hash/entry/bounds, two call fixups, threshold and argument ABI,
  CPU14/full-RAM differential, native replay, semantic sanitizer checks.
- score: five evidence categories (identity, structure, differential,
  native replay, contract checks); no probabilistic score.
- verdict: likely_match, pending independent verification.
- review_notes: queue extent includes 17 data bytes; synthetic native frames;
  fixed SYSTEM layout, bounded dependencies and nonwrapping return frames.
  No classifier, acceptance, accounting or credit changes in this author PR.

## Checker registration (2026-10-29)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_0038_1730 --require-current-main
```
