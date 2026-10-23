# U1C +0111: raw GOSUB chain

ECL opcode 02 saves the script PC after loading its target operand, links
that return address into a six-byte heap node, then installs the target PC.
This separate implementation composes the existing raw loader, string cores,
GOSUB allocator, word assembler, and SYSTEM GetMem. It closes the callback
boundary in the [provisional wrapper](U1C_GOSUB_FULLSTATE_0111.md); the older
API and its report remain unchanged.

## Identity and composition

The complete executable body is **[0111,0126), 21 bytes / 10 instructions**,
ending at RETF +0125, with no conditional branches or trailing data.

- GAME.OVR file span [0x0E57,0x0E6C).
- Body SHA-256:
  7917b6be7b4ae2f3d53de4c2a9d80ec430fba207ca34e689e88ac2186c4b0edc.
- GAME.OVR SHA-256:
  872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.
- START.EXE SHA-256:
  d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
- Whole loaded U25 SHA-256:
  cbffa29c694cb0fdd04c85cd93c0f8353ddcce64aab71629a45929eb9234ba74.

| Call site | Loaded stub | Dependency |
| --- | --- | --- |
| 0117 | 002B:002A | U25 +0000: load one operand |
| 011F | 002B:0075 | U25 +1951: push GOSUB frame and select target |

The source audit asserts both call offsets and segment fixups. C validates
each reached LCALL and EA stub, reads its loaded segment live, and composes
all callee CPU/RAM effects. U25 +1951 reads its GetMem binding live, allocates
six bytes (rounded to eight), stores the advanced PC and old head, publishes
the new head at DS:6F70/6F72, then uses +06AF to assemble the target from
DS:7046/7086. Loader tags 1/2/3 consume a high byte; tag 0 and unrecognized
tags retain the stale high cell. String tags use the existing raw cores.

## Contract and stops

Entry IP is 0111, with a far return frame and no arguments. Initial SP must
be in [0232,FFFC]. Both calls receive AX with AL set to one; the then-current
AH is preserved. The caller itself changes no flags. POP BP and RETF read
the live stack, so data/heap/stack aliases are retained in instruction order.

The profile requires exactly 1 MiB A20-off RAM, immutable loaded code and
bindings, synchronous execution, and CPU/limits/diagnostic storage distinct
from RAM. Effective offsets wrap at 16 bits and physical addresses at 20.
Every reached dependency return frame must remain nonwrapping, including
after alias-induced changes. Dependencies retain their documented profiles;
logical AF follows the measured normal/386 zero convention. Interrupts,
faults, and writes overlapping code are outside this profile.

Operand and string budgets must be positive; GetMem's walk-edge budget may
be zero. CODE stops before a malformed reached call or stub. CALLEE preserves
the dependency's exact stop and partial state; its diagnostic includes the
last callee/status and, for the list allocator, the nested callee/status.
TRANSFER preserves an altered direct return. ARGUMENT changes no CPU/RAM
and initializes the diagnostic; a null diagnostic is allowed. No stop rolls
back earlier effects. RunError and non-default HeapError bodies are explicit
boundaries, not translated continuations.

## Measured evidence

- **283 original-chain cases**, comparing CPU14 and every byte of 1 MiB RAM:
  265 returns, 12 dependency stops (seven RunError 203, two heap limits,
  one custom HeapError, one operand limit, one string limit), two changed
  direct returns, and four rejected bindings.
- **10/10 caller instruction starts**, no caller conditionals. The exercised
  chain reaches 567 instruction starts and 62 conditional outcomes. These
  are observed totals, not exhaustive dependency coverage.
- **72 DOSBox-X normal/386 cases**: 54 returns and 18 RunError 203 stops.
  All CPU14 words, SS1,024, DS65,536, and heap16,384 bytes match unmasked.
- **2,068 synthetic C checks** pass with ASan/UBSan. Focused CTest: **1/1**.
- Native RESULT.BIN SHA-256:
  94ae076dc94914909fd266fceaba4b7fcc649f8087d4a931af111f366997ec6b.
- Combined private resources/u1c-0111/evidence.json SHA-256:
  79527e108ee78335e59ea62365c037837168724598f533d846673abe2a2dd2c2.

Original-chain cases cover all 256 tag bytes, stale high bytes and flags,
nil/existing heads, exact/split/top/failing/custom/cyclic heaps, budget stops,
string operands, operand-count aliases, changed return frames, boundary and
odd stacks, SS=DS, A20 wrap, randomized RAM, and node/table/old-head/stack
aliases. No flag masks, memory normalization, or substituted callee results
are used. Native fixtures cover canonical and unknown tags, strings, old
heads, and heap outcomes, with original relocated caller/U25/SYSTEM code.
IRQs are masked during execution; a far jump at RunError entry captures
state before its body executes.

These native frames are synthetic and do not establish natural gameplay
reach. Original media, decoded bytes, generated assembly/COM, input snapshots,
and captures remain in ignored resources/. Queue readiness identifies
composable dependencies; it does not remove the documented execution profile.

With owner media under resources/DOS/POOLRAD and Capstone/Unicorn installed:

    PYTHONDONTWRITEBYTECODE=1 python3 tools/overlay/u1c_gosub_0111_cpu_audit.py --unicorn --native --output resources/u1c-0111/evidence.json
    cmake -S . -B resources/u1c-0111/build -DBUILD_TESTING=ON
    cmake --build resources/u1c-0111/build --target poolrad_overlay_u1c_gosub_0111_cpu_tests
    ctest --test-dir resources/u1c-0111/build -R '^poolrad_overlay_u1c_gosub_0111_cpu_tests$' --output-on-failure

## Reuse and assessment

Pre-implementation recall for U1C 0111 GOSUB timed out during indexing;
deja-health reported an incomplete staging directory and probe timeout,
and deja-health --repair started a background rebuild. No indexed result
was reused. Direct source reuse follows the provisional report above and
accepted [U1C +00F4](U1C_GOTO_00F4_CPU.md) live-stub composition and fixture.
The differential/native harness adapts [U1C +18F8](U1C_ON_GOTO_18F8_CPU.md)
from [PR #989](https://github.com/huntergdavis/poolrad-decomp/pull/989).
Loader/string cores, [U25 +1951](https://github.com/huntergdavis/poolrad-decomp/pull/984),
and [SYSTEM GetMem](https://github.com/huntergdavis/poolrad-decomp/pull/983)
are linked unchanged.

- target_function: OVL_001C_0111, executable [0111,0126).
- source_candidates: overlay_u1c_gosub_0111_cpu.c with unchanged raw
  U25 +0000/+1951/+06AF, string cores, and SYSTEM GetMem.
- signals: body hash/bounds, two call/fixup sites, stack/PC/list ordering,
  CPU14/full-RAM differential, native heap/error replay, sanitizer checks.
- score: five evidence categories (identity, structure, differential,
  native replay, contract tests); no probabilistic score.
- verdict: likely_match, pending independent verification.
- review_notes: synthetic native frames, nonwrapping dependency returns,
  explicit allocator error stops. No classifier, acceptance, accounting,
  or credit changes are included in this author PR.

## Checker registration (2026-10-29)

This row's audit is now a registered checker: it carries `--compile-only`, which builds and binds the candidate library with the audit's own compiler command and no game media, and `CHECKER_ROWS`. Proven on current main with

```sh
python3 tools/overlay/check_checker_builds.py OVL_001C_0111 --require-current-main
```
