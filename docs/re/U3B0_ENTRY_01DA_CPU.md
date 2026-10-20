# U3B0 +01DA raw prompt-row drawer

This adds the complete raw CPU caller, composing SYSTEM set membership and
the full resident glyph implementation. It preserves stack, registers,
flags, RAM, OUT ordering and drawing effects across the selected-range,
set-member, ordinary-color and trailing-clear paths. Independent acceptance
and identity classification remain reviewer responsibilities. No accounting
ledger is changed and no game-function credit is claimed.

## Identity and selection

On tested/integration base 3e5a334e44e052b594062c4d8c4f6154c3819a88, the refreshed
schema-8 successor to requested schema-4 ranks OVL_03B0_01DA 355th, score 26,
with two of two direct targets ready. It is the next remaining substantive
composable source-mention body. Classification remains
**game_or_other_library_unresolved / unresolved**, with no exported slot.
Four aligned PUSH CS/CALL sites in U3B0 +03BD reach it: +048B, +05E2, +0606,
and +0685. The existing slot_a_u3b0_prompt_row implementation is a bounded
page adapter, not this raw CPU interface.

The queue's 483-byte span includes 64 trailing data bytes. The complete
executable is **[01DA,037D), 419 bytes / 172 instructions**, RETF 4 at +037A.
GAME.OVR file range [182953,183372), SHA256:
8676c97ae6dcaa88c9f7d68d75c75f81e4936572b517bd84a7e19876c58d96ce.
The trailing bytes are not claimed as executable coverage.

| Call sites | Target | Convention |
|---|---|---|
| 0273 / 02CA / 0305 / 0372 | resident 0709:01DF glyph | fixup-backed far CALL, RETF 12 |
| 028F | SYSTEM 0AF8:0DC0 set membership | fixup-backed far CALL, RETF 6 |

The glyph composes SYSTEM UpCase at +182A. Five C sources compile together:
one new caller and four unchanged dependency implementations. The 32-byte
set at CS:01BA is data, excluded from decoded coverage; SHA256:
2d66b9ae33311a4e813f8a5263360a0aaeb0e64d86f53bdeeb2f0849d5e2cbe0.
Its original contents remain private. Fixtures can supply synthetic set bits
before entry; the set is immutable during execution.

## Behavior and supported profile

SS:[entry SP+4] supplies context offset L in SS; [SP+6] supplies a selection
low byte. Length is the byte at L-64, starting column at L-65, character i at
L+i-53 for one-based i. A zero length returns without clearing. Nonzero
length is copied to a local byte; the loop counter starts at one. Selection
range endpoints are unsigned bytes at L+2*selection-8F and -8E. When the
position lies in that inclusive range and selected color L+0E is nonzero,
the character uses that color. Otherwise SYSTEM tests its bit in CS:01BA:
a member uses L+0E even when that color is zero; a nonmember uses L+10.
Each glyph uses transform 1, count 1 and row 24. Its x argument comes from
starting column plus the current position minus one; the glyph consumes the
low byte. The caller does not impose the bounded adapter's coordinate clip.

After the loop, the caller rereads length and starting column. If their sum
is below 39, it issues one black-space glyph call beginning at that sum with
count 40 minus the sum. Thus sum 38 clears columns 38 and 39, while sum 39
skips the tail call. The [older bounded adapter](SLOT_A_PREINPUT_PROMPT_ROW_PAGE.md)
limits its page operation to column 38; this raw interface preserves the
original caller's different tail operands. Synthetic EGA checks and original
native execution cover both sides of this boundary. The older adapter and
its saved-game witness are unchanged; no new gameplay replay is claimed.

The header defines the profile: exactly 1 MiB A20-off RAM, attached EGA,
entry IP01DA and initial SP [0040,FFF4]. Initial DS/SS windows and all caller
or flat-RAM dependency operands exclude A0000..AFFFF. This is a precondition,
not an all-reads dynamic proof. Drawing uses the bus. Code, bindings and set
are immutable outside the aperture; writes avoid them, reached frames do not
wrap, and other data aliases remain live. CPU, bus, EGA, limits, diagnostics
and RAM occupy distinct storage. Execution is synchronous with normal/386
flags; callbacks only observe or refuse OUT without changing state. No IRQ,
fault, self-modification or arbitrary-PC resumption is modeled. Modes 0/2
ordinary-memory effects do not establish CGA/Tandy hardware closure.

Caller budget counts taken JMP +0312; LIMIT stops before its transfer.
Glyph budgets refresh per invocation. Reached root far opcodes and target
offsets are checked before CALL, with segment words read live. CALLEE keeps
nested glyph/UpCase status; TRANSFER detects unexpected dependency returns.
Root RETF uses its live frame. Partial effects remain without rollback.
ARGUMENT preserves CPU/RAM/EGA and resets optional diagnostics.

## Measured author evidence

| Check | Result |
|---|---|
| Original-byte differential | 95 cases: 78 returns, 5 CODE, 10 CALLEE, 2 caller LIMIT |
| Original state | CPU14 (no flag mask) + the **entire 1,048,576-byte backing RAM**, no excluded range; independent EGA planes/latches/registers (262,164 B); 25,543 ordered OUT events, 48,076 VRAM write operands. **Superseded measurement:** the author run compared only 983,040 bytes, excluding `A0000..AFFFF` (65,536 B), and its "zero masks" referred to flags only; see *Aperture re-audit* |
| Caller coverage | 171/172 instruction starts, 15/16 conditional outcomes; **171/171 feasible starts and 15/15 feasible outcomes** after the machine-checked infeasible edge `+01FC→+01FE` (and its start `+01FE`), both kept in the denominator |
| Registered closure | 592/596 starts, 67/72 conditional outcomes |
| DOSBox-X EGA normal/386 | 42 returning cases: CPU14 (no flag mask) + 197,632 RAM bytes + 262,144 plane bytes; 38,998 ordered OUT events. **Native RAM is partial: 850,944 backing bytes are not compared natively** (unchanged by the re-audit) |
| ASan/UBSan semantics | 19,047 checks passed |
| Focused CMake/CTest | 1/1 passed |

The missing caller start is +01FE, reached only by the untaken outcome of
JBE +01FC. The entry test has already required a nonzero length, that byte
has just been copied to BP-2, and AL is one. With synchronous execution and
no intervening write, one is always at most that nonzero byte. Both the
instruction and outcome remain in the denominator; coverage is not masked.
The other missing starts are glyph +021A, +021D and +022D. Exact five missing
conditional outcomes remain in the private report. Complete translation is
not a claim that every dependency branch was exercised.

Original cases cover all three color paths, zero selected color, reversed
ranges, empty/long strings, unsigned selection extremes and high-byte noise,
character set bits, mode/flags/column variants, missing font, SS/DS aliasing,
A20 stack addressing, wrapped context fields, stack bounds, all five root
call guards, the UpCase guard, caller/glyph limits and refused ports. Long
synthetic text can overlap context fields; actual operand bytes remain live.
The oracle uses CODE/WRITE/OUT hooks, without a memory-read hook.

Independent semantic tests cover all 256 characters under both membership
results and three selection/color states, every starting-column byte at the
tail boundary, zero-length no-clear behavior, exact colors and black padding
on both EGA pages/all planes, unchanged neighbors, caller/nested stops and
atomic invalid arguments. Character classification is checked before the
glyph's UpCase transformation.

Native plain execution captures CPU, DS/FONT/DATA 64 KiB each, a 1,024-byte
stack window and all four planes. A separate guest INT1 run records exact
ordered OUT operands with a 2,048-event per-case capacity. INPUT.BIN is
reproduced from final fixtures by the receipt generator. RESULT.BIN SHA256:
78890ae85ce06588989ab809343d965da6405ab7157beb7f9fa9db920ea1276d.
Native uses the original set and synthetic text/context/returning frames;
modified set bitmaps are original-differential and semantic cases only.
No native partial-stop, wrapped-context, gameplay reachability, separate
latch/register equality or directed unexpected dependency-return claim is
made. Original comparison checks raw VRAM operands and, since the re-audit, the independent EGA's planes, latches and registers; native checks planes.

## Reproduction and provenance

Original media, extracted bytes, generated guests, captures and receipts stay
under ignored resources/. With local Capstone and Unicorn dependencies:

    export PYTHONPATH=resources/pythondeps:tools/overlay
    export PYTHONDONTWRITEBYTECODE=1
    python3 tools/overlay/u3b0_entry_01da_cpu_audit.py --unicorn --output resources/original.json
    python3 tools/overlay/u3b0_entry_01da_cpu_audit.py --native --output resources/native.json
    cmake -S . -B resources/build -DBUILD_TESTING=ON
    cmake --build resources/build --target poolrad_u3b0_entry_01da_cpu_tests -j4
    ctest --test-dir resources/build -R '^poolrad_u3b0_entry_01da_cpu_tests$' --output-on-failure

Sanitizers compile the audit's SOURCES and focused test with C11,
Wall/Wextra/Wpedantic/Werror, debug symbols, address/undefined sanitizers,
frame pointers and include path include; run with ASAN_OPTIONS=detect_leaks=1.
The private receipt pins source/header and local Python import closures,
reports, queue inputs, tool/media hashes and native artifacts. VP handoff
provides its path and SHA256. The four accounting CSVs are read-only provenance.

Pre-implementation shared memory context for U3B0 01DA prompt row drawer full
raw CPU ownership succeeded. Session 01a12b3e-db20-70e1-95f9-d3fe44b8d309
identified the earlier page witness; session 4d9851e1-f261-48b9-9cfa-5876d1500eb7
provided historical relative-clear screens. These are navigation evidence,
not current ownership or acceptance. Current inbox, remote PRs, source
contracts, original bytes and refreshed queue establish this submission's
scope. memory-context.txt retains the query result.

The [earlier page witness](SLOT_A_PREINPUT_PROMPT_ROW_PAGE.md) supplies the
419-byte identity and context interpretation. The raw caller/audit framework
adapts [U7D +05F0](U7D_ENTRY_05F0_CPU.md); native capture mechanics adapt
[U3B0 +0C33](U3B0_ENTRY_0C33_CPU.md). The existing glyph audit supplies its
hash-checked original body and UpCase. SYSTEM +0DC0 is read from the pinned
unpacked executable and composed through the existing raw runtime leaf.

## Author parity assessment

- target_function: complete OVL_03B0_01DA [01DA,037D).
- source_candidates: pr_u3b0_entry_01da_cpu and unchanged raw dependencies.
- signals: original identity, reachable caller coverage, CPU/RAM and partial
  stops, independent native CPU/RAM/planes/OUT, semantic tail/color checks.
- score: strong documented-profile author evidence; identity remains unresolved.
- verdict: likely_match within the documented profile.
- review_notes: retain unresolved game/library ownership, code/data correction,
  unreachable branch denominator, ordinary-RAM restrictions, bounded-adapter
  distinction, remaining glyph outcomes, synthetic scope and independent review.

## Aperture re-audit and extent repair

**Masking correction.** The original author evidence compared 983,040 ordinary
RAM bytes and said "zero masks". Flags were unmasked, but the 65,536-byte video
aperture `A0000..AFFFF` was excluded, the original differential checked only VRAM
write operands (not plane contents), and the native row's "zero masks" sat beside
a partial capture. This revision re-runs the same 95 cases with the aperture served
by the independent Python EGA used for `RES_00BA_1693` (MMIO): CPU14 without flag
masks, **all 1,048,576 backing RAM bytes**, ordered OUT events, every write operand
and the full device state (262164 bytes). Excluded ranges: **none**. Result: **95 cases,
zero mismatches**, stops `{RETURN 78, CODE 5, CALLEE 10, caller LIMIT 2}`
(unchanged); 25,543 OUT events, 48,076 VRAM write operands, 0 aperture reads. Closure
**592/596 starts, 67/72 outcomes**; caller **171/172 starts, 15/16 outcomes**, i.e.
**171/171 feasible starts and 15/15 feasible outcomes**.

**The infeasible caller edge, now machine-checked.** `+01E3` compares the byte at
`SS:[DI-64h]` with zero and `+01E8 JA` enters only when it is nonzero; `+01ED`
reloads DI from the same unwritten `[BP+6]` and `+01F0` reads the *same* byte again
with no intervening memory write, so `[BP-2]` >= 1 and `+01F9 CMP 1,[BP-2]` makes
`JBE +01FC` always taken. The fall-through `+01FC→+01FE`, and the `JMP +0315` at
`+01FE`, never execute. Feasibility rests on immutable code, flat RAM and
synchronous execution (no interrupt or DMA modifies the byte between two
instructions). The audit keeps both items in the denominator and **fails the run if
either is observed**; they were not observed in 95 cases.

**Extent repair (64 trailing bytes).** The queue and ledger bound `[01DA,03BD)`
(483 B / 176 instructions) includes a 64-byte tail that is data. The executable is
`[01DA,037D)`, 419 B / 172 instructions, SHA-256
`8676c97ae6dcaa88c9f7d68d75c75f81e4936572b517bd84a7e19876c58d96ce`, one RETF
(`+037A`). The reasoning, hashes and the hash-bound `function_ledger.py` entry are
recorded in [QUEUE_METADATA_REPAIR.md](QUEUE_METADATA_REPAIR.md). In short: no
instruction in the body branches to or loads from `[037D,03BD)`, and the next
routine `+03BD` loads `CS:037D` and `CS:039D` as the two 32-byte SYSTEM `SET IN`
constants it uses. This change does **not** edit `overlay_function_ledger.csv`.

The audit now declares `CHECKER_ROWS = ('OVL_03B0_01DA',)` and a media-free
`--compile-only` mode, so `check_checker_builds.py` covers it. No body, header or
model changed. Native evidence is unchanged and partial (above). Classification
stays `game_or_other_library_unresolved`; strict credit, identity and manifest
decisions are untouched. The earlier "identity pending" caveats in this document
are unchanged.
