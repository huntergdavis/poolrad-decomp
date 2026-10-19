# U77 +0901 party-row display composition

This adds the complete raw CPU caller, composed with existing clear-region,
decimal-byte conversion, printer, and SYSTEM string implementations. Independent
review owns acceptance and credit; no classification or accounting ledger changes.

## Identity and selection

The refreshed schema-8 successor to the requested schema-4 queue ranks this
body fourth, score 71, the highest remaining substantive composable candidate,
with six of six distinct direct callees ready. Selection used main `dba36361`
after PR #1072 was independently merged.

The queue's 434-byte/183-instruction bound includes 103 bytes of trailing data.
The executable body is **[0901,0A4C), 331 bytes, 129 instructions**, ending with
RETF 4 at 0A49. GAME.OVR file range [137531,137862); SHA256:
`5c88f5a5a192ccfaa0d5df8d6d3b610e129845c73c049c6249b32c2bf9ce3a87`.
Nine read-only string bytes at [08F8,0901) are loaded privately, never counted
as instructions. Trailing [0A4C,0AB3) data is excluded. Exported entry slot 3;
current classification `game_evidenced`, confidence `provisional` is unchanged.

Near caller: U77 +004A, PUSH CS/CALL site +02D3. Fixup-backed far callers:
U6C +056C at +1BD3, and U6C +2033 at +204C.

| Call sites | Composed target |
|---|---|
| 0933 | resident 03F1:0137 clear region |
| 0971, 09AF | resident 00BA:1037 byte-to-decimal string |
| 0976, 0A41 | resident 0709:0626 printer |
| 09D8, 0A28 | SYSTEM 0AF8:0B0B StrLoad |
| 09BD, 09F0, 0A0E | SYSTEM 0AF8:0B25 StrStore |
| 09E2, 0A32, 0A3C | SYSTEM 0AF8:0B98 StrCat |

All 13 call segment words have relocation fixups. Dependencies retain their
complete CPU, stack, RAM, bus, and port effects.

## Behavior and profile

Two word arguments supply a color selector byte and signed row/index byte.
The caller chooses color 10 or 13, clears columns 5 through 11 on row index+7,
reads a byte from the record at DS:5D92, and prints its decimal form. Values
below 10 shift the starting column from 5 to 6. For index zero, value 18, and
a positive extension byte at record+16, it also formats and prints the extension:
padding below 10, a special two-byte literal for 100, and surrounding strings
read live from CS. Stack buffers and record pointers are reread in source order.
Opcode 98 is CBW here despite Capstone's CWDE label in some versions.

The header defines the profile: exactly 1 MiB A20-off RAM, attached EGA, entry
IP0901, SP [0300,FFF4], ordinary-RAM initial DS/SS windows. Caller record reads
and every flat-RAM decimal/SYSTEM access must exclude physical A0000..AFFFF;
this is a precondition, not an all-reads dynamic check. Drawing uses the bus.
Code, bindings and literals are immutable outside the aperture; writes avoid
them; reached return frames do not wrap. Other aliases remain live. CPU, bus,
EGA, limits, diagnostics and RAM must be distinct.

Each clear, rectangle, printer and glyph invocation gets fresh budgets. There
is no root loop. Reached LCALL opcode/offset bytes are checked before pushing
the far return, with segment bindings read live. CODE preserves pre-call state;
CALLEE and TRANSFER preserve actual partial execution and nested diagnostics,
without rollback. ARGUMENT changes only optional diagnostics. Execution uses
normal/386 flags without IRQs, faults, self-modification or arbitrary-PC resume.
Port callbacks only observe/refuse OUT, never mutate state. Modes 0/2 ordinary
memory effects do not establish CGA/Tandy hardware closure.

## Measured author evidence

| Check | Measured result |
|---|---|
| Original-byte differential | 95 cases: 67 returns, 13 CODE, 15 CALLEE |
| Original state | CPU14 + 983,040 ordinary RAM bytes; 6,920 ordered OUT events and 14,553 raw VRAM write operands; zero flag masks |
| Caller coverage | 129/129 starts, 14/14 conditional outcomes |
| Registered closure | 1,068/1,094 starts, 101/122 conditional outcomes |
| DOSBox-X EGA normal/386 | 44 returning cases; CPU14 + 197,632 ordinary RAM bytes + 262,144 EGA plane bytes; 6,414 ordered OUT events; zero flag masks |
| ASan/UBSan semantics | 4,241 checks passed |
| Focused CMake/CTest | 1/1 passed |

Compiled together: 15 C sources, 14 unchanged dependencies.
Native RESULT.BIN SHA256:
ada9f66a8446aa39f86d85b99e6a13a39da1d08133d84727669b69dda694760b

Tests ran on dba36361b396df172583635ae26d2c10a9ab4a57. Integration base
76a1cda4d95d312a0b39b50c8dc4d5b76ef2dc42 changes only the queue documentation
and accounting checkpoint; all compiled and imported dependencies are identical.
The private receipt checks both bases and reproduces final native INPUT.BIN.

Original fixtures exercise formatting/color branches, byte and signed-row
boundaries, modes 0/1/2/3, absent font, stack bounds, SS/DS aliasing, A20 stack
addressing, every root call-site rejection, nested rejection, bad decimal table,
exhausted budgets and refused OUTs. No original instruction is replaced by a C
callback or fabricated result. Semantic tests independently check every byte,
all positive extension values, synthetic CS literals, row gating, EGA colors
and neighbors on both pages/all four planes, partial stops, and invalid entry.

Native plain execution supplies CPU/RAM/plane results; a separate guest INT1
run captures ordered OUT operands. Exact event-list equality checks the fixed
trace capacity. Native evidence covers synthetic returning cases only, not
native error/limit/refusal stops or natural gameplay reachability. Separate
hardware latch/register equality is not claimed. Original comparison checks
raw VRAM write operands; native comparison checks actual plane effects.

Full caller coverage is not exhaustive dependency coverage. Registered gaps
include signed LongInt formatting, string truncation/overflow, UpCase branches,
and drawing alternatives not reached by these fixtures. The private JSON lists
every uncovered start and outcome. TRANSFER behavior is implemented but no
directed altered-return case is claimed in this batch.

## Reproduction and provenance

Original media, extracted code/data, generated guests and receipts remain under
ignored resources/. With local Capstone and Unicorn dependencies installed:

```sh
export PYTHONPATH=resources/pythondeps:tools/overlay
export PYTHONDONTWRITEBYTECODE=1
python3 tools/overlay/u77_party_0901_cpu_audit.py --unicorn --output resources/original.json
python3 tools/overlay/u77_party_0901_cpu_audit.py --native --output resources/native.json
cmake -S . -B resources/build -DBUILD_TESTING=ON
cmake --build resources/build --target poolrad_u77_party_0901_cpu_tests -j4
ctest --test-dir resources/build -R '^poolrad_u77_party_0901_cpu_tests$' --output-on-failure
```

Sanitizer build: the audit's SOURCES plus the focused C test, with C11,
Wall/Wextra/Wpedantic/Werror, debug symbols, address/undefined sanitizers,
frame pointers, include path `include`, and ASAN_OPTIONS=detect_leaks=1.
The private receipt pins compiled sources/headers, local Python imports, tools,
media, reports, native artifacts and selection inputs; VP handoff gives its hash.

Pre-implementation `deja "U77 0901"` returned no matches. No recalled code or
ownership claim was reused. Current inbox/remote checks found no conflicting
assignment. A later ctypes recall stalled; health/repair attempts timed out,
and no result from that recall was used.

Composition primitives reuse merged U1C +2E7D. Audit/native mechanics reuse
merged clear +0137, printer +0626, glyph +01DF, SYSTEM Str/resident +1037, and
U1C +2E7D harnesses. The final entry point shares its module identity with
native fixtures so ctypes types agree. An early bad-table fixture was corrected
to flip a bit: zeroing an already-zero byte was not a rejection test.

## Author parity assessment

- target_function: complete OVL_0077_0901 [0901,0A4C).
- source_candidates: pr_u77_party_0901_cpu and unchanged raw dependencies.
- signals: pinned identity, full caller control-flow coverage, CPU/RAM and
  partial-stop equality, independent native CPU/RAM/planes/OUT comparison.
- score: strong bounded-profile author evidence, not independent acceptance.
- verdict: likely_match within the documented profile.
- review_notes: retain provisional classification, exact code/data bounds,
  closure gaps, ordinary-RAM restrictions and synthetic-only native evidence.
  No merge or self-credit is included.
