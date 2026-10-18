# U1C +18F8: raw ON GOTO / ON GOSUB

The complete caller loads a selector and target count, loads the target
operands, then selects a zero-based target. Opcode DS:6F7F = 25h writes
the script PC directly; every other opcode pushes the return PC onto the
raw GOSUB heap list before jumping. Out-of-range selectors leave the PC
after the target list. The implementation composes the merged raw U25
loader, slot accessor, word assembler, and list allocator without hooks.

## Boundary and identity

The executable body is **[18F8,198D), 149 bytes / 57 instructions**,
ending at RETF +198C. The queue's 184-byte / 75-instruction upper bound
includes 35 trailing data bytes before the next function at +19B0.
Those bytes are not part of this translation or its body hash.

- GAME.OVR file span [0x263E,0x26D3).
- Body SHA-256:
  c51ab20d9189f38a22d05ad50624117f0ff0dc3251b3921fd319e53454f2c8ba.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- Whole loaded U25 SHA-256:
  cbffa29c694cb0fdd04c85cd93c0f8353ddcce64aab71629a45929eb9234ba74.

| Call sites | Loaded overlay stub | Raw dependency |
| --- | --- | --- |
| 1905, 1928 | 002B:002A | U25 +0000 operand loader |
| 190D, 1918 | 002B:0025 | U25 +0136 slot value |
| 1968 | 002B:004D | U25 +06AF word assembler |
| 1979 | 002B:0075 | U25 +1951 GOSUB list push |

Each caller segment word has a loader fixup. The implementation validates
the reached LCALL opcode/offset and the stub's EA target offset, reads
the loaded segment live, and preserves all callee effects. U25 +1951
reads its own GetMem segment words live and uses the existing SYSTEM
allocator. The source audit asserts all six caller sites and fixups.

## Contract and partial effects

Entry is a far-call frame at IP=18F8, with no arguments; RETF consumes
four bytes. PUSH BP reserves six local bytes. The first loader consumes
two operands; the two slot-accessor results are truncated to AL for
selector and count. DEC DS:49ED backs over the loader's final PC advance
before the second loader. The loop rereads index, count, match flag,
opcode, and selected target bytes in original instruction order.

Counts span all byte values, including zero and 255. There is no host
slot-array limit: the original tag/high/low tables overlap above 64
entries, and these writes and subsequent reads are retained. GOSUB's
six-byte allocation is rounded by GetMem; word zero stores the PC after
the target list and words +2/+4 link the previous head. A failed GetMem
preserves the pre-allocation list head and all earlier loader/stack writes.

The profile requires exactly 1 MiB A20-off RAM, initial SP in
[0238,FFFC], immutable loaded code/bindings, synchronous execution, and
CPU/limits/diagnostic objects distinct from RAM. Data, heap and stack may
alias. All reached dependency return frames must remain nonwrapping,
including after alias-induced changes. Logical AF uses the measured
normal/386 zero convention. Interrupts, faults, and self-modifying code
are outside the profile.

All dependencies retain their documented contracts. In particular,
U25 +0136 has no party-list iteration cap: its party-index route requires
a terminating list. Queue composability does not guarantee termination
for arbitrary corrupted lists. SYSTEM RunError and non-default HeapError
bodies are explicit stops, not translated continuations.

Operand/string/caller budgets must be positive. Loader budgets reset per
invocation, string budgets per string-core invocation. The caller budget
counts visits to +1931, including the final exit check; heap_steps counts
GetMem's taken free-list walk edges and may be zero. Stops preserve exact
partial CPU/RAM: CODE before a malformed binding, CALLEE at the dependency
stop, CALLER_LIMIT at +1931, or TRANSFER after an altered direct return.
The diagnostic names the last dependency and status; ALLOC also exposes
its nested dependency/status. ARGUMENT changes neither CPU nor RAM and
initializes the diagnostic. A null diagnostic is permitted.

## Evidence and reproduction

- **333 original-chain cases**: 311 returns, six dependency stops
  (two operand limits, one string limit, RunError 203, custom HeapError,
  heap-walk limit), two caller limits, two altered returns, and 12 binding
  rejections. Every case matches CPU14 and all 1,048,576 RAM bytes.
- Caller coverage **57/57 instruction starts and 8/8 branch outcomes**.
  The exercised chain reaches **566 instruction starts and 75 conditional
  outcomes**; these are observed totals, not a claim of exhaustive runtime
  coverage. The underlying dependencies retain their separate evidence.
- **58 DOSBox-X normal/386 cases**: 54 returns and four RunError 203
  exits; CPU14 + SS1,024 + DS65,536 + heap16,384 bytes match unmasked.
  Both direct and memory-backed selector/count paths are included.
- **1,413 synthetic C checks** pass under ASan/UBSan; focused CTest **1/1**.
- Native result SHA-256:
  a29534d35774c90daabb3d6038c92b295bafe8b5d0d0f4102ae0ce60f791d94a.
- Private receipt resources/u1c-18f8/evidence.json SHA-256:
  b771a3d3152386d606f069cba17f7ab5103ecec2f93373838f5dab1a614c7fb5.

The original-chain differential compares all CPU14 words and the entire
1 MiB RAM, with no flag masks, memory normalization, or replacement callee
results. Inputs cover all counts 0..255, selectors before/at/after the
range, nil/existing GOSUB heads, literal and memory-backed selector/count,
all loader tag classes, string-core budget exhaustion, exact/split/top
allocation, full/custom/budgeted heaps, all six malformed call sites,
malformed stubs, caller/operand limits, altered loader returns, odd and
boundary stacks, SS=DS, A20 wrap, and a randomized RAM background.

The DOSBox-X oracle runs original relocated caller/U25/SYSTEM code with
IRQs masked. A far jump at RunError's entry records the state before
RunError executes. These supplied synthetic frames do not demonstrate
natural gameplay reach. Native comparison covers all 14 CPU words,
1,024 stack bytes, all 65,536 DS bytes, and a 16,384-byte heap arena.
Original media, expanded/decoded bytes, generated assembly/COM, input
snapshots, and output captures remain under ignored resources/.

With owner media under resources/DOS/POOLRAD and Capstone/Unicorn installed:

    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u1c_on_goto_18f8_cpu_audit.py --unicorn --native --output resources/u1c-18f8/evidence.json
    cmake -S . -B resources/u1c-18f8/build -DBUILD_TESTING=ON
    cmake --build resources/u1c-18f8/build --target poolrad_overlay_u1c_on_goto_18f8_cpu_tests
    ctest --test-dir resources/u1c-18f8/build -R '^poolrad_overlay_u1c_on_goto_18f8_cpu_tests$' --output-on-failure

## Reuse and parity assessment

Pre-implementation local recall for U1C 18F8 returned no session match.
The existing [bounded model from PR #189](https://github.com/huntergdavis/poolrad-decomp/pull/189)
and [its report](U1C_ON_GOTO_18F8.md) establish prior semantic work; that
model is unchanged. Its earlier supplied-state probe is not reused as
native reach evidence. Memory/flags primitives adapt accepted U3C2 +0909;
live-stub composition and the script fixture adapt U1C +00F4. The native
capture design follows GetMem/U6A +0E37, with disk-fed synthetic inputs.
The raw list allocator from [PR #984](https://github.com/huntergdavis/poolrad-decomp/pull/984)
and GetMem from [PR #983](https://github.com/huntergdavis/poolrad-decomp/pull/983)
are linked unchanged, as are the loader/string cores and full slot accessor.

- target_function: OVL_001C_18F8, executable [18F8,198D).
- source_candidates: overlay_u1c_on_goto_18f8_cpu.c and the unchanged raw
  U25 +0000/+0136/+06AF/+1951 dependency implementations.
- signals: executable hash/bounds, six call/fixup sites, four caller
  conditionals, stack/table/list ordering, CPU14/RAM differential,
  native heap/error replay, semantic sanitizer tests.
- score: five evidence categories (identity, structure, differential,
  native replay, contract tests); no probabilistic score.
- verdict: likely_match, pending independent verification.
- review_notes: synthetic native frames; terminating-party-list and
  nonwrapping dependency-return profiles; explicit allocator error stops;
  queue extent correction for the independent lane. No classifier,
  acceptance, accounting, or credit is changed by this author PR.
