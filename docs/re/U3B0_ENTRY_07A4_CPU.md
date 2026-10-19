# U3B0 +07A4 list-viewport display composition

This adds the complete raw CPU caller: clear a context rectangle, select a
starting node, preserve the flagged-node helper's result, print successive
list rows with per-node colors, and fill short rows with black space glyphs.
It composes existing raw implementations throughout; no behavioral callback
supplies a missing game callee. Acceptance and credit belong to the independent
reviewer. No accounting ledger is changed.

## Identity and selection

On `9c5788cf06cfbbe71336ca1ccfdb04041f29b860`, the refreshed schema-8 successor to the requested schema-4
queue ranks this body 31st, score 62: the highest remaining substantive
composable target, five of five direct targets ready. Classification remains
game_evidenced, confidence provisional. It has no exported entry slot.
Near callers are U3B0 +0C33 and +0D9A, at aligned PUSH CS/CALL sites +0CBC
and +0EE4 respectively.

Complete **[07A4,08D7), 307 bytes / 115 instructions**, RETF 4 at +08D4.
GAME.OVR file range [184435,184742); SHA256:
b4d0d09d692e50b31125fa64fbedb3316385f9dd60dc1efb5c74605b2e669eef.

| Call site | Target | Convention |
|---|---|---|
| 07CA | resident 03F1:0137 clear | fixup-backed far CALL |
| 07DE | U3B0 +0000 selector | PUSH CS; near CALL; RETF 6 |
| 07F9 | U3B0 +0711 flagged-node traversal | PUSH CS; near CALL; RETF 8 |
| 0849 / 0869 | resident 0709:0626 printer | fixup-backed far CALL |
| 08B5 | resident 0709:01DF padding glyph | fixup-backed far CALL |

## Behavior and supported profile

SS:[entry SP+4] is a context offset within SS; [SP+6] is the starting index.
The caller clears the context rectangle from bytes +18/+1A/+1C/+1E, then
selects from the far head at +14/+16. It calls +0711 with the same head and
index, plus the context word. Its returned DX:AX pointer is saved at BP-8/-6
even though the remainder of this caller does not use it. Omitting this call
would lose its CPU and stack effects.

The row byte begins at context[1C]. Each iteration exits on a null selected
pointer or when the row is greater than context[18] in signed-byte order.
Node byte +29 selects context color +22 when zero or +24 when nonzero.
The full node Pascal string is printed at context[1E], row. If its live length
byte is less than context[-54], a direct glyph call pads the difference with
character 20h, transform 1, color zero, starting at signed context[1E]+length.
The caller follows separate link words +2A/+2C, increments the row byte with
wrap, and jumps back. Long synthetic strings can contain the flag and link
fields; their actual bytes remain live.

row_loopbacks counts unconditional JMP +08CE, including a jump after a row
whose next pointer is null. Zero permits the first row's effects and stops
before its jump, after the link and row updates. A finite bound is necessary
for cyclic lists whose signed row byte wraps while bottom is 127. Other
budgets refresh per dependency invocation: clear outer loops, each rectangle,
each printer, and each glyph. Selector and flagged traversal retain their
existing finite-index/signed-bound contracts.

The header defines the profile: exactly 1 MiB A20-off RAM, attached EGA,
entry IP07A4, initial SP [0300,FFF4]. Initial DS/SS windows and all caller or
flat-RAM callee accesses exclude A0000..AFFFF. This is a precondition, not an
all-reads dynamic proof. Drawing implementations use the bus; code and bindings
are immutable outside the aperture, writes avoid code, and reached return
frames do not wrap. Other aliases are read live. CPU, bus, EGA, limits,
diagnostics and RAM are distinct storage objects. Normal/386 flags,
synchronous execution; no IRQs, faults, self-modification, or arbitrary-PC
resumption. Callbacks only observe or refuse OUT, without mutating state.

Reached root near/far opcodes and offsets are checked before CALL, with live
far segment words. PUSH CS has already executed at a near CODE stop. Inherited
callee validation remains as implemented; rectangle near helpers do not add
new opcode guards. CALLEE, TRANSFER and LIMIT retain partial CPU/RAM/hardware
state with no rollback. Diagnostics preserve the latest clear/rectangle,
printer/glyph, and direct glyph statuses. ARGUMENT preserves CPU/RAM/EGA and
resets optional diagnostics. Modes 0/2 ordinary-memory writes do not establish
CGA/Tandy hardware closure.

Integration base b25c8ed36323ece7c1c0ced8c94f5ccf56b6fb05 adds independent acceptance
of #1078. Compiled and imported dependencies are byte-identical to the tested
base; read-only accounting metadata is pinned at the integration base.

## Measured author evidence

| Check | Result |
|---|---|
| Original-byte differential | 105 cases: 66 returns, 10 CODE, 26 CALLEE, 3 row LIMIT |
| Original state | CPU14 + 983,040 ordinary RAM bytes, 28,648 ordered OUT events, 70,954 raw VRAM write operands; zero masks |
| Caller coverage | 115/115 starts, 8/8 conditional outcomes |
| Registered dependency closure | 1026/1035 starts, 98/108 conditional outcomes |
| DOSBox-X EGA normal/386 | 52 returning cases: CPU14 + 197,632 RAM bytes + 262,144 plane bytes, 29,410 ordered OUT events; zero masks |
| ASan/UBSan semantics | 6,606 checks passed |
| Focused CMake/CTest | 1/1 passed |

Fourteen C sources compile together, including thirteen existing implementations.
Original cases exercise both color paths, padding/no padding, empty/long
strings, list exhaustion and null head, finite cyclic traversal and row-wrap
LIMIT, signed index bounds, viewport boundaries, all video modes, active pages,
flags, stack bounds, A20 stack addressing, SS/DS aliasing, source-offset wrap,
call-site rejection, nested limits and refused ports.

Semantic checks independently cover all 256 string lengths, both flag states,
all 256 padding widths, selected/flagged pointers, row counts, exact padding
arguments at a pre-call stop, partial row-loop updates, nested clear stops,
and atomic invalid arguments. EGA checks verify text colors, black padding,
and unchanged neighboring bytes on both pages and all four planes.

Native plain execution captures CPU/RAM/planes. A separate guest INT1 run
records ordered video OUT operands; exact trace equality checks the finite
2,048-event trace capacity. The stack context is loaded on a separate scratch
stack before entering the original code. Native INPUT.BIN is reproduced from
the final fixtures by the receipt generator. RESULT.BIN SHA256:
4a3526840de2240624d5f9b85906dc0e7c5c706d64c0aec303dd46cf3d6ff737.

Native cases use synthetic returning frames with valid nodes, including a
finite cyclic viewport. They do not establish native null/exhausted-head,
partial-stop, row-wrap LIMIT, or gameplay reachability. Hardware latch/register
equality is not separately claimed. Original comparison checks raw VRAM write
operands; native comparison checks actual planes. No directed altered-return
case is claimed for implemented TRANSFER handling. Full caller coverage does
not mean exhaustive dependency coverage; exact missing starts/outcomes remain
in the private report. Original closure gaps include +0711 flagged-node
stores, clear/rectangle alternatives, StrStore, UpCase and glyph branches.

## Reproduction and provenance

Original game media, extracted code, generated guests, captures and receipts
stay under ignored resources/. With local Capstone/Unicorn dependencies:

    export PYTHONPATH=resources/pythondeps:tools/overlay
    export PYTHONDONTWRITEBYTECODE=1
    python3 tools/overlay/u3b0_entry_07a4_cpu_audit.py --unicorn --output resources/original.json
    python3 tools/overlay/u3b0_entry_07a4_cpu_audit.py --native --output resources/native.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_u3b0_entry_07a4_cpu_tests -j4
    ctest --test-dir resources/build -R '^poolrad_u3b0_entry_07a4_cpu_tests$' --output-on-failure

Sanitizers compile the audit's SOURCES and focused C test with C11,
Wall/Wextra/Wpedantic/Werror, debug symbols, address/undefined sanitizers,
frame pointers and include path include; run with ASAN_OPTIONS=detect_leaks=1.
The private receipt pins source/header and local Python import closures,
reports, queue inputs, tool/media hashes and native artifacts. VP handoff
provides its path and SHA256.

Pre-implementation shared memory context succeeded. The query
U3B0 07A4 prefix ownership 0711 returned session
01a1386e-bea7-7883-8ddd-06fd2be8db9b, identifying the +0711 dependency,
and historical ownership screens. These were navigation evidence; current
contracts, original bytes, queue, inbox and remote checks establish this work's
scope. The private memory-context.txt retains the result. No historical
ownership message is treated as a current exclusive claim.

The caller/audit/native framework adapts merged
[PR #1078](https://github.com/huntergdavis/poolrad-decomp/pull/1078), with
[its predecessor evidence](U3B0_ENTRY_0A08_CPU.md). The original clear/rectangle,
printer/glyph, selector and +0711 audits supply independently hash-checked
bodies and established raw interfaces. This change adds complete clear/print/
padding composition, live traversal, row budgets and nested stop propagation.

## Author parity assessment

- target_function: complete OVL_03B0_07A4 [07A4,08D7).
- source_candidates: pr_u3b0_entry_07a4_cpu and unchanged raw dependencies.
- signals: pinned original identity, caller execution coverage, CPU/RAM and
  partial-stop equality, independent native CPU/RAM/planes/OUT comparison.
- score: strong documented-profile author evidence, not independent acceptance.
- verdict: likely_match within the documented profile.
- review_notes: retain provisional identity, ordinary-RAM restrictions,
  closure gaps, synthetic native scope and independent acceptance/credit.
