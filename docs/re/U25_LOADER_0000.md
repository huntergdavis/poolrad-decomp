# U25 +0000 complete operand loader

The [C loader](../../src/recovered/overlay_u25_loader_0000.c) implements U25
public entry 2 on the original shared CPU/RAM state. It composes the actual
+06AF operand-word helper and the complete
[+1153/+12FB string cores](U25_STRING_CORES.md), including their SYSTEM
routines. There are no unresolved hooks or result-only callees in this chain.
The [API](../../include/poolrad/recovered/overlay_u25_loader_0000.h) reuses
the string cores' fourteen-word CPU layout directly.

The older pr_rec_ecl_load_operands convenience API is unchanged. Its
separate bounded arrays and string metadata do not represent the complete
original CPU/RAM effects, particularly when operand tables overlap.

## Source identity

Owner-supplied START.EXE: 65,231 bytes, SHA-256
d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd.
Owner-supplied GAME.OVR: 206,105 bytes, SHA-256
872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614.

| Body, end exclusive | OVR file start | Bytes / instructions | SHA-256 |
| --- | --- | --- | --- |
| U25 [0000,0136), public entry 2 | 0x74D3 | 310 / 119 | 750610f1ea8c924651ee615595556377352cf441f3fd7e27a01c79ed3f6259cd |
| U25 [06AF,06D3), public entry 9 | 0x7B82 | 36 / 16 | 53565df24104754f01dff68b10cf0dda3fe72ad394a5253c611ceb594d93e862 |

Entry 2's original stub identity is 002B:002A. The complete loader has
three near-call sites, each preceded by PUSH CS: +00BC to +1153, +0110 to
+06AF, and +011E to +12FB. Each callee returns through RETF 4; the loader
returns through RETF 2. The string report records the other eight body
hashes and bounds. The audit verifies both public entry descriptors,
exact body hashes, all three near calls, and the string runtime fixups.

## Recovered behavior

The count is the low byte of SS:[entry SP+4]. The loader copies it into
SS:[BP-7], resets its local string count, and visits operand indices 1
through the copied count. Count zero skips the operand loop but still
increments DS:49ED on exit. Counts through 255 are accepted, without the
older model's 63-slot restriction.

For each operand, the loader separately reads script bytes at PC+1 and
PC+2 through the live DS:49DE far pointer and the original 6700h offset.
It writes the tag at DS:(7005+i), first value byte at DS:(7085+i), and
advances PC by two. The order is preserved: an earlier write can alias a
pointer, PC, stack local or subsequent read.

Tags 1, 2 and 3 consume a third byte into DS:(7045+i). Tag 80h increments
the local string index and either calls +1153 with the packed length, or
clears only that string slot's length byte for zero length. Tag 81h
consumes the third byte, calls +06AF to assemble the address, then calls
+12FB. Other tags consume only two bytes. High-byte cells that a path does
not write retain their previous contents; no host metadata reset is added.

After every operand, the live local index is compared with the live copied
count. Both can participate in shared-memory aliases. The final PC
increment occurs only when the original caller reaches +012C; host limit
or unexpected-return stops do not pretend that the routine completed.

The tag, high and low arrays are only 40h apart. Higher indices overlap
earlier cells, and string buffers can overlap these arrays. They remain
one RAM image. The C tests explicitly check tag index 129 overwriting low
index 1. Neither bounds checks on separate arrays nor cached decoded
operands reproduce these effects.

## Execution contract

The caller supplies exactly 1 MiB synchronous RAM distinct from CPU,
IP=0000, the real far-return/count frame, and the already-loaded SYSTEM
segment. Code and loaded bindings must remain immutable; data writes may
not overlap code. Entry SP in [022A,FFFA] reserves the deepest nested frame
and the original return/count frame without offset wrap. Effective offsets wrap
at 16 bits; word and LES bytes are physically consecutive with A20 off.

The two positive budgets have separate meanings:

- max_operands counts reached +0022 headers. OPERAND_LIMIT exposes the
  state before that instruction, after the original index increment when
  applicable.
- max_string_iterations resets for each call to +1153 or +12FB and follows
  the string-core loop headers. STRING_LIMIT exposes the callee's exact
  reached CPU/RAM, including its active frames.

TRANSFER exposes an unexpected callee return. CALLEE exposes a rejected
string entry at its reached state. These are explicit host boundaries,
not alternate DOS returns or claims that all aliased states terminate.
Invalid initial arguments reject atomically.

Logical/shift AF is zero by convention. Multi-bit shift OF is retained
until later instructions overwrite it. Native saved-route reachability,
undefined-flag policy, A20 crossings, wrapped-stack profiles, overlay
loading and asynchronous/self-modifying execution remain unverified.

## Validation and parity review

The [original-byte audit](../../tools/overlay/u25_loader_0000_audit.py)
passed **390 complete-chain cases**. All fourteen CPU words and every byte
of 1,048,576-byte RAM matched: 385 normal returns, two operand-budget stops,
two nested string-budget stops, and one corrupted-callee-return stop.

Coverage reached **119/119 loader instructions**, **16/16 +06AF
instructions**, **166/166 +1153 instructions**, and **229/229 +12FB
instructions**. The audit executes the actual six remaining dependencies,
without runtime stubs; their separate detailed coverage is recorded in
the string-core report.

The matrix includes all 256 tag bytes; counts 0, 2, 14, 63, 64, 65, 127,
128, 254 and 255; zero-length and mixed strings; every address class; PC
wrap; relocated code; low/high accepted stack limits; five shared
stack/table alias fixtures; and 100 seeded random operand sequences.
The [focused C test](../../tests/recovered/test_overlay_u25_loader_0000.c)
checks mixed outputs, stale high-byte preservation, zero/255 counts,
actual table overlap, both limit states and atomic rejection.
Both loader/string CTest targets passed, as did strict-warning compilation,
AddressSanitizer/UndefinedBehaviorSanitizer and the full recovered-library
build.

target_function: OVL_0025_0000.
source_candidates: pr_u25_load_operands_0000.
signals: hash/extent and descriptor identity; exact call graph; ordered
RAM/register/flag/frame effects; complete-state original-byte differential.
score: four evidence classes. verdict: match within the documented contract.
review_notes: the string APIs provide full-state dependencies, while their
budgets and loaded-code preconditions remain explicit. The old convenience
API is not promoted by association. No strict-credit or queue-accounting
change is made before independent review.

## Reuse and reproduction

The required deja query "ecl_load_operands" found Claude sessions
997d7fe9-a03 and 4d9851e1-f26. Their loader-bound and reused-string-metadata
findings informed the adversarial cases, without treating those host-state
models as full-state evidence. ecl_helpers.c supplied semantic comparison
points; the original bodies independently establish the effects above.
CPU helpers follow overlay_u25_string_cores.c, and the audit reuses its
hash checks and synthetic CPU utilities. Its prior parity is not treated
as execution of this loader composition.

~~~sh
python3 tools/overlay/u25_loader_0000_audit.py \
  --exe /private/START.EXE --ovr /private/GAME.OVR
cmake -S . -B /tmp/poolrad-loader-build -DBUILD_TESTING=ON
cmake --build /tmp/poolrad-loader-build --target poolrad_overlay_u25_loader_0000_tests
ctest --test-dir /tmp/poolrad-loader-build -R poolrad_overlay_u25_loader_0000_tests --output-on-failure
~~~

Original media, extracted disassembly and private logs remain ignored.
