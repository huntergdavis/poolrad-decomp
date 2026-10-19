# U1C +2E7D text/glyph composition

Author evidence for the complete `OVL_001C_2E7D` body, composed with the existing
SYSTEM FillChar, U25 operand/string/load-word routines, and resident EGA glyph
implementation. This adds a raw CPU implementation; it does not alter acceptance,
classification, or credit ledgers.

## Identity and selection

- Tested integration base: `25914cb7f6dd3f870b8b78a0e74dd6cedf960fb9`.
- Refreshed queue rank 1, score 80, five of five direct callees ready. The current
  generator emits schema 8, the successor to the requested schema-4 queue.
- Complete `[2E7D,2F2D)`: 176 bytes, 64 instructions, final RETF at `2F2C`.
- GAME.OVR file range `[15299,15475)`; body SHA256:
  `4c04ef11f1280fba187b51494683c9bda992c5ce66e4c57aa003fd2796d59cb0`.
- No exported entry slot. An internal PUSH CS/CALL at U1C `3162` reaches it from
  `2F2D`. Existing classification is `game_verified`; this is not a new identity
  claim. The selected base had neither this raw implementation nor a strict-map row.

| Call site | Target |
|---|---|
| 2E8F | SYSTEM 0AF8:16A8 FillChar |
| 2E9C | 002B:002A stub -> U25 +0000 operand loader |
| 2EA9 | 002B:004D stub -> U25 +06AF word assembly |
| 2EE9 | 002B:0057 stub -> U25 +0E0C load-word |
| 2F0D | resident 0709:01DF glyph |

Each relocation is the segment word at call-site +3. The audit pins each original
body, decodes the complete caller, and installs only relocation bindings; it does
not replace executed original instructions with the candidate implementation.

## Behavior and supported profile

The routine clears DS:84C8/84C9 and DS:84E0, loads one operand, and assembles slot 1
into a base address. If DS:84DE is nonzero and y exceeds 13, it resets x/y to 2/5
and sets DS:84E2. It sets DS:84DF, reads successive addresses through U25 +0E0C,
and increments each returned word. Low-byte results 0 and 1 terminate; other
results draw one raw glyph at the current x/y, color 15, count 1, transform 0.
The routine increments x and its byte index after a glyph, and y on exit.
Addresses wrap at 16 bits, indices at 8 bits. Stack locals and cursor globals
remain live under supported aliasing, including aliases that alter return words.

The public header specifies the execution profile: exact 1 MiB A20-off RAM,
attached EGA, entry IP 2E7D, initial SP in [023A,FFFC], and initial DS/SS windows
outside physical A0000..AFFFF. Every access made by the flat-RAM SYSTEM/U25
callees must also remain outside that aperture; this is a caller precondition,
not a claim that every indirect read is dynamically validated. Glyph accesses
use the bus. Code and bindings are immutable and outside the aperture, reached
return frames do not wrap, and execution is synchronous without IRQs, faults,
self-modification, or arbitrary-PC resumption. CPU, bus, EGA, limits, diagnostic,
and RAM storage must be distinct. Port callbacks only observe or refuse OUT.

Positive operand/string budgets are inherited by the loader. The outer budget
counts taken 2F23 -> 2EDB edges and stops before the branch; zero permits a first
glyph. Glyph budget resets on each invocation. Load-word retains its existing
acyclic-party-list precondition. Reached LCALL/stub opcodes and target offsets
are checked before the CALL; segment bindings are read live. Loaded SYSTEM
bindings must agree with the supplied `system_cs`, as required by the existing
loader. CODE, CALLEE, TRANSFER, and LIMIT preserve real partial state, with no
rollback. Diagnostic fields retain nested glyph status. ARGUMENT preserves CPU,
RAM, and EGA, while resetting an optional diagnostic.

## Measured author validation

| Check | Result and comparison scope |
|---|---|
| Original-byte Unicorn differential | 177 cases: 154 returns, 13 CODE, 6 CALLEE, 1 TRANSFER, 3 outer LIMIT |
| Original state | 14 CPU words, 983,040 ordinary RAM bytes, zero flag masks; 5,075 ordered OUT events and 9,548 raw VRAM write operands |
| Caller coverage | 64/64 starts, 8/8 conditional outcomes |
| Registered closure coverage | 819/1,504 starts, 81/264 conditional outcomes; remaining locations listed in the private JSON |
| DOSBox-X native replay | 48 returning cases, machine EGA/mode 0Dh, normal core/386 CPU |
| Native state | 14 CPU words, 197,632 ordinary RAM bytes, 262,144 EGA plane bytes, zero flag masks; 3,672 ordered OUT events |
| Semantic ASan/UBSan test | 23,076 checks passed |
| Focused CMake/CTest | 1/1 passed |

The six CALLEE stops comprise two string-budget, two glyph-budget, and two
refused-OUT cases. Original cases include byte-index wrap, high-byte text values,
mode differences, page/font boundaries, aliasing at DS:84C8/84DE/5E26/5E2C, and
altered far returns. Semantic tests cover all 256 low-byte values, expected glyph
pixels on all four planes, unchanged neighbors, cursor reset, partial stops,
and atomic invalid entry. Nine C sources compile together; eight are unchanged
dependencies at the tested base.

Native plain execution supplies CPU/RAM/plane results; a separate guest INT1
replay records OUT order and values. The final fixture generator reproduces the
saved INPUT.BIN exactly. RESULT.BIN SHA256 is
`9da383dc35335d614bc291b16b3be5d52c7d273006a473443beb23c68e28ed31`.
Native evidence covers returning cases only, not the original harness's error,
budget, refusal, and alias stops. Hardware latch/register bytes are not compared
separately. The original harness compares raw VRAM write operands; the native
replay separately checks the actual four-plane effects.

Full caller coverage is not exhaustive callee coverage. This composition reuses
existing audited callees; many U25 alternatives are unexercised here, and the
registered glyph UpCase path is unreachable with transform 0. Natural gameplay
reachability and arbitrary interrupt behavior remain unproven.

During harness development, adding even a no-op Unicorn memory-read hook with
shared mapped RAM altered the observed FillChar RETF destination. The final
harness uses the established write-only memory observer; no original instruction
was patched to compensate. Private trace-return/trace-shared reproductions and
logs preserve that observer limitation. The independent native replay agrees
with the final candidate and original run. This observer does not establish an
all-reads aperture audit; the flat-RAM precondition above remains explicit.

## Reproduction and provenance

All original media, generated original-code blobs, native captures, reports, and
receipt remain under ignored `resources/`. With local Capstone/Unicorn installed:

```sh
export PYTHONPATH=resources/pythondeps:tools/overlay
export PYTHONDONTWRITEBYTECODE=1
python3 tools/overlay/u1c_text_2e7d_cpu_audit.py --unicorn --output resources/original.json
python3 tools/overlay/u1c_text_2e7d_cpu_audit.py --native --output resources/native.json
cmake -S . -B resources/build -DBUILD_TESTING=ON
cmake --build resources/build --target poolrad_u1c_text_2e7d_cpu_tests -j4
ctest --test-dir resources/build -R '^poolrad_u1c_text_2e7d_cpu_tests$' --output-on-failure
```

The semantic sanitizer build compiles the audit's nine SOURCES with the focused
C test, `-std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined
-fno-omit-frame-pointer -I include`, then runs with `ASAN_OPTIONS=detect_leaks=1`.
The author worktree's `resources/evidence.json` pins public sources and transitive
local imports, unchanged dependencies, media, tool binaries, reports, native
artifacts, and the queue snapshot. The handoff supplies its path and hash.

Pre-implementation recall: `deja "U1C 2E7D U25"` returned historical ownership
clearances in Claude sessions `997d7fe9-a03` and `9742b6e1-d50`. Only that ownership
context was reused; current remote and inbox checks supplemented it. Composition
idioms reuse `overlay_u1c_compare_and_0aed_cpu.c` and
`resident_0709_space_05eb_cpu.c`; identity/fixture/native mechanics reuse the
merged U25 loader/string/load-word and resident glyph/EGA audit modules.

## Author parity assessment

- target_function: complete OVL_001C_2E7D [2E7D,2F2D).
- source_candidates: `pr_u1c_text_2e7d_cpu` plus eight existing C dependencies.
- signals: exact body identity; full caller control-flow coverage; live CPU/RAM
  and partial-stop equality; independent native CPU/RAM/planes/OUT comparison.
- score: strong bounded-profile author evidence, not an independent acceptance score.
- verdict: likely_match within the documented profile.
- review_notes: retain closure coverage gaps, synthetic-only native reachability,
  flat-RAM aperture restrictions, and observer caveat. VP/reviewer decides
  acceptance and credit; this PR makes no accounting changes.
