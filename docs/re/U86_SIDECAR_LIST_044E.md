# U86 +044E: sidecar-gated item-list builder

The complete caller checks the active character's sidecar key 10h and a
signed-byte/class condition, updates an object gate, then appends eligible
items from three object slots through the raw U86 +028D string-list builder.
Every successful append records the current object pointer in a DS table.
Both the resident lookup and the nested heap/string operations execute as
full CPU/RAM compositions, with no host result callbacks.

## Identity and calls

The executable body is **[044E,051C), 206 bytes / 71 instructions**,
ending at RETF 2 at +0519. There is no trailing data in this extent.

- GAME.OVR file span [0x27466,0x27534).
- Body SHA-256:
  a6b87b8c682cc8664e5ada6c55e970983f20987a9780bb65b5bda61770c04606.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- Composed U86 +028D body SHA-256:
  5f82bdbca3270a4d7dcf6a93f41fa14debd04f8f4a21d8860a9b4a64d2cd5038.
- Resident +2199 body SHA-256:
  721e86627eac3e72deed41d2a26283e7c95444d99fc4f762a9714b11504dfa9c.

| Call site | Target | Composition |
| --- | --- | --- |
| 0464 | loaded 00BA:2199 | Raw sidecar lookup, RETF 0Ah |
| 04EE | near +028D after PUSH CS | Raw grouped string-list append, RETF 2 |

The source audit checks the far-call segment fixup, both target offsets,
and the two incoming PUSH CS/CALL sites: +057E in U86 +051C and +068E in
U86 +059C. C validates each reached call opcode/offset and reads the far
segment live. The +028D dependency retains its nine live SYSTEM bindings.

## Ordered behavior and profile

The argument's low byte selects the mode. The caller reserves six local
bytes and supplies its local cursor cell to the resident lookup, with the
active character at DS:5D92/5D94 and key 10h. A match clears byte +35h of
the current object at DS:6B80/6B82. On a miss, it also clears that byte if
the character's signed byte +96h is positive and the class table byte at
DS:5582 + 16*object[2Eh] equals 0Ch. The signed comparison matters for
values 80h..FFh. A nonzero remaining gate skips the item scan.

Otherwise, a byte local index begins at one. Each slot is reread from
object[index+3Bh]. Nonzero mode accepts only values strictly greater than
80h; mode zero accepts any nonzero value. Accepted bytes are passed intact
to +028D, preserving its prefix bit and low-seven-bit item index. After a
successful append, the current object pointer is reread and stored at
DS:477F + 4*byte[4843] and DS:4781 + 4*byte[4843], using the index loaded
before either store; DS:4843 is then incremented as a live byte. No host
array bound is imposed. In particular, index 49 makes the low-pointer
store overlap the counter itself; the subsequent increment uses that
changed byte. Index 255 can wrap the counter to zero.

The loop compares its live byte local with three and otherwise increments
it. Heap/stack aliases can alter this local and extend the scan, so a
caller-visit budget bounds visits to +04AD, including the first visit.
The list-walk budget resets per +028D invocation. Resident loopback and
GetMem walk-edge budgets retain their dependency definitions and may be
zero; caller/list budgets must be positive.

The profile requires exactly 1 MiB A20-off RAM, initial IP=044E and
SP in [0150,FFFA], immutable loaded code, prefixes and bindings, synchronous
execution, and CPU/limits/diagnostic storage distinct from RAM. All data,
heap and stack aliases are live. Reached dependency return frames must
remain nonwrapping, including after alias changes. Offsets wrap at 16 bits
and physical addresses at 20 bits. Logical AF follows normal/386 zero;
shift/MUL conventions remain those of the accepted dependencies. IRQs,
faults and writes overlapping code are outside the profile.

CODE stops before a malformed reached direct call. CALLEE preserves the
raw dependency stop and partial state, including nested binding errors,
list/lookup/heap limits, altered nested returns, RunError 203, or custom
HeapError. The diagnostic exposes the direct callee/status and, for APPEND,
the nested callee/status. TRANSFER preserves a changed direct return;
LOOP_LIMIT stops before the next +04AD. ARGUMENT changes no CPU/RAM and
initializes the diagnostic to NO_CALLEE/-1; null diagnostic is allowed.
No stop rolls back earlier effects. RunError/custom HeapError bodies are
explicit boundaries rather than translated continuations.

## Measured evidence

- **298 original-chain cases**: 283 returns, ten dependency stops (two
  lookup limits, two list limits, two RunError 203, one custom HeapError,
  one heap limit, one nested binding rejection, one changed nested return),
  two caller limits, two direct binding rejections, and one changed direct
  return. Every case matches CPU14 and all 1,048,576 RAM bytes.
- Caller coverage: **71/71 instruction starts, 18/18 branch outcomes**.
  The exercised chain reaches 418 instruction starts and 55 conditional
  outcomes; these are observed totals, not exhaustive dependency coverage.
- **72 DOSBox-X normal/386 cases**: 64 returns and eight RunError 203 stops;
  full CPU14 and all documented capture regions match unmasked.
- **5,391 synthetic C checks** pass ASan/UBSan; focused CTest **1/1**.
- Native RESULT.BIN SHA-256:
  99df50264a575fdfbb0bc09dca2103736f7551ac9731623ea7fb3e98ef4fab4a.
- Combined private resources/u86-044e/evidence.json SHA-256:
  4e57bec436e10016dd9e788a5d3f11e2e0c4c2052966fcc668a203901d3288a1.

The original-chain differential compares every CPU14 word and all 1 MiB
RAM bytes without masks, normalization, or replacement callee results.
Cases cover all item byte values, both modes, the 80h threshold, signed
gate extremes, found/missing/nil/cyclic sidecars, counter/table overlap and
wrap, nil/existing string lists, exact/split/top/failing heaps, explicit
budgets, malformed direct/nested calls, data/heap/stack aliases, boundary
and odd stacks, SS=DS, A20 wrap, and randomized background RAM.

Native replay runs original relocated caller, +028D, resident +2199, and
SYSTEM code with IRQs masked during execution. A far jump at RunError's
entry captures state before its body. Native comparison includes all
CPU14 words, SS1,024, DS65,536, heap65,536, and the 16,384-byte region holding
old list nodes, objects and sidecars. These supplied synthetic frames do
not establish natural gameplay reach. Original media, decoded bytes,
generated assembly/COM, inputs and captures stay under ignored resources/.

With owner media in resources/DOS/POOLRAD and Capstone/Unicorn installed:

    mkdir -p resources/u86-044e
    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u86_sidecar_list_044e_audit.py --unicorn --native --output resources/u86-044e/evidence.json
    cmake -S . -B resources/u86-044e/build -DBUILD_TESTING=ON
    cmake --build resources/u86-044e/build --target poolrad_overlay_u86_sidecar_list_044e_tests
    ctest --test-dir resources/u86-044e/build -R '^poolrad_overlay_u86_sidecar_list_044e_tests$' --output-on-failure

## Reuse and assessment

Pre-implementation local recall for poolrad U86 044E resident 2199 returned
no match; no indexed session result was reused. Memory/flags helpers and
original/native fixtures adapt accepted [U86 +028D](U86_STRING_NODE_028D.md),
[PR #992](https://github.com/huntergdavis/poolrad-decomp/pull/992). That C
implementation and its SYSTEM GetMem/StrStore/StrLoad/StrCat dependencies
are unchanged. The [resident lookup](RESIDENT_SIDECAR_LOOKUP_2199_CPU.md)
from [PR #902](https://github.com/huntergdavis/poolrad-decomp/pull/902)
is also linked unchanged. Current schema-8 queue rank 66 was the highest
substantive composable row in the continuing schema-4 workflow.

- target_function: OVL_0086_044E, executable [044E,051C).
- source_candidates: overlay_u86_sidecar_list_044e.c and unchanged raw
  U86 +028D, RES_00BA_2199, SYSTEM allocator/string dependencies.
- signals: hash/bounds, incoming calls and outgoing fixup, ordered signed
  gate/slot/counter behavior, CPU14/full-RAM differential, native replay,
  sanitizer contract checks.
- score: five evidence categories (identity, structure, differential,
  native replay, contract checks); no probabilistic score.
- verdict: likely_match, pending independent verification.
- review_notes: synthetic native frames, bounded loops, nonwrapping reached
  dependency returns, explicit allocator error stops. Queue composability
  retains these profiles. No classifier, acceptance, accounting or credit
  changes are included in this author PR.
