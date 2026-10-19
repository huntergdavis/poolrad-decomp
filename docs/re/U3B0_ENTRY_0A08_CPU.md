# U3B0 +0A08 conditional-color selected-string display composition

This adds the complete raw CPU caller and composes the existing list selector,
leading/trailing-space scans, SYSTEM Copy, and resident printer. Acceptance and
credit remain with the independent reviewer; no accounting ledger is edited.

## Identity and selection

On tested/selection base `93c7a3add6ac8d4a55800325926c823cf843f7d9`, the refreshed schema-8
successor to the requested schema-4 queue ranks this body 24th, score 65:
the highest remaining substantive composable target, with five of five distinct
direct callees ready. Existing classification is `game_evidenced`, confidence
`provisional`; this change does not promote it.

Complete code **[0A08,0B03), 251 bytes, 107 instructions**, RETF 4 at 0B00.
GAME.OVR file range [185047,185298); SHA256:
`1b6c5b1ac9f26c64572151759cc73a44003c4321870f36d907e4f3d7d32bbf86`.
No exported entry slot. The decoded near caller is U3B0 +0D9A, with aligned
PUSH CS/CALL at +0FEB.

| Call site | Target | Convention |
|---|---|---|
| 0A1E | U3B0 +0000 list selector | PUSH CS; near CALL; callee RETF 6 |
| 0A30 | U3B0 +08D7 leading-space scan | PUSH CS; near CALL; callee RETF 6 |
| 0A45 | U3B0 +0923 trailing-space scan | PUSH CS; near CALL; callee RETF 6 |
| 0AA1 / 0AF3 | SYSTEM 0AF8:0B57 Copy | fixup-backed far CALL |
| 0AA6 / 0AF8 | resident 0709:0626 printer | fixup-backed far CALL |

## Behavior and supported profile

SS:[entry SP+4] is a context offset in SS; the next word is the list index.
The selector uses the context's far head at +14/+16. The two scans operate on
a local copy capped at 40 bytes. Their returned bounds determine a byte-sized
substring count; Copy reads the original selected Pascal string into the
caller's local buffer. After the scans, selected node byte +29 chooses the
color: zero uses context[22], and every nonzero value uses context[24]. The
printer receives that buffer and the chosen live context color and coordinates:
x is signed context[1E]+leading bound-1; y is signed
context[1C]+index-DS:[6B29]. All word arithmetic wraps at 16 bits.

The model preserves the original capped-scan behavior: all spaces do not imply
an empty string. In particular, the byte count can wrap after subtracting the
bounds, and Copy then clips against the original string length. Long strings
can contain the node flag at +29 and list-link words at +2A/+2C; fixtures read
those actual bytes.
A selected null far pointer is the ordinary address 0000:0000, not a host null
string. Source, context and local operands are reread in original order.

The header defines the profile: exactly 1 MiB A20-off RAM, attached EGA,
initial IP0A08, SP [0300,FFF4], ordinary-RAM DS/SS windows. Caller, selector,
scan and flat-RAM SYSTEM accesses must exclude physical A0000..AFFFF. This is
a precondition, not an all-reads dynamic audit. Printer/glyph accesses use the
bus. Code and bindings are immutable outside that aperture, writes avoid code,
and reached return frames do not wrap. Other aliases remain live. CPU, bus,
EGA, limits, diagnostics and RAM storage are distinct.

Printer and glyph invocations get fresh budgets. The selector/scans retain
their existing contracts; no new cap is invented. Near/far CALL opcode and
offset validation occurs before CALL; PUSH CS has already executed at a near
CODE stop. Far segment words are read live. CALLEE and TRANSFER preserve actual
partial execution, with no rollback; diagnostics retain nested printer/glyph
status and the latest scan's store status. ARGUMENT changes only diagnostics.
Normal/386 flags, synchronous execution, no IRQs/faults/self-modification or
arbitrary-PC resumption. Callbacks only observe/refuse OUT. Modes 0/2 ordinary
RAM effects do not establish CGA/Tandy hardware closure.

Integration base `400cc95a03248f7f546957a5ffe65edb7a1e75ea` adds the
independent acceptance of #1076. Compiled and imported dependencies are
byte-identical to the tested base; read-only accounting metadata is pinned
at the integration base.

## Measured author evidence

| Check | Result |
|---|---|
| Original-byte differential | 202 cases: 172 returns, 10 CODE, 20 CALLEE |
| Original state | CPU14 + 983,040 ordinary RAM bytes, 13,618 ordered OUT events, 25,624 raw VRAM write operands; zero flag masks |
| Caller coverage | 107/107 starts, 2/2 conditional outcomes |
| Registered dependency closure | 705/712 starts, 82/90 conditional outcomes |
| DOSBox-X EGA normal/386 | 100 returning cases: CPU14 + 197,632 RAM bytes + 262,144 plane bytes, 10,880 ordered OUT events; zero masks |
| ASan/UBSan semantics | 190,899 checks passed |
| Focused CMake/CTest | 1/1 passed |

Ten C sources compile together, including nine unchanged implementations.

The original cases include both color branches, empty/all-space/long strings,
cap boundaries, list selection and exhaustion, null head, a cyclic list with
finite index, source-offset wrap, signed coordinate extremes, modes 0/1/2/3,
absent font, SS/DS aliasing, A20 stack addressing, stack bounds, every caller
call-site rejection, nested call rejection, printer/glyph limits, and refused
OUTs. Unreached far-call corruption returns normally in the other color arm.
Semantic checks independently cover all 256 string lengths in four styles
and both arms, selected pointers, exact substring bytes, all 256 flag values,
actual EGA colors and unchanged neighbors on both pages/all four planes,
near-call stop stack state, and atomic invalid arguments.

Native plain execution provides CPU/RAM/plane results. A separate guest INT1
run captures ordered OUT operands; exact event equality checks the trace's
2,048-event capacity. The stack context is loaded on a separate scratch stack
before entering the original caller. Final native INPUT.BIN is reproduced by
the receipt generator. Each native fixture is exercised with flag 0 and 255
and different context colors. RESULT.BIN SHA256:
`cea3165f0029bc09a58640733be9a0f0eb3d0cf1fb6c7eeea17439e59dfd42ff`.

Native cases are synthetic, returning, and select valid nodes; they do not
establish native null/exhausted-list, error/limit/refusal, or gameplay reachability.
Hardware latch/register equality is not separately claimed. Original comparison
checks raw VRAM operands; native comparison checks actual planes. No directed
altered-return case is claimed for implemented TRANSFER handling. Remaining
closure gaps include Copy's negative index/count branches, an UpCase outcome,
and glyph alternatives; complete locations are in the private report. Full
caller coverage is not exhaustive dependency coverage.

## Reproduction and provenance

Original media, extracted code, generated guests, captures and receipts remain
under ignored resources/. With local Capstone/Unicorn dependencies:

```sh
export PYTHONPATH=resources/pythondeps:tools/overlay
export PYTHONDONTWRITEBYTECODE=1
python3 tools/overlay/u3b0_entry_0a08_cpu_audit.py --unicorn --output resources/original.json
python3 tools/overlay/u3b0_entry_0a08_cpu_audit.py --native --output resources/native.json
cmake -S . -B resources/build -DBUILD_TESTING=ON
cmake --build resources/build --target poolrad_u3b0_entry_0a08_cpu_tests -j4
ctest --test-dir resources/build -R '^poolrad_u3b0_entry_0a08_cpu_tests$' --output-on-failure
```

Sanitizers compile the audit's SOURCES and focused C test with C11,
Wall/Wextra/Wpedantic/Werror, debug symbols, address/undefined sanitizers,
frame pointers and include path `include`; run with ASAN_OPTIONS=detect_leaks=1.
The private receipt pins sources/headers, local Python imports, tools, media,
reports, native artifacts and queue inputs. VP handoff supplies its path/hash.

Pre-implementation `deja "U3B0 0A08"` timed out; bounded deja-health and
deja-health --repair also timed out. No recalled result was reused. Current
inbox/remote checks found no conflicting assignment. The implementation and
audit/native framework adapt merged [PR #1076](https://github.com/huntergdavis/poolrad-decomp/pull/1076),
whose [evidence](U3B0_ENTRY_0969_CPU.md) describes the same raw dependencies
and native stack-window loading. This caller adds the node-flag branch,
two distinct live context colors, and validation at each arm's own call sites.
Original-byte source extraction independently checks the new body's identity
and all seven call sites. No guessed callee callback supplies game behavior.

## Author parity assessment

- target_function: complete OVL_03B0_0A08 [0A08,0B03).
- source_candidates: pr_u3b0_entry_0a08_cpu and unchanged raw dependencies.
- signals: pinned identity, complete caller execution, CPU/RAM partial-stop
  equality, independent native CPU/RAM/planes/OUT agreement.
- score: strong bounded-profile author evidence, not independent acceptance.
- verdict: likely_match within the documented profile.
- review_notes: retain provisional classification, ordinary-RAM restrictions,
  closure gaps and synthetic-only native scope. No merge or self-credit.
