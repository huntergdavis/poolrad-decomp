# U3B0 +0C33 page movement and redraw composition

The complete caller preserves the cursor's displacement from the visible list
base, moves that base by a page with signed clamping, runs the existing raw
+0B03 step scanner, and redraws through raw +07A4. All reachable game and
runtime callees use existing implementations. Acceptance and credit belong
to the independent reviewer; no accounting ledger is changed.

## Identity and selection

On tested base 6fdc7eb1a96c18bd16306fc0930088a0a2fc3eeb, the refreshed schema-8 successor
to the requested schema-4 queue ranks OVL_03B0_0C33 38th, score 60, with two
of two direct targets ready: the highest remaining substantive composable
body. +0B03 is strict-credited; +07A4 is modeled, with independent acceptance
PR #1081 still open at selection. Raw interface availability does not itself
establish acceptance or credit. Classification remains game_evidenced,
confidence provisional; there is no exported entry slot. Four aligned
PUSH CS/CALL sites in U3B0 +0D9A reach this helper: +1023, +1038, +1049, +1057.

Complete **[0C33,0CC5), 146 bytes / 49 instructions**, RETF 4 at +0CC2.
GAME.OVR file range [185602,185748), SHA256:
c7cb3ab8625088a43a7c7d35f9fa21dde2252bdfa9132e2fc5592070489fe86a.
This queue extent is entirely executable.

| Call site | Target | Convention |
|---|---|---|
| 0CB1 | U3B0 +0B03 step scan | PUSH CS; near CALL; RETF 4 |
| 0CBC | U3B0 +07A4 viewport display | PUSH CS; near CALL; RETF 4 |

The scanner composes +0000. The display composes +0000, +0711, resident
03F1:0137 clear and resident 0709:0626 printer/01DF glyph, with their full
runtime and drawing chains. Sixteen C sources compile together, fifteen
unchanged dependency implementations.

## Behavior and supported profile

SS:[entry SP+4] is the parent's context offset L in SS; [SP+6] supplies a
direction low byte. The caller reads the far pointer X at SS:[L+0A] and
saves the wrapping difference X^ minus DS:[6B29] in its local word. A nonzero
direction adds SS:[L-59] to DS:[6B29], then clamps the result against the
wrapping word SS:[L-56] minus SS:[L-59] using signed comparison. Zero
subtracts SS:[L-59] and clamps a signed-negative result to zero.

It adds the saved displacement to the new base and writes X^, rereading
X's pointer first. It passes the direction and context to +0B03, then passes
the live DS:[6B29] and context to +07A4. Every operand is read in original
order, so aliases among the index, global, context and stack locals remain
observable. Direction uses only the low byte. Arithmetic wraps at 16 bits;
the signed clamps are not unsigned saturation.

The header defines the profile: exactly 1 MiB A20-off RAM, attached EGA,
entry IP0C33 and initial SP [0320,FFF4]. Initial DS/SS windows and all caller
or flat-RAM dependency operands exclude A0000..AFFFF. This is a precondition,
not an all-reads dynamic proof. Drawing uses the bus. Code and bindings are
immutable outside the aperture, writes avoid code, reached frames do not
wrap, and other data aliases remain live. CPU, bus, EGA, limits, diagnostics
and RAM occupy distinct storage. Execution is synchronous with normal/386
flags; callbacks only observe or refuse OUT without changing state. No IRQ,
fault, self-modification or arbitrary-PC resumption is modeled. Modes 0/2
ordinary-memory effects do not establish CGA/Tandy hardware closure.

Root near-call opcode/offset checks happen after PUSH CS and before CALL;
CODE preserves that pushed word. Inherited callee checks remain unchanged:
+0B03 does not guard its internal selector call bytes. Immutable original
bindings are a precondition there. The step budget uses the existing unsigned
loopback count and stops at the loop head after the exhausted jump. Display
budgets retain their existing before-jump stops and per-call resets. CALLEE
preserves the step selector status and nested display diagnostics. TRANSFER
handles unexpected dependency returns; root RETF reads its live frame.
Partial stops retain all preceding effects without rollback. ARGUMENT
preserves CPU/RAM/EGA and resets optional diagnostics.

Integration base 3b5ce8e40e3737539fe8229924ced35e63875656 includes independent
+07A4 acceptance PR #1081. Only queue documentation and accounting checkpoint
metadata changed from the tested base. The receipt verifies that every compiled
and imported dependency is byte-identical across both bases.

## Measured author evidence

| Check | Result |
|---|---|
| Original-byte differential | 118 cases: 74 returns, 2 root CODE, 42 nested CALLEE stops |
| Original state | CPU14 (no flag mask) + the **entire 1,048,576-byte backing RAM**, no excluded range; independent EGA planes/latches/registers (262,164 B); 10,792 ordered OUT events, 30,802 VRAM write operands. **Superseded measurement:** the author run compared only 983,040 bytes, excluding `A0000..AFFFF` (65,536 B), and its "zero masks" referred to flags only; see *Aperture re-audit* |
| Caller coverage | 49/49 instruction starts, 6/6 conditional outcomes |
| Registered dependency closure | 1171/1187 starts, 118/132 conditional outcomes |
| DOSBox-X EGA normal/386 | 44 returning cases: CPU14 (no flag mask) + 197,632 RAM bytes + 262,144 plane bytes; 10,834 ordered OUT events. **Native RAM is partial: 850,944 backing bytes are not compared natively** (unchanged by the re-audit) |
| ASan/UBSan semantics | 13,090 checks passed |
| Focused CMake/CTest | 1/1 passed |

Original cases exercise both directions and both clamp outcomes, zero/nonzero
high direction bytes, signed word extremes, arithmetic wrap, marked/mixed
nodes, empty strings, missing font, null list, stack bounds, A20 stack/index
addressing, SS/DS and index/global/page-size/local aliases, wrapped context
and index addressing, modes/flags/pages, both root guards, inherited display
and drawing guards, refused ports and nested budgets. The step limit cases
compare its actual loop-head stop; display stops remain at the exhausted
branch. CPU flags are compared without masks. The oracle uses CODE/WRITE/OUT
hooks and does not install a memory-read hook.

Independent semantic checks cover 2,662 combinations of signed-boundary word
values and both directions at the first guarded call, every direction low
byte with high-byte noise through the returning chain, global/local alias
updates, exact EGA colors and black padding with unchanged neighbors, nested
limits, refused ports, pre-call stack state and atomic invalid arguments.

Native plain execution captures CPU, DS/FONT/DATA 64 KiB each, a 1,024-byte
stack window and all four planes. A separate guest INT1 run records exact
ordered OUT operands with a 2,048-event per-case capacity. INPUT.BIN is
reproduced from the final fixtures by the receipt generator. RESULT.BIN SHA256:
3b50f02612d1cd42eb0817c5f53203907caae499fe70a569230fffd6c5c5a473.
Native cases include both directions, both clamps, flagged and mixed nodes,
null list, a finite cyclic viewport, page sizes zero/one/larger-than-total,
styles/widths, coordinates, flags and pages. They are synthetic returning
frames, not gameplay reachability or native partial-stop/alias/wrap evidence.
No separate native latch/register equality or directed unexpected dependency
return case is claimed. Original comparison checks raw VRAM write operands
and, since the re-audit, the independent EGA's planes, latches and registers;
native comparison checks actual planes.

Full caller coverage does not imply exhaustive dependency coverage. Missing
starts/outcomes include +0B03's secondary-bound clamps, clear/rectangle
alternatives, StrStore, UpCase, printer and glyph branches; exact gaps remain
in the private report. The corrected wrapped-context fixture explicitly wraps
effective offsets when installing parent/index pointers. Its initial fixture
failure log is retained. This correction changes no native fixture inputs,
as verified by exact INPUT.BIN reproduction.

## Reproduction and provenance

Original game media, extracted bytes, generated guests, captures and receipts
stay under ignored resources/. With local Capstone and Unicorn dependencies:

    export PYTHONPATH=resources/pythondeps:tools/overlay
    export PYTHONDONTWRITEBYTECODE=1
    python3 tools/overlay/u3b0_entry_0c33_cpu_audit.py --unicorn --output resources/original.json
    python3 tools/overlay/u3b0_entry_0c33_cpu_audit.py --native --output resources/native.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_u3b0_entry_0c33_cpu_tests -j4
    ctest --test-dir resources/build -R '^poolrad_u3b0_entry_0c33_cpu_tests$' --output-on-failure

Sanitizers compile the audit's SOURCES and focused test with C11,
Wall/Wextra/Wpedantic/Werror, debug symbols, address/undefined sanitizers,
frame pointers and include path include; run with ASAN_OPTIONS=detect_leaks=1.
The private receipt pins source/header and local Python import closures,
reports, queue inputs, tool/media hashes and native artifacts. VP handoff
provides its path and SHA256.

Shared memory context preceded implementation. Query U3B0 0C33 07A4 raw CPU
ownership returned sessions 01a1281b-85e3-7e23-bdf6-1ea8af4d69de and
01a1386e-bea7-7883-8ddd-06fd2be8db9b, identifying prior +07A4 ownership
screens and related raw recovery work. These are navigation evidence; current
source contracts, original bytes, refreshed queue, inbox and remote PR checks
establish this work's scope. A wrap-fixture query also returned prior operand
addressing discussions, retained in memory-wrap-context.txt. The fix follows
the actual helper and original instructions, not a historical parity claim.

The caller/audit/native framework adapts the merged
[+07A4 composition](U3B0_ENTRY_07A4_CPU.md) from
[PR #1080](https://github.com/huntergdavis/poolrad-decomp/pull/1080).
The [step scanner](U3B0_STEP_SCAN_0B03.md) supplies the original-body identity,
raw interface and loop-stop contract. This change adds the enclosing page
movement, live alias handling, full step/display composition and nested stops.

## Author parity assessment

- target_function: complete OVL_03B0_0C33 [0C33,0CC5).
- source_candidates: pr_u3b0_entry_0c33_cpu and unchanged raw dependencies.
- signals: pinned original identity, complete caller execution coverage,
  CPU/RAM and partial-stop equality, independent native CPU/RAM/planes/OUT.
- score: strong documented-profile author evidence, not independent acceptance.
- verdict: likely_match within the documented profile.
- review_notes: retain provisional identity, signed arithmetic and live aliases,
  ordinary-RAM restrictions, inherited unchecked selector sites, dependency
  coverage gaps, synthetic native scope and independent acceptance/credit.

## Aperture re-audit (doc-truth correction)

The original author evidence compared 983,040 ordinary RAM bytes and said
"zero masks" without qualification. Flags were unmasked, but the 65,536-byte
video aperture `A0000..AFFFF` was excluded from the RAM compare, the original
differential checked only the VRAM write operands (not plane contents), and the
native row's "zero masks" sat beside a partial capture. The checker was also not
registered with the checker-build gate.

This revision re-runs the same 118 cases with the aperture served by the
independent Python EGA used for `RES_00BA_1693` (MMIO). The compare covers CPU14
without flag masks, **all 1,048,576 backing RAM bytes**, ordered OUT events, every
write operand and the full device state (four planes, latches, sequencer and
graphics registers; 262164 bytes). Excluded ranges: **none**. Result: **118 cases, zero
mismatches**, stops `{RETURN 74, CODE 2, CALLEE 42}`; caller **49/49 starts, 6/6 outcomes**; closure
**1171/1187 starts, 118/132 outcomes** (the unexecuted starts and outcomes lie in delivered
callees and are listed in the ignored report); 10,792 OUT events, 30,802 VRAM write
operands; 0 aperture reads. The earlier doc's stop split was 74 returns, 2 root
CODE and 42 nested CALLEE; the re-run's split is above.

The audit now declares `CHECKER_ROWS = ('OVL_03B0_0C33',)` and a media-free
`--compile-only` mode using the audit's own compiler command and symbol binding,
so `check_checker_builds.py` covers it. No body, header or model changed. Native
evidence is unchanged and still partial (above). The selection note above that
calls `+07A4` "modeled, acceptance PR #1081 open" is historical; `+07A4`'s own
evidence is re-audited separately (PR #1202). Strict credit, identity and manifest
decisions are untouched.

## Identity under the two-factor standard (par-a, 2026-10-20, i20050)

The standard is in [REVIEW_LANE_HANDOFF.md](REVIEW_LANE_HANDOFF.md#identity-standard-two-factor-adopted-2026-10-20). Everything below was re-derived from GAME.OVR (unit `0x3B0` at file offset 182,479) and from the CSVs on main. This section is an argument, not a credit. Credit is an acceptance commit by par-e.

1. **Reached from game code (near call).** Each site below decodes from the media as `0E` (push cs) followed by `E8` call to this entry. Each lies inside the caller's ledger extent (`overlay.csv` bound), so the relocation clause does not apply (ruling i19576). The callers are `game_evidenced`, and **none is strict-credited**, so the basis is the classification, as for `OVL_03B0_11F3`.
   - Sites: `+1023`, `+1038`, `+1049` and `+1057` in `OVL_03B0_0D9A` [0D9A,10D1).
2. **Operates on documented game state.** The body reads and writes exactly the parent-frame slots that credited `OVL_03B0_0CC5`'s doc ([U3B0_CURSOR_STEP_0CC5.md](U3B0_CURSOR_STEP_0CC5.md)) defines, then composes credited `OVL_03B0_0B03` and `OVL_03B0_07A4`.
   - The slots: limit `L-59h`, bound `L-56h`, the index far pointer `X` at `L+0Ah/0Ch`, and the list base `G = DS:[6B29h]`.
   - `0CC5`, `0B03` and `0969` are each in both `strict_full_c_addresses.csv` and `reviewed_function_identities.csv`.


## Reviewer acceptance record (added by the review lane, not the author)

Reproduced by par-e on main `813a9d06` (2026-10-20). Unicorn: 118 cases; caller 49/49 starts, 6/6 outcomes, CPU14 + 1,048,576 B, `ram_excluded_ranges []`,
flag masks 0. Native: 44 cases, RESULT.BIN `3b50f026…` (matches this document). Body 146 B, SHA-256
`c7cb3ab8625088a43a7c7d35f9fa21dde2252bdfa9132e2fc5592070489fe86a`, from GAME.OVR. Identity: par-a's two-factor section above (#1262). Each factor-1 site was checked from the
media: it decodes as `0E E8`, sits at an instruction boundary inside a `game_evidenced` caller, and that caller is
not strict.

**Reviewer-added limit (native oracle):** the native run used `/usr/bin/dosbox-x` 2026.01.02, SHA-256
`99d7460153175d13a21036fb4b0e7524e05b6a1e50cd4e59f2470afad12a0df3`, not the pinned `5a9c7c53` build. This document
did not name the binary and gives no insulation argument for it. The primary evidence is the full-1 MiB Unicorn
differential, with the EGA device compared. Native agreement is claimed for that binary only.
