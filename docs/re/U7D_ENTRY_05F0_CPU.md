# U7D +05F0 numeric display composition

The complete caller prints a title, two numeric fields and separators, then
an arithmetic-derived numeric field. It composes the existing raw +0565
formatter and resident printer throughout. Acceptance and credit belong to
the independent reviewer; this change does not edit accounting ledgers.

## Identity and selection

On base 48110db35ea7017d429dd348b2d4841b4125db9f, the refreshed schema-8
successor to the requested schema-4 queue ranks OVL_007D_05F0 43rd, score 59,
with all three direct targets ready: the next remaining substantive composable
body. Classification remains game_evidenced, confidence provisional, with no
exported entry slot. Eight aligned near-call sites are recorded in the queue.

The queue span is 305 bytes, but includes 73 trailing data bytes. The complete
executable is **[05F0,06D8), 232 bytes / 102 instructions**, ending with RETF 2
at +06D5. GAME.OVR file range [148412,148644), SHA256:
ac3eb0a915fa30d77b06b262429879f2400d3b8584b6e97ba413dde924709c7d.
The trailing bytes are not claimed as executable coverage.

| Call sites | Target | Convention |
|---|---|---|
| 0633 / 0695 | SYSTEM 0B0B StrLoad | fixup-backed far CALL |
| 0638 / 0677 / 069A / 06CD | resident 0709:0626 printer | fixup-backed far CALL |
| 0674 / 06CA | U7D +0565 formatter | PUSH CS; near CALL |

The formatter composes resident 00BA:1037, SYSTEM Str 1366/1254, StrStore,
StrLoad, StrCat and Copy. The printer composes StrStore, glyph 01DF and UpCase.
All eleven dependency C sources are unchanged.

## Behavior and profile

SS:[entry SP+4] supplies the selection low byte. Seven stack color words are
initialized to 10. The selection is sign-extended and used directly in a word
store of 15 at BP+2*index-0F. It is not clamped: values outside 0..6 can alter
other stack bytes, including the final return frame. Selection 7 overwrites
the low byte of saved BP; larger indices can change root RETF's destination.
Those final returns follow the live frame and are distinct from TRANSFER,
which detects unexpected dependency return addresses.

The title at CS:05E3 is copied into a local buffer and printed at x1, y17,
color 10. The loop visits indices 4 then 3, printing the low bytes of
DS:6DCA and DS:6DC8 at x12 and x15 in their selected colors. The separator at
CS:05EE prints at x14 and x17 in color 10. The final field at x18 uses color
index 2 and the low byte of the wrapping word expression
10*DS:6DC6+DS:6DC4. The formatter zero-prefixes single digits and returns the
first two decimal characters: 100 becomes "10", 255 becomes "25". This is
not a modulo-100 operation. Title, separator and formatter prefix remain live
code-segment data; original literal contents stay private.

The header defines the supported profile: exactly 1 MiB A20-off RAM, attached
EGA, entry IP05F0 and initial SP [0500,FFF6]. Initial DS/SS windows and all
caller or flat-RAM dependency operands exclude A0000..AFFFF. This is a
precondition, not an all-reads dynamic proof. Drawing uses the bus. Code and
bindings are immutable outside the aperture; writes avoid code, reached frames
do not wrap, and other data aliases remain live. CPU, bus, EGA, limits,
diagnostics and RAM occupy distinct storage. Execution is synchronous with
normal/386 flags; callbacks only observe or refuse OUT without mutating state.
No IRQ, fault, self-modification or arbitrary-PC resumption is modeled.
Modes 0/2 ordinary-memory effects do not establish CGA/Tandy hardware closure.

Caller budget counts taken JNE +0611/+06A7, shared across the two loops;
normal execution needs seven. LIMIT stops before the branch. Printer and
glyph budgets refresh on each invocation. Reached root call opcodes/offsets
are checked before CALL, and far segments are read live. A near-call CODE
stop preserves the preceding PUSH CS. CALLEE preserves nested diagnostics;
all partial stops retain CPU/RAM/hardware effects without rollback. ARGUMENT
preserves CPU/RAM/EGA and resets optional diagnostics. MUL's arithmetic flags
are immediately overwritten by ADD before any modeled stop.

## Measured author evidence

| Check | Result |
|---|---|
| Original-byte differential | 93 cases: 62 returns, 8 CODE, 20 CALLEE, 3 caller LIMIT |
| Original state | CPU14 + 983,040 ordinary RAM bytes; 24,217 ordered OUT events, 45,580 raw VRAM write operands; zero masks |
| Caller coverage | 102/102 instruction starts, 4/4 conditional outcomes |
| Registered dependency closure | 772/798 starts, 84/104 conditional outcomes |
| DOSBox-X EGA normal/386 | 31 returning cases: CPU14 + 198,656 RAM bytes + 262,144 plane bytes; 17,748 ordered OUT events; zero masks |
| ASan/UBSan semantics | 27,955 checks passed |
| Focused CMake/CTest | 1/1 passed |

Twelve C sources compile together. Original cases include selections outside
the seven-word array, signed byte extremes, numeric and wrapping arithmetic
values, empty/short synthetic labels, modes/flags/pages, missing font, SS/DS
aliasing, A20 stack addressing, stack bounds, all root and formatter call
site guards, resident/video guards, caller/nested limits, numeric table
rejection, and refused ports. The original oracle observes CODE/WRITE/OUT;
it does not install a memory-read hook.

Semantic tests independently check every byte value with all seven valid
selections, all 256 signed selection stores at the first guarded call, colors
and untouched neighbors across both pages and all four planes, partial loop
state, nested rejection and atomic invalid arguments. Synthetic all-set font
rows make expected EGA color bytes explicit.

Native plain execution captures CPU, DS/FONT/DATA 64 KiB each, a 2,048-byte
stack window and all planes. A separate guest INT1 run records exact ordered
OUT operands with a 2,048-event per-case capacity. The receipt reproduces
INPUT.BIN from the final fixtures. Native RESULT.BIN SHA256:
44d425133fb1dae106183f351de06b9b5c7b0b53484bef949d4d60919246819d.

Native cases retain original label contents and use synthetic returning
frames, including selection 7's saved-BP change. They exclude selection values
that alter the root return destination, partial stops and gameplay reachability.
No separate native hardware latch/register equality or directed unexpected
dependency-return test is claimed. Original comparison checks raw VRAM write
operands; native comparison checks actual planes. Full caller coverage does
not imply exhaustive dependency coverage: gaps include string overlap/length
alternatives, signed/general-width numeric conversion, UpCase, printer and
glyph branches. Exact missing starts/outcomes remain in the private report.

## Reproduction and provenance

Original media, extracted code, generated guests, captures and receipts stay
under ignored resources/. With local Capstone and Unicorn dependencies:

    export PYTHONPATH=resources/pythondeps:tools/overlay
    export PYTHONDONTWRITEBYTECODE=1
    python3 tools/overlay/u7d_entry_05f0_cpu_audit.py --unicorn --output resources/original.json
    python3 tools/overlay/u7d_entry_05f0_cpu_audit.py --native --output resources/native.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_u7d_entry_05f0_cpu_tests -j4
    ctest --test-dir resources/build -R '^poolrad_u7d_entry_05f0_cpu_tests$' --output-on-failure

Sanitizers compile the audit's SOURCES and focused test with C11,
Wall/Wextra/Wpedantic/Werror, debug symbols, address/undefined sanitizers,
frame pointers and include path include; run with ASAN_OPTIONS=detect_leaks=1.
The private receipt pins source/header and local Python import closures,
reports, queue inputs, tool/media hashes and native artifacts. VP handoff
provides its path and SHA256.

Pre-implementation memory context succeeded for U7D 05F0 0565 raw CPU.
Session 281c8700-5acc-438b-b93d-c061e28437eb identifies the Claude lane's
+0565 raw far-call composition; session 01a14ffa-0647-7309-abe5-3a04fb3869ab
provides historical verification navigation. Current merged contracts,
original bytes, refreshed queue, inbox and remote ownership checks establish
this work's scope. The retained memory-context.txt records the query results.
Historical credit and ownership messages are not current acceptance evidence.

The caller/audit/native framework adapts
[PR #1078](https://github.com/huntergdavis/poolrad-decomp/pull/1078).
The [U7D +0565 implementation](../../src/recovered/overlay_u7d_two_digits_0565_cpu.c)
and existing printer audit supply hash-checked original bodies and established
raw interfaces. The receipt verifies all compiled/imported dependencies at the
stated base. Accounting files are included only as read-only provenance.

## Author parity assessment

- target_function: complete OVL_007D_05F0 [05F0,06D8).
- source_candidates: pr_u7d_entry_05f0_cpu and unchanged raw dependencies.
- signals: original identity, complete caller execution coverage, CPU/RAM and
  partial-stop equality, independent native CPU/RAM/planes/OUT comparison.
- score: strong documented-profile author evidence, not independent acceptance.
- verdict: likely_match within the documented profile.
- review_notes: retain provisional identity, queue code/data correction,
  signed unchecked selection effects, ordinary-RAM restrictions, dependency
  coverage gaps, synthetic native scope and independent acceptance/credit.


## Checker registration (review lane, 2026-10-20)

`CHECKER_ROWS = ('OVL_007D_05F0',)` registers this audit with the handoff gate. `--compile-only` runs the audit's own
`library()` build (`-std=c11 -Wall -Wextra -Wpedantic -Werror`) before any media access and prints `CHECKER_BUILD_OK`.
Registration makes the row checkable; it is **not** a credit. This row is `game_evidenced`, so acceptance still needs a
two-factor identity argument, which this document does not yet carry.

## Identity under the two-factor standard (par-a, 2026-10-20, VP i20127)

The standard is in [REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#identity-standard-two-factor-adopted-2026-10-20). Everything below was re-derived from GAME.OVR (sha `872964a3…`) and from the CSVs on main `eb3149ac`. Each listed site decodes as `0E` (push cs) followed by `E8` call to this entry, and lies inside its caller's `overlay.csv` bound. They are near calls, so the relocation clause does not apply (ruling i19576). This section is an argument, not a credit.

1. **Reached from game code: PASS.** There are 8 sites in five callers, all `game_evidenced` and **none credited**, which is the same basis as `OVL_03B0_11F3` and tonight's four U3B0 rows:
   - `+0734` in `OVL_007D_0721` [0721,0871);
   - `+08D1` in `OVL_007D_0871` [0871,0925);
   - `+0993` in `OVL_007D_0925` [0925,09E7);
   - `+0AB9` in `OVL_007D_09E7` [09E7,0B0A);
   - `+0D13`, `+0D5E`, `+0DAD` and `+0E13` in `OVL_007D_0C9A` [0C9A,0E60).
2. **Operates on documented game state: PASS.** The body reads a byte at `DS:[DI+6DC2h]` and the words `DS:6DC4` and `DS:6DC6`, all inside the seven-word array `DS:6DC2..6DCF`.
   - The native oracle of credited `OVL_007D_03A3` ([U7D_UPDATE_03A3_NATIVE_ORACLE.md](U7D_UPDATE_03A3_NATIVE_ORACLE.md), 128 cases) rebuilds that array as game state, alongside the party list at `DS:5D96`. `03A3` is in both `strict_full_c_addresses.csv` and `reviewed_function_identities.csv`.

**Limit:** the array's layout and its updater are documented, but no document names its gameplay meaning, and this section does not claim one.

**Verdict: game code.**


## Reviewer acceptance record (added by the review lane, not the author)

Reproduced by par-e on main `c9d80b75` (2026-10-20). Unicorn: 93 cases (62 return, 8 CODE, 20 CALLEE, 3 LIMIT); caller 102/102 starts, 4/4 outcomes; closure 772/798 and 84/104; 983,040 ordinary RAM bytes + aperture write stream (24,217 OUTs, 45,580 VRAM operands); ASan/UBSan 27,955; CPU14, flag masks 0. Native: 31 cases, RESULT.BIN `44d42513…` (matches this document). Body 232 B, SHA-256
`ac3eb0a915fa30d77b06b262429879f2400d3b8584b6e97ba413dde924709c7d`, from GAME.OVR. Identity: par-a's two-factor section above (#1282). Each factor-1 site was checked from the
media: it decodes as `0E E8` and sits at an instruction boundary inside the caller extent par-a names.

**Reviewer-added limit (native oracle):** the native run used `/usr/bin/dosbox-x` 2026.01.02, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, not the pinned `5a9c7c53` build. This document
did not name the binary and gives no insulation argument for it. The primary evidence is the full-1 MiB Unicorn
differential. Native agreement is claimed for that binary only; native frames are synthetic, not a reached game caller.

## Pinned flag split after #1433 (2026-10-22, no claim moves)

After #1433 a composed shift sets AF per the pinned rule; raw Unicorn leaves it clear, so the audit failed on main at `bad-root-674` (`flags: C 0012, original 0002`, AF).

The audit now declares the split through the shared `tools/overlay/pinned_shift_flags.py`: it tracks the last FLAGS-writing instruction the original executed (decoded from the original bytes at run time) and requires C to equal Unicorn **exactly**, except that after a word `DIV`/`IDIV` (or word `MUL`) the six arithmetic bits must equal the pinned `instructions.h` values, and after a shift with a non-zero count AF must be **set** (pinned `get_AF` = `count & 1Fh`). A model that reverted to the Unicorn value fails. Nothing is masked; the audit reports how many stops used the split, by rule.

Run (PR #1446, `--unicorn`): **93** cases, registered dependency closure 772/798 starts (unchanged from the documented figure); the split was used at **2** stops, both after a shift.

**Per-credit sentence (OVL_007D_05F0): evidence did not depend on AF after a shift**; those 2 stops hold C to AF=1 and every other stop is exact. Credit stands.
