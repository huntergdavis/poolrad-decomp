# U1C +0B52: complete IF-family chain

The six ECL IF opcodes now execute through a full CPU/RAM translation of
U1C +0B52, composing the complete U25 +1EA6 skip routine, operand loader and
string callees. A true comparison advances past the IF opcode and returns;
a false comparison skips the following statement with all original side
effects. No callback or count-only substitute stands in for that chain.

Source: [overlay_u1c_if_0b52.c](../../src/recovered/overlay_u1c_if_0b52.c).
API: [overlay_u1c_if_0b52.h](../../include/poolrad/recovered/overlay_u1c_if_0b52.h).
Dependency: [U25 +1EA6](U25_SKIP_1EA6.md), PR #864, over #863/#862.
Independent strict-credit review and native route validation remain open.

## Original identity and extent

| Evidence | Value |
| --- | --- |
| START.EXE SHA-256 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR SHA-256 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| Caller | unit 001C, [0B52,0BC8); GAME file [1898,190E) |
| Callable extent | 118 bytes, 41 instructions, final RETF at 0BC7 |
| Body SHA-256 | 11830c28e2ca43d40e80db72b6374a46e73b4e0cc0522ba9728e7026e3718061 |
| Far calls | 0B67,0B79,0B8B,0B9D,0BAF,0BC1 to stub 002B:008E |
| Binding | U25 public entry 22 resolves to +1EA6; all six segment operands have fixups |

The generated overlay ledger bounds this function by the next prologue at
0BD1, reporting 127 bounded bytes, 126 decoded bytes and 45 instructions.
Nine trailing bytes after RETF are data and are excluded from this callable
body. The older [function ledger](FUNCTION_LEDGER.md) already records the
correct 118-byte/41-instruction extent. This discrepancy was reported to the
queue owner; this PR does not change accounting CSVs.

Required recall queries 'deja "0B52"' and 'deja "IF family"' both timed out
at 15 seconds without session results. No indexed-session finding was reused.
The existing path-complete model in ecl_helpers.c and the
[ECL contracts](ECL_CONTRACTS.md) supplied the prior opcode/flag interpretation;
the original bytes independently establish all branches, calls and effects.
DS4 confirmed this target's ownership in inbox i17176. The refreshed schema-3
snapshot placed it at rank 63, score 54, game_verified. Higher-ranked U1C
candidates remained reserved or transitively blocked; this target's sole
callee is now fully composed through PR #864. Queue transitive-readiness
corrections are handled separately by DS4.

## Behavior and API contract

Entry IP is 0B52. SS:SP holds return IP/CS, with no arguments. After pushing
BP, the routine increments DS:49ED **before** reading the latch at DS:6F7F.
It compares the latch against 16..1B and reads the corresponding comparison
byte. Zero calls the skip routine; any nonzero byte returns directly. Latch
values outside this range also return directly after the PC increment.

| Opcode (hex) | Predicate | Comparison byte |
| --- | --- | --- |
| 16 | equal | DS:6F78 |
| 17 | not equal | DS:6F79 |
| 18 | less | DS:6F7A |
| 19 | greater | DS:6F7B |
| 1A | less or equal | DS:6F7C |
| 1B | greater or equal | DS:6F7D |

All compares retain their original flag effects. The callee gets the real
far-call CS/IP frame. The loaded overlay stub performs a far JMP to U25
+1EA6 with no further register, flag or stack effects. The C API accepts the
already-loaded U25 and SYSTEM body segments, composes that jump directly,
and checks the callee's return CS/IP before executing the continuation.
The original audit executes the loaded stub instruction itself.

RAM is exactly 1 MiB with A20-off wrapping. CPU/RAM must be distinct and
synchronous; code, loaded bindings and SYSTEM code must stay immutable, and
data writes must not overlap them. Effective offsets wrap16; word bytes
remain physically consecutive modulo 1 MiB. Initial SP [0238,FFFC] reserves
the nested stack: the U25 entry SP is six bytes below the IF entry, satisfying
its 0232 lower bound. The final RETF may wrap SP to zero at the upper bound.

Positive budgets are forwarded unchanged into the skip/loader/string chain.
OPERAND_LIMIT and STRING_LIMIT expose the actual reached nested state;
TRANSFER exposes an unexpected nested return, and CALLEE a rejected nested
entry. Earlier RAM effects survive these stops. No successful return or
termination is manufactured. Invalid initial arguments reject atomically.
Undefined flags follow the composed dependency's published convention.

## Verification

The [audit](../../tools/overlay/u1c_if_0b52_audit.py) rechecks the caller extent,
six exact fixup-backed far calls, U25 entry mapping and every composed body
hash. Only linker segment words are relocated. A loaded far-JMP stub leads
to the actual original U25 body; all loader/string/SYSTEM instructions run.
Compiled C is compared against all 14 original CPU words and all 1 MiB RAM.

All 919 cases match: 914 returns, two operand-limit stops, two string-limit
stops and one altered nested return. Cases cover all 256 incoming latches,
all 256 comparison-byte values, all six IF branches with zero/nonzero/high
comparison bytes, all 256 following opcodes, every address-string class,
packed/empty strings, PC wrap, relocation, stack limits, seven stack/data
aliases, and 100 seeded mixed streams.

| Body | Instruction coverage |
| --- | ---: |
| U1C +0B52 | 41/41 |
| U25 +1EA6 | 133/133 |
| U25 +0000 | 118/119 |
| U25 +06AF | 16/16 |
| U25 +1153 | 166/166 |
| U25 +12FB | 229/229 |

Loader +0017 is its zero-count jump. The skip caller only supplies positive
counts; the independent loader audit covers that instruction. The coverage
gate requires exactly this exclusion. Four focused CTests, strict warnings,
ASan/UBSan and the full recovered-library build pass. Owner media and
extracted disassembly stay in ignored resources.

Reproduce using installed Capstone and Unicorn:

~~~sh
PYTHONPATH=/tmp/poolrad-capstone:/tmp/poolrad-unicorn python3 tools/overlay/u1c_if_0b52_audit.py --exe resources/DOS/POOLRAD/START.EXE --ovr resources/DOS/POOLRAD/GAME.OVR
cmake -S . -B /tmp/poolrad-if-build
cmake --build /tmp/poolrad-if-build --target poolrad_overlay_u1c_if_0b52_tests
ctest --test-dir /tmp/poolrad-if-build -R poolrad_overlay_u1c_if_0b52_tests --output-on-failure
~~~

## Adversarial parity review

- **target_function:** OVL_001C_0B52, complete 118-byte IF-family body.
- **source_candidates:** pr_u1c_if_0b52 with the complete U25 skip chain.
- **signals:** pinned body/RETF extent; six fixup-backed calls; exact predicate
  bytes and compare order; ordered PC/latch and shared-RAM effects; original
  execution versus compiled C across all CPU words and RAM bytes.
- **score:** 5/5 listed evidence categories agree, not a state-space proof.
- **verdict:** match within the published machine and loaded-binding contract.
- **review_notes:** treating comparison bytes as signed booleans would reject
  high nonzero values incorrectly. Moving the PC update before the prologue stack write can change aliased
  RAM effects; intermediate reads must retain the original ordering.
  A count-only skip drops string, stack and flag effects. Trailing data must
  not inflate the instruction count. Unexpected returns and nested limits
  remain visible. Immutable code/bindings, initial stack bounds, dependency
  undefined-flag conventions, and unmeasured native route remain limits.
